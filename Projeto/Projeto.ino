#include "pid.h"
#include "command.h"
#include "luminaire.h"
#include "consensus.h"
#include "Communication.h"
#include "extrafunctions.h"

#define TIME_ACK 2500

// luminaire
const int LED_PIN = 15;     // led pin
const int DAC_RANGE = 4096; // range of the DAC
const float VCC = 3.3;
const float adc_conv = 4095.0 / VCC;
const float dutyCycle_conv = 4095.0 / 100.0;
pid my_pid{0.01, 0.3, 0.1}; // h, k, Tt

Node node;

uint8_t this_pico_flash_id[8], node_address; // o set K e o set B, têm de ser definidos no node  address

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

bool flag_temp = false;

enum consensusStage : uint8_t
{
  CONSENSUSITERATON,
  CONSENSUSWAIT
};

consensusStage consensusStage;
/*
 * Setup of the first core
 */
void setup()
{ // the setup function runs once
  Serial.begin(115200);
  analogReadResolution(12);    // default is 10
  analogWriteFreq(60000);      // 60KHz, about max
  analogWriteRange(DAC_RANGE); // 100% duty cycle
  add_repeating_timer_ms(-10, my_repeating_timer_callback, NULL, &timer);
}

/*
 * Setup of the second core
 */
void setup1()
{
  rp2040.idleOtherCore();
  flash_get_unique_id(this_pico_flash_id);
  node_address = this_pico_flash_id[6];
  rp2040.resumeOtherCore();
  comm.resetCan0();
  comm.setCan0Bitrate();
  comm.setCan0NormalMode();
  gpio_set_irq_enabled_with_callback(interruptPin, GPIO_IRQ_EDGE_FALL, true, &read_interrupt);
  randomSeed(node_address);
  comm.add2TimeToConnect(random(13) * 300); // To ensure that the nodes don't connect at the same time
  comm.setConnectTime(millis());
}

/*
 * 1st core loop
 */
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
        consensusLoop();
        controllerLoop(read_adc);
      }
      float lux = adc_to_lux(read_adc);
      my_desk.Compute_avg(my_pid.get_h(), lux, my_desk.getRef(), my_desk.getDeskNumber());
      if (Serial.available() > 0)
      {
        String command = Serial.readStringUntil('\n');
        read_command(command, 0);
      }
      my_desk.store_buffer_l(my_desk.getDeskNumber(), lux);
      my_desk.store_buffer_d(my_desk.getDeskNumber(), my_desk.getDutyCycle());
      real_time_stream_of_data(time / 1000, lux);
    }
  }
}

/*
 * 2nd core loop
 */
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

void runConsensus()
{
  if (comm.getNumDesks() == 3 && !node.getConsensusRunning())
  {
    // NODE INITIALIZATION
    node.initializeNode(comm.getCouplingGains(), my_desk.getDeskNumber() - 1, comm.getExternalLight());
    // RUN CONSENSUS ALGORITHM
    node.setConsensusRunning(true);
    node.setConsensusIterations(0);
    consensusStage = consensusStage::CONSENSUSITERATON;
  }
  else
  {
    Serial.println("Error: Not all nodes are connected or the luminaire is off or consensus is already running.\n");
  }
}

void consensusLoop()
{
  if (node.getConsensusRunning())
  {
    switch (consensusStage)
    {
    case consensusStage::CONSENSUSITERATON:
    {
      // COMPUTATION OF THE PRIMAL SOLUTIONS
      node.consensusIterate();
      node.setConsensusReady(true);

      consensusStage = consensusStage::CONSENSUSWAIT;
      node.resetOtherD(); // Reset the other d values
      break;
    }
    case consensusStage::CONSENSUSWAIT:
    {
      if (node.checkOtherDIsFull() && !node.getConsensusReady())
      {
        // COMPUTATION OF THE AVERAGE
        double temp;
        double *dutycycle1 = node.getOtherD(0);
        double *dutycycle2 = node.getOtherD(1);
        for (int j = 0; j < 3; j++)
        {
          temp = (node.getDIndex(j) + dutycycle1[j] + dutycycle2[j]) / 3;
          node.setDavIndex(j, temp);
        }
        // COMPUTATION OF THE LAGRANGIAN UPDATES
        for (int j = 0; j < 3; j++)
        {
          node.setLambdaIndex(j, node.getLambdaIndex(j) + node.getRho() * (node.getDIndex(j) - node.getDavIndex(j)));
        }

        consensusStage = consensusStage::CONSENSUSITERATON;
        // Update the last d values
        node.copyArray(node.getLastD(), node.getD());
      }
      break;
    }
    }
  }
}
