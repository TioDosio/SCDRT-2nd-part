#include "pid.h"
#include "command.h"
#include "luminaire.h"
#include "consensus.h"
#include "Communication.h"
#include "extrafunctions.h"

#define TIME_ACK 2500

// luminaire
const int LED_PIN = 15;
const int DAC_RANGE = 4096;
const float VCC = 3.3;
const float adc_conv = 4095.0 / VCC;
const float dutyCycle_conv = 4095.0 / 100.0;
pid my_pid{0.01, 0.15, 1.5}; // h, k, Tt
// (float _h, float _K, float Tt_,float b_,float Ti_, float Td_, float N_)
luminaire my_desk{-0.89, log10(225000) - (-0.89), 0.0158}; // m, b(offset), Pmax, desk_number
// system my_desk{float _m, float _offset_R_Lux, float _Pmax, unsigned short _desk_number}
float read_adc;
bool debbuging = false;

communication comm{&my_desk};

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

bool flag_temp = false; // TODO

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
  add_repeating_timer_ms(-10, my_repeating_timer_callback, NULL, &timer);
}

void setup1()
{
  comm.resetCan0();
  comm.setCan0Bitrate();
  comm.setCan0NormalMode();
  gpio_set_irq_enabled_with_callback(interruptPin, GPIO_IRQ_EDGE_FALL, true, &read_interrupt);

  // Connection timers
  int seed = 0;
  for (int i = 0; i < 5; i++)
  {
    seed += analogRead(A0);
  }
  randomSeed(seed);
  comm.add2TimeToConnect(random(51) * 75); // To ensure that the nodes don't connect at the same time
  comm.setConnectTime(millis());
}

void loop()
{ // the loop function runs cyclically
  read_command();
  if (comm.getIsCalibrated())
  {
    controllerLoop();
  }
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
      my_desk.setDeskNumber(comm.find_desk());
      comm.connection_msg('A');
      flag_temp = true; // TODO CONECTAR PARA QUALQUER NUM DE DESKS
      // TODO Confirmar mensagens de e N antes de correr o new_calibration
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
    if (!comm.isMissingAckEmpty()) // Check if there are any missing acks
    {
      comm.acknowledge_loop();
    }
    else
    {
      while (comm.IsMsgAvailable()) // Check if something has been received
      {
        can_frame canMsgRx;
        comm.ReadMsg(&canMsgRx);
        if (canMsgRx.data[2] == comm.int_to_char_msg(my_desk.getDeskNumber()) || canMsgRx.data[2] == comm.int_to_char_msg(0)) // Check if the message is for this desk (0 is for all the desks)
        {
          comm.add_msg_queue(canMsgRx); // Put all the messages in the queue
        }
      }

      while (!comm.isQueueEmpty())
      {
        can_frame canMsgRx;
        canMsgRx = comm.get_msg_queue();
        switch (canMsgRx.data[0])
        {
        case 'W':
          comm.msg_received_connection(canMsgRx);
          break;
        case 'C':
          comm.msg_received_calibration(canMsgRx);
          break;
        case 'S':
          comm.msg_received_consensus(canMsgRx);
          break;
        default:
          break;
        }
      }
    }
  }

  time_now = millis();
  // RESEND LAST MESSAGE IF NO ACK RECEIVED
  if ((time_now - comm.time_ack_get()) > TIME_ACK && !comm.isMissingAckEmpty())
  {
    comm.resend_last_msg();
    comm.time_ack_set(millis());
  }
  if (flag_temp && (comm.getNumDesks()) == 3) // Initialize the calibration when all the desks are connected
  {
    if (my_desk.getDeskNumber() == 3)
    {
      comm.new_calibration();
    }
    flag_temp = false;
  }
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

void runConsensus()
{
  if (comm.getNumDesks() == 3)
  {
    // NODE INITIALIZATION
    node.initializeNode(comm.getCouplingGains(), my_desk.getDeskNumber() - 1, comm.getExternalLight()); // TODO VER EXTERNAL LIGHT

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

      comm.consensus_msg(node.getD()); // Send the d values to the neighbors

      consensusStage = consensusStage::CONSENSUSWAIT;
      otherD[3][3] = {0};
      break;
    }
    case consensusStage::CONSENSUSWAIT:
    {
      if (true) // TODO received all neighbors d values with acks (TO DO)
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
          // Update the last d values
          node.copyArray(node.getLastD(), node.getD());
        }
      }
      break;
    }
    }
  }
}
