#include "pid.h"
#include "command.h"
#include "lumminaire.h"
#include "consensus.h"

#include <hardware/flash.h> //for flash_get_unique_id
#include "mcp2515.h"
#include <queue> // std::queue
#define TIME_ACK 2500

// Lumminaire
const int LED_PIN = 15;
const int DAC_RANGE = 4096;
const float VCC = 3.3;
const float adc_conv = 4095.0 / VCC;
const float dutyCycle_conv = 4095.0 / 100.0;
pid my_pid{0.01, 0.15, 1.5}; // h, k, Tt
// (float _h, float _K, float Tt_,float b_,float Ti_, float Td_, float N_)
lumminaire my_desk{-0.89, log10(225000) - (-0.89), 0.0158}; // m, b(offset), Pmax, desk_number
// system my_desk{float _m, float _offset_R_Lux, float _Pmax, unsigned short _desk_number}
float read_adc;
bool debbuging = false;

communication comm;

// TIMERS AND INTERRUPTS
struct repeating_timer timer;
volatile bool timer_fired{false};
const byte interruptPin{20};
volatile byte data_available{false};

void read_interrupt(uint gpio, uint32_t events)
{
  data_available = true;
}

bool my_repeating_timer_callback(struct repeating_timer *t)
{
  if (!timer_fired)
  {
    timer_fired = true;
  }
  return true;
}

// Consensus
Node node;
bool consensusRunning = false;
int consensusIteration = 0;
int maxiter = 100;
double otherD[2][3];
double K[3][3];

enum consensusStage : uint8_t
{
  CONSENSUSITERATON,
  CONSENSUSWAIT
};

consensusStage consensusStage;

void setup()
{ // the setup function runs once
  Serial.begin(115200);
  analogReadResolution(12);    // default is 10
  analogWriteFreq(60000);      // 60KHz, about max
  analogWriteRange(DAC_RANGE); // 100% duty cycle
  // Gain measurement at the beginning of the program
  Gain();
  add_repeating_timer_ms(-10, my_repeating_timer_callback, NULL, &timer);
}

void setup1()
{
  flash_get_unique_id(this_pico_flash_id);
  node_address = this_pico_flash_id[5];
  can0.reset();
  can0.setBitrate(CAN_1000KBPS);
  can0.setNormalMode();
  gpio_set_irq_enabled_with_callback(interruptPin, GPIO_IRQ_EDGE_FALL, true, &read_interrupt);

  // Connection timers
  randomSeed(node_address);
  time_to_connect += (random(21) * 100); // To ensure that the nodes don't connect at the same time
  connect_time = millis();
}

void loop()
{ // the loop function runs cyclically
  controllerLoop();
}

void loop1()
{
  communicationLoop();
}

inline void controllerLoop()
{
  float u;
  int pwm;
  float v_adc, total_adc;
  float time;
  if (timer_fired)
  {
    time = millis();
    timer_fired = false;
    read_adc = digital_filter(20.0);
    if (my_desk.isON() && (!my_desk.isIgnoreReference()))
    {
      // Feedforward
      my_pid.compute_feedforward(my_desk.getRefVolt());

      // Feedback
      if (my_pid.get_feedback())
      {
        v_adc = adc_to_volt(read_adc);                           // Volt na entrada
        u = my_pid.compute_control(my_desk.getRefVolt(), v_adc); // Volt
        my_pid.housekeep(my_desk.getRefVolt(), v_adc);
      }
      else
      {
        u = my_pid.get_u();
      }
      pwm = u * 4095;
      analogWrite(LED_PIN, pwm);
      my_desk.setDutyCycle(pwm / dutyCycle_conv);
    }
    float lux = adc_to_lux(read_adc);
    my_desk.Compute_avg(my_pid.get_h(), lux, my_desk.getRef());
    my_desk.store_buffer(lux);
    read_command();
    real_time_stream_of_data(time / 1000, lux);
  }
}

inline void communicationLoop()
{
  static unsigned long timer_new_node = 0;
  long time_now = millis();
  if (!comm.isConnected())
  {
    if (time_now - comm.getConnectTime() > comm.getTimeToConnect())
    {
      comm.setConnected(true);
      my_desk.setDeskNumber(find_desk());
      comm.connection_msg('A');
      flag_temp = true; // TODO CONECTAR PARA QUALQUER NUM DE DESKS
      // TODO Confirmar mensagens de W N antes de correr o new_calibration
    }
    else
    {
      if (time_now - timer_new_node > 200) // Send a new connection message every 200ms
      {
        comm.connection_msg('N');
        timer_new_node = millis();
      }
    }
  }
  if (data_available)
  {
    data_available = false;
    if (!missing_ack.empty()) // Check if something has been received
    {
      while (can0.checkReceive() && !missing_ack.empty())
      {
        can_frame canMsgRx;
        can0.readMessage(&canMsgRx);

        if (canMsgRx.data[0] != 'A')
        {
          command_queue.push(canMsgRx);
        }
        else
        {
          confirm_msg(canMsgRx);
          if (missing_ack.empty())
          {
            switch (last_msg_sent.data[0]) // Trigger of some actions after receiving the ack
            {
            case 'C':
              switch (last_msg_sent.data[1])
              {
              case 'B':
              {
                light_off = adc_to_lux(digital_filter(50.0));
                calibration_msg(0, 'E');
              }
              break;
              case 'E':
              {
                calibration_msg(1, 'S');
              }
              break;
              case 'R':
              {
                if (desk != desks_connected.size() + 1)
                {
                  analogWrite(LED_PIN, 0);
                  calibration_msg(desk + 1, 'S');
                }
                else
                {
                  is_calibrated = true;
                  calibration_msg(0, 'F');
                  analogWrite(LED_PIN, 0);
                }
              }
              break;
              default:
                break;
              }
            default:
              break;
            }
          }
        }
      }
    }
    else
    {
      while (can0.checkReceive())
      {
        can_frame canMsgRx;
        can0.readMessage(&canMsgRx);
        if (canMsgRx.data[2] == int_to_char_msg(desk) || canMsgRx.data[2] == int_to_char_msg(0))
        {
          command_queue.push(canMsgRx);
        }
      }
      while (!command_queue.empty())
      {
        can_frame canMsgRx;
        canMsgRx = command_queue.front();
        switch (canMsgRx.data[0])
        {
        case 'W':
          msg_received_connection(canMsgRx);
          break;
        case 'C':
          msg_received_calibration(canMsgRx);
          break;
        case 'S':
          msg_received_consensus(canMsgRx);
          break;
        default:
          break;
        }
        command_queue.pop();
      }
    }
  }

  time_now = millis();
  // RESEND LAST MESSAGE IF NO ACK RECEIVED
  if ((time_now - time_ack) > TIME_ACK && !missing_ack.empty())
  {

    Serial.printf("Se %lu %lu\n", time_now, time_ack);
    resend_last_msg();
    time_ack = millis();
  }
  if (flag_temp && (desks_connected.size() + 1) == 3)
  {
    if (desk == 3)
    {
      new_calibration();
    }
    flag_temp = false;
  }
  if (Serial.available() > 0) // TO DELETE
  {
    char c = Serial.read();
    if (is_connected)
    {
      Serial.printf("Desk Number: %d\n", desk);
      Serial.printf("Conectadas a mim:");
      for (const int &elem : desks_connected)
      {
        Serial.printf(" %d,", elem);
      }
      Serial.printf("\nTime to connect -> %d\n", time_to_connect);
      if (is_calibrated)
      {
        for (int i = 1; i <= desks_connected.size() + 1; i++)
        {
          Serial.printf("Desk %d -> %f\n", i, desk_array[i - 1]);
        }
      }
    }
    else
    {
      Serial.printf("Not connected yet\n");
    }
  }
}

// Conversões
float adc_to_volt(int read_adc)
{
  return read_adc / adc_conv;
}

int volt_to_adc(float input_volt)
{
  return input_volt * adc_conv;
}

float adc_to_lux(int read_adc)
{
  float LDR_volt;
  LDR_volt = read_adc / adc_conv;
  return volt_to_lux(LDR_volt);
}

float volt_to_lux(float volt)
{
  float LDR_resistance = (VCC * 10000.0) / volt - 10000.0;
  return pow(10, (log10(LDR_resistance) - my_desk.getOffset_R_Lux()) / (my_desk.getM()));
}

// Funções extras
void Gain()
{
  Serial.println("Calibrating the gain of the system:");
  analogWrite(LED_PIN, 0);
  delay(2500);
  float y1 = adc_to_lux(digital_filter(50.0));
  analogWrite(LED_PIN, 4095);
  delay(2500);
  float y2 = adc_to_lux(digital_filter(50.0));
  analogWrite(LED_PIN, 0);
  delay(2500);
  float Gain = (y2 - y1);
  my_desk.setGain(Gain);
  my_pid.set_b(my_desk.getRefVolt() / my_desk.getRef(), Gain);
  Serial.printf("The static gain of the system is %f [LUX/DC]\n", Gain);
}

// Média de medições, para reduzir noise;
float digital_filter(float value)
{
  float total_adc;
  int j;
  for (j = 0, total_adc = 0; j < value; j += 1)
  {
    total_adc += analogRead(A0);
  }
  return total_adc / value;
}

void ref_change(float value)
{
  my_desk.setRef(value);
  my_desk.setIgnoreReference(false);
  my_pid.set_b(my_desk.getRefVolt() / my_desk.getRef(), my_desk.getGain());
  my_desk.setON(true);
  my_pid.set_Ti(Tau(my_desk.getRef()));
}

float Tau(float value)
{
  if (value >= 0.5)
  {
    float R1 = 10e3;
    float R2 = pow(10, (my_desk.getM() * log10(value) + my_desk.getOffset_R_Lux()));
    float Req = (R2 * R1) / (R2 + R1);
    return Req * 10e-6;
  }
  else
  {
    return 0.1;
  }
}

int runConsensus()
{
  if (numberOfDesks == 3)
  {
    // NODE INITIALIZATION
    node.initializeNode(K[luminaire.getDeskNumber() - 1], luminaire.getDeskNumber() - 1);

    // RUN CONSENSUS ALGORITHM
    consensusRunning = true;
    consensusIteration = 0;
    consensusStage = consensusStage::CONSENSUSITERATON;
    otherD[2][3] = {0};
  }
}

void ConsensusLoop()
{
  if (consensusRunning)
  {
    switch (consensusStage)
    {
    case consensusStage::CONSENSUSITERATON:
    {
      // COMPUTATION OF THE PRIMAL SOLUTIONS
      node.consensusIterate();

      send_msg(node.getD()); // Send the d values to the neighbors

      consensusStage = consensusStage::CONSENSUSWAIT;
      otherD[2][3] = {0};
      break;
    }
    case consensusStage::CONSENSUSWAIT:
    {
      if (true) // received all neighbors d values
      {
        // COMPUTATION OF THE AVERAGE
        double temp;
        for (int j = 0; j < 3; j++)
        {
          temp = (node.getDIndex(j) + otherD[0][j] + otherD[1][j]) / 3;
          node.setDavIndex(j, temp);
        }

        // COMPUTATION OF THE LAGRANGIAN UPDATES
        for (int j = 0; j < 3; j++)
        {
          node.setLambdaIndex(j, node.getLambdaIndex(j) + node.getRho() * (node.getDIndex(j) - node.getDavIndex(j)));
        }

        // Check if a consensus has been reached, if so, break the loop
        if (node.checkConvergence() || consensusIteration >= maxiter)
        {
          consensusRunning = false;
          // UPDATE DUTY CYCLE CONTROL INPUT
        }
        else // If not, update the iteration counter and go back to the iteration stage
        {
          consensusIteration++;
          consensusStage = consensusStage::CONSENSUSITERATON;
        }

        // Update the last d values
        node.copyArray(node.getLastD(), node.getD());
      }

      break;
    }
    }
  }
}

void consensus_msg(double d[3])
{
  struct can_frame canMsgTx;
  canMsgTx.can_id = desk;
  canMsgTx.can_dlc = 8;
  canMsgTx.data[0] = 'S';
  canMsgTx.data[1] = 'D';
  memcpy(canMsgTx.data + (2 * sizeof(char)), &d[0], sizeof(d[0]));
  memcpy(canMsgTx.data + (2 * sizeof(char) + sizeof(d[0])), &d[1], sizeof(d[1]));
  memcpy(canMsgTx.data + (2 * sizeof(char) + 2 * sizeof(d[0])), &d[2], sizeof(d[2]));
  err = can0.sendMessage(&canMsgTx);
  if (err != MCP2515::ERROR_OK)
  {
    Serial.printf("Error sending message: %s\n", err);
  }
}