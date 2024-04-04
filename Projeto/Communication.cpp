#include "Communication.h"
#include "extrafunctions.h"
#include <cstring>
#include <Arduino.h>

communication::communication(luminaire *_my_desk) : my_desk{_my_desk}, is_connected{false}, time_to_connect{500}, light_off{0.0}, light_on{0.0}, is_calibrated{false}, coupling_gains{NULL}
{
}

/*
 * Function to finds desks on the system
 */
int communication::find_desk()
{
  int i = 1;
  while (desks_connected.find(i) != desks_connected.end())
  {
    i++;
  }
  return i;
}

/*
 * Function to check acks and put the messages on the queue
 * @param node - Node object
 */
void communication::acknowledge_loop(Node *node, pid *my_pid)
{
  while (can0.checkReceive() && !isMissingAckEmpty())
  {
    can_frame canMsgRx;
    can0.readMessage(&canMsgRx);
    if (canMsgRx.data[0] != 'A')
    {
      if (canMsgRx.can_id == my_desk->getDeskNumber() || canMsgRx.can_id == 0) // Check if the message is for this desk (0 is for all the desks)
      {
        command_queue.push(canMsgRx);
      }
    }
    else
    {
      confirm_msg(canMsgRx);
      if (isMissingAckEmpty())
      {
        msg_received_ack(last_msg_sent, node, my_pid);
      }
    }
  }
}

/*
 * Function to check the acks and trigger some actions
 * @param canMsgRx - can_frame object
 * @param node - Node object
 */
void communication::msg_received_ack(can_frame canMsgRx, Node *node, pid *my_pid)
{
  switch (canMsgRx.data[0])
  {
  case 'C':
  {
    switch (canMsgRx.data[1])
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
      if (my_desk->getDeskNumber() != getNumDesks())
      {
        ChangeLEDValue(0);
        calibration_msg(my_desk->getDeskNumber() + 1, 'S');
      }
      else
      {
        is_calibrated = true;
        calibration_msg(0, 'F');
        ChangeLEDValue(0);
        my_pid->set_b(my_desk->lux_to_volt(my_desk->getRef()) / my_desk->getRef(), my_desk->getGain());
      }
    }
    break;
    case 'r':
    {
      reset_values(node);
    }
    default:
      break;
    }
  }
  break;
  case 'Q':
  {
    int next_desk = char_msg_to_int(canMsgRx.data[7]) + 1 > getNumDesks() ? 1 : char_msg_to_int(canMsgRx.data[7]) + 1;
    node->setConsensusIterations(node->getConsensusIterations() + 1);
    if (node->getConsensusIterations() == node->getConsensusMaxIterations() || (node->checkConvergence() && node->checkOtherDIsFull()))
    {
      double l = node->getKIndex(0) * node->getDavIndex(0) + node->getKIndex(1) * node->getDavIndex(1) + node->getKIndex(2) * node->getDavIndex(2) + node->getO();

      ref_change(l);
      node->setConsensusRunning(false); // Stop the consensus when the max iterations are reached
      consensus_msg_switch(0, 'E');
    }
    else
    {
      consensus_msg_switch(next_desk, 'T');
    }
  }
  break;
  default:
    break;
  }
}

/************General Messages********************/

/*
 * Function to send acks
 * @param orig_msg - can_frame object
 */
void communication::ack_msg(can_frame orig_msg)
{
  struct can_frame canMsgTx;
  canMsgTx.can_id = my_desk->getDeskNumber();
  canMsgTx.can_dlc = 8;
  canMsgTx.data[0] = 'A';
  for (int i = 0; i < 6; i++)
  {
    canMsgTx.data[i + 1] = orig_msg.data[i];
  }
  err = can0.sendMessage(&canMsgTx);

  if (err != MCP2515::ERROR_OK)
  {
    Serial.printf("Error sending message: %s\n", err);
  }
}

/************CONNECTION FUNCTIONS********************/

/*
 * Function to send connection messages Message -> "W A/N/R {desk_number}" (Wake Ack/New/Received)
 * @param type - char
 */
void communication::connection_msg(char type)
{
  struct can_frame canMsgTx;
  canMsgTx.can_id = 0;
  canMsgTx.can_dlc = 8;
  canMsgTx.data[0] = 'W';
  canMsgTx.data[1] = type;
  canMsgTx.data[2] = int_to_char_msg(my_desk->getDeskNumber()); // TO direct the message to everyone
  for (int i = 3; i < 8; i++)
  {
    canMsgTx.data[i] = ' ';
  }
  err = can0.sendMessage(&canMsgTx);
  if (err != MCP2515::ERROR_OK)
  {
    Serial.printf("Error sending message: %s\n", err);
  }
}

/************CALIBRATION FUNCTIONS********************/

/*
 * Function to send calibration messages; Message -> "C B/E/F/R/S {desk_number}" (Calibration Beginning/External/Finished/Read/Start)
 * @param dest_desk - int
 * @param type - char
 */
void communication::calibration_msg(int dest_desk, char type)
{
  struct can_frame canMsgTx;
  canMsgTx.can_id = dest_desk;
  canMsgTx.can_dlc = 8;
  canMsgTx.data[0] = 'C';
  canMsgTx.data[1] = type;
  canMsgTx.data[2] = int_to_char_msg(my_desk->getDeskNumber()); // DESK TO WHICH THE MESSAGE IS DIRECTED, in messages that require it
  for (int i = 3; i < 8; i++)
  {
    canMsgTx.data[i] = ' ';
  }
  err = can0.sendMessage(&canMsgTx);
  if (err != MCP2515::ERROR_OK)
  {
    Serial.printf("Error sending message: %s\n", err);
  }
  if (type != 'F' && type != 'S')
  {
    missing_ack = desks_connected;
    time_ack = millis();
    last_msg_sent = canMsgTx;
    last_msg_counter = 0;
  }
}

/*
 * Function to receive calibration messages
 * @param canMsgRx - can_frame object
 */
void communication::msg_received_calibration(can_frame canMsgRx, Node *node, pid *my_pid)
{
  switch (canMsgRx.data[1])
  {
  case 'B': // Quando lerem o begin desligam as luzes
  {
    if (coupling_gains != NULL)
    {
      free(coupling_gains);
    }
    coupling_gains = (double *)malloc((getNumDesks()) * sizeof(double)); // array of desks
    ack_msg(canMsgRx);
    ChangeLEDValue(0);
  }
  break;
  case 'E':
  {
    light_off = adc_to_lux(digital_filter(50.0));
    ack_msg(canMsgRx);
  }
  break;
  case 'F':
  {
    is_calibrated = true;
    Serial.printf("Calibration Finished through message\n");
    my_pid->set_b(my_desk->lux_to_volt(my_desk->getRef()) / my_desk->getRef(), my_desk->getGain());
  }
  break;
  case 'R':
  {
    light_on = adc_to_lux(digital_filter(50.0));
    ack_msg(canMsgRx);
    coupling_gains[char_msg_to_int(canMsgRx.data[2]) - 1] = light_on - light_off; // change
  }
  break;
  case 'r':
  {
    ack_msg(canMsgRx);
    reset_values(node);
  }
  break;
  case 'S':
  {
    if (canMsgRx.can_id == my_desk->getDeskNumber())
    {
      cross_gains();
    }
    else
    {
      ChangeLEDValue(0);
    }
  }
  break;
  default:
    break;
  }
}

/*
 * Function to lights on the LED and measure the light to calculate the gain and send it to the other desks
 */
void communication::cross_gains()
{
  ChangeLEDValue(4095);
  delay_manual(3000);
  calibration_msg(0, 'R');
  light_on = adc_to_lux(digital_filter(50.0));
  coupling_gains[my_desk->getDeskNumber() - 1] = light_on - light_off;
  my_desk->setGain(light_on - light_off);
}

/*
 * Function to receive consensus messages and know the connected desks
 */
void communication::msg_received_connection(can_frame canMsgRx)
{

  switch (canMsgRx.data[1])
  {
  case 'N':
  {
    if (is_connected)
    {
      connection_msg('R');
    }
  }
  break;
  case 'R':
  {
    if (!is_connected)
    {
      desks_connected.insert(char_msg_to_int(canMsgRx.data[2]));
    }
  }
  break;
  case 'A':
  {
    if (is_connected)
    {
      desks_connected.insert(char_msg_to_int(canMsgRx.data[2]));
    }
  }
  break;
  default:
    break;
  }
}

/*
 * Function to start a new calibration
 */
void communication::new_calibration()
{
  coupling_gains = (double *)malloc((desks_connected.size() + 1) * sizeof(double)); // array of desks
  if (desks_connected.empty())
  {
    Gain();
    is_calibrated = true;
  }
  else
  {
    calibration_msg(0, 'B');
    ChangeLEDValue(0);
  }
}

/*
 * Function to calculate the gain
 */
void communication::Gain()
{
  ChangeLEDValue(0);
  delay_manual(2500);
  light_off = adc_to_lux(digital_filter(50.0));
  ChangeLEDValue(4095);
  delay_manual(2500);
  light_on = adc_to_lux(digital_filter(50.0));
  ChangeLEDValue(0);
  delay_manual(2500);
  coupling_gains[0] = (light_on - light_off);
}

// CONSENSUS
void communication::msg_received_consensus(can_frame canMsgRx, Node *node)
{
  double d[3];
  int index = std::distance(desks_connected.begin(), desks_connected.find(char_msg_to_int(canMsgRx.data[7])));
  for (int i = 0; i < 3; i++)
  {
    d[i] = (static_cast<int>(canMsgRx.data[2 * i + 1]) + (static_cast<int>(canMsgRx.data[2 * i + 2]) << 8)) / 10000.0;
  }
  node->setOtherD(index, d);
  //  Send ack
  ack_msg(canMsgRx);
}

// Message -> "C E/T {desk_number}" (Consensus End/Transmission)
void communication::consensus_msg_switch(int dest_desk, char type)
{
  struct can_frame canMsgTx;
  canMsgTx.can_id = dest_desk;
  canMsgTx.can_dlc = 8;
  canMsgTx.data[0] = type;
  canMsgTx.data[1] = ' ';
  canMsgTx.data[2] = int_to_char_msg(my_desk->getDeskNumber());
  int msg;
  for (int i = 3; i < 8; i++)
  {
    canMsgTx.data[i] = ' ';
  }
  err = can0.sendMessage(&canMsgTx);
}

void communication::consensus_msg_duty(double d[3])
{
  struct can_frame canMsgTx;
  canMsgTx.can_id = 0;
  canMsgTx.can_dlc = 8;
  canMsgTx.data[0] = 'Q';
  int msg;
  for (int i = 1, j = 0; i < 7; i += 2, j++)
  {
    msg = d[j] > 0 ? static_cast<int>(d[j] * 10000) : 0;
    canMsgTx.data[i] = static_cast<unsigned char>(msg & 255);    // Same as msg0 % 256, but more efficient
    canMsgTx.data[i + 1] = static_cast<unsigned char>(msg >> 8); // Same as msg0 / 256, but more efficient
  }
  canMsgTx.data[7] = int_to_char_msg(my_desk->getDeskNumber());
  err = can0.sendMessage(&canMsgTx);
  if (err != MCP2515::ERROR_OK)
  {
    Serial.printf("Error sending message: %s\n", err);
  }
  missing_ack = desks_connected;
  time_ack = millis();
  last_msg_sent = canMsgTx;
  last_msg_counter = 0;
}

//------------------------UTILS------------------------

/*
 * Function to check if the missing ack is empty
 * @param node - Node object
 */
void communication::confirm_msg(can_frame ack_msg)
{

  for (int i = 0; i < 6; i++)
  {
    if (ack_msg.data[i + 1] != last_msg_sent.data[i])
    {
      return;
    }
  }
  missing_ack.erase(ack_msg.can_id);
  return;
}

/*
 * Function to convert a char message to an int
 * @param msg - char to cinvert
 */
int communication::char_msg_to_int(char msg)
{
  return msg - '0';
}

/*
 * Function to convert an int to a char message
 * @param msg - int to convert
 */
char communication::int_to_char_msg(int msg)
{
  return msg + '0';
}

/*
 * Function to resend the last message
 */
void communication::resend_last_msg()
{
  last_msg_counter++;
  if (last_msg_counter == 5) // After 5 tries, remove the nodes that didn't ack
  {
    last_msg_counter = 0;
    for (const int &element : missing_ack)
    {
      desks_connected.erase(element);
      Serial.printf("Node %d removed from the connected nodes do to inactivity\n", element);
    }
    missing_ack.clear();
  }
  struct can_frame canMsgTx;
  canMsgTx.can_id = last_msg_sent.can_id;
  canMsgTx.can_dlc = 8;
  for (int i = 0; i < 8; i++)
  {
    canMsgTx.data[i] = last_msg_sent.data[i];
  }
  err = can0.sendMessage(&canMsgTx);
  if (err != MCP2515::ERROR_OK)
  {
    Serial.printf("Error sending message: %s\n", err);
  }
}

/*
 * Function to assyncronously delay
 * @param delay: value in milliseconds to do a delay
 */
void communication::delay_manual(unsigned long delay)
{
  unsigned long delay_start = millis();
  unsigned long delay_end = millis();
  while (delay_end - delay_start < delay)
  {
    delay_end = millis();
  }
}

void communication::reset_values(Node *node)
{
  setConnected(false);
  setIsCalibrated(false);
  ref_change(5);
  my_desk->setLuxFlag(0);
  my_desk->setDutyFlag(0);
  my_desk->setIgnoreReference(false);
  my_desk->setBufferFullL(false);
  my_desk->setBufferFullD(false);
  my_desk->setIdxBuffer_l(0);
  my_desk->setIdxBuffer_d(0);
  my_desk->resetMetrics();
  my_desk->setHub(false);
  node->setLowerBoundUnoccupied(5);
  node->setLowerBoundOccupied(20);
  node->setOccupancy(0);
  node->setConsensusRunning(false);
  ChangeLEDValue(0);
  desks_connected.clear();
  missing_ack.clear();
  while (!command_queue.empty())
  {
    command_queue.pop();
  }
  add2TimeToConnect(millis());
}