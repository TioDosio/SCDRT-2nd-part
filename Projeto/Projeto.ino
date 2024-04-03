#include "pid.h"
#include "command.h"
#include "luminaire.h"
#include "consensus.h"
#include "Communication.h"
#include "extrafunctions.h"

#define TIME_ACK 3500

// luminaire
const int LED_PIN = 15;     // led pin
const int DAC_RANGE = 4096; // range of the DAC
const float VCC = 3.3;
const float adc_conv = 4095.0 / VCC;
const float dutyCycle_conv = 4095.0 / 100.0;
pid my_pid{0.01, 0.3, 0.1}; // h, k, Tt

Node node;

luminaire my_desk{-0.9, log10(225000) - (-0.9), 0.0158, node.getCurrentLowerBound()}; // m, b(offset), Pmax, InitialRef

bool debbuging = false;
int counter = 0;
float array_lux[3];
float array_dc[3];

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
  if (comm.getIsCalibrated())
  {
    if (timer_fired)
    {
      timer_fired = false;
      float time;
      time = millis();
      float read_adc = digital_filter(20.0);
      if (!my_desk.isIgnoreReference())
      {
        controllerLoop(read_adc);
      }
      float lux = adc_to_lux(read_adc);
      my_desk.Compute_avg(my_pid.get_h(), lux, my_desk.getRef(), my_desk.getDeskNumber());
      if (Serial.available() > 0)
      {
        String command = Serial.readStringUntil('\n');
        read_command(command, 0);
      }

      if (!my_desk.getHub())
      {
        array_lux[counter] = lux;
        array_dc[counter] = my_desk.getDutyCycle();
        if (counter == 2)
        {
          // send_arrays_buff(array_lux, 0);
          // send_arrays_buff(array_dc, 1);
          counter = -1;
        }
        counter++;
      }

      my_desk.store_buffer_l(my_desk.getDeskNumber(), lux);
      real_time_stream_of_data(time / 1000, lux);
    }
  }
}

void loop1()
{
  wakeUp();
  communicationLoop();
  resendAck();
  start_calibration();
}

inline void controllerLoop(float read_adc)
{
  float u;
  int pwm;
  float v_adc;
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

void mainConsensus(double newLuminance[3])
{
  int numberOfDesks = comm.getNumDesks();

  if (numberOfDesks > 1 && !node.getConsensusRunning())
  {
    int myDesk = my_desk.getDeskNumber();
    // nodes
    Node nodes[numberOfDesks];

    for (int i = 0; i < numberOfDesks; i++)
    {
      if (i == myDesk)
      {
        nodes[i] = node;
      }
      else
      {
        int index = std::distance(comm.getDesksConnected().begin(), comm.getDesksConnected().find(i));
        OtherLuminaires Lums = node.getLums(index);
        nodes[i].initializeNode(Lums.getK(), i, Lums.getC(), Lums.getL(), Lums.getO());
      }
    }

    // RUN CONSENSUS ALGORITHM
    double *d_av = runConsensus(nodes, numberOfDesks);

    for (int i = 0; i < numberOfDesks; i++)
    {
      newLuminance[i] = nodes[i].getKIndex(0) * nodes[i].getDavIndex(0) + nodes[i].getKIndex(1) * nodes[i].getDavIndex(1) + nodes[i].getKIndex(2) * nodes[i].getDavIndex(2) + nodes[i].getO();
    }
    ref_change(newLuminance[myDesk]);
    node = nodes[myDesk];
  }
  else
  {
    Serial.println("Error: Not all nodes are connected or the luminaire is off or consensus is already running.\n");
  }
}

double *runConsensus(Node *nodes, int numberOfDesks)
{
  // iterations
  for (int i = 1; i < nodes[0].getConsensusMaxIterations(); i++)
  {
    // COMPUTATION OF THE PRIMAL SOLUTIONS
    for (int j = 0; j < numberOfDesks; j++)
    {
      nodes[j].consensusIterate();
    }

    // COMPUTATION OF THE AVERAGE
    for (int j = 0; j < numberOfDesks; j++)
    {
      nodes[j].setDavIndex(j, (nodes[nodes[j].getIndex()].getDIndex(j) + nodes[nodes[j].getIndex()].getDIndex(j) + nodes[nodes[j].getIndex()].getDIndex(j)) / numberOfDesks);
    }

    // COMPUTATION OF THE LAGRANGIAN UPDATES
    for (int j = 0; j < numberOfDesks; j++)
    {
      nodes[j].setLambdaIndex(j, nodes[j].getLambdaIndex(j) + nodes[j].getRho() * (nodes[j].getDIndex(j) - nodes[j].getDavIndex(j)));
      nodes[j].setLambdaIndex(j, nodes[j].getLambdaIndex(j) + nodes[j].getRho() * (nodes[j].getDIndex(j) - nodes[j].getDavIndex(j)));
      nodes[j].setLambdaIndex(j, nodes[j].getLambdaIndex(j) + nodes[j].getRho() * (nodes[j].getDIndex(j) - nodes[j].getDavIndex(j)));
    }

    // Check if a consensus has been reached, if so, break the loop
    int count = 0;
    for (int j = 0; j < numberOfDesks; j++)
    {
      if (nodes[j].checkConvergence())
      {
        count++;
      }
      // Update the last d values
      nodes[j].copyArray(nodes[j].getLastD(), nodes[j].getD());
      nodes[j].copyArray(nodes[j].getLastD(), nodes[j].getD());
      nodes[j].copyArray(nodes[j].getLastD(), nodes[j].getD());
    }
    if (count == numberOfDesks)
    {
      break;
    }
  }
  return nodes[0].getDav();
}
