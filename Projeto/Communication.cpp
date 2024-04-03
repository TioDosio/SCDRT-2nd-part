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
void communication::acknowledge_loop(Node *node)
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
      Serial.printf("Ack received from %d -> %c %c %c %c\n", canMsgRx.can_id, canMsgRx.data[1], canMsgRx.data[2], canMsgRx.data[3], canMsgRx.data[4]);
      confirm_msg(canMsgRx);
      if (isMissingAckEmpty())
      {
        Serial.printf("All nodes acked\n");
        msg_received_ack(last_msg_sent, node);
      }
    }
  }
}

/*
 * Function to check the acks and trigger some actions
 * @param canMsgRx - can_frame object
 * @param node - Node object
 */
void communication::msg_received_ack(can_frame canMsgRx, Node *node)
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
        node->initializeNode(getCouplingGains(), my_desk->getDeskNumber(), 1, 5, getExternalLight());
      }
    }
    break;
    default:
      break;
    }
  }
  break;
  case '1':
  {
    Serial.printf("ACK response '1'\n");
    send_consensus_data('2', node, canMsgRx.can_id);
  }

  case '2':
  {
    int id = *(std::next(desks_connected.begin()));
    Serial.printf("ESte é o do 2 %d\n", id);
    send_consensus_data('1', node, id);
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
void communication::msg_received_calibration(can_frame canMsgRx, Node *node)
{
  switch (canMsgRx.data[1])
  {
  case 'B': // Quando lerem o begin desligam as luzes
  {
    // TODO : LIMPAR TODOS OS OUTRAS MENSAGENS DE CALIBRATIONS
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
    node->initializeNode(getCouplingGains(), my_desk->getDeskNumber(), 1, 5, getExternalLight());
  }
  break;
  case 'R':
  {
    light_on = adc_to_lux(digital_filter(50.0));
    ack_msg(canMsgRx);
    coupling_gains[char_msg_to_int(canMsgRx.data[2]) - 1] = light_on - light_off; // change
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
    // Serial.printf("ERROR DURING CALIBRATION. Message C %c received.\n", canMsgRx.data[1]);
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
    // Serial.printf("ERROR DURING WAKE UP. Message W %c received.\n", canMsgRx.data[1]);
    // TODO Substituir o serial
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
    Serial.printf("Calibration Started\n");
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

/*
 * Function to send the calibration data to the other desks
 * @param lux: array of lux values
 */
void communication::consensus_msg_lux(double lux[3])
{
  struct can_frame canMsgTx;
  canMsgTx.can_id = 0;
  canMsgTx.can_dlc = 8;
  canMsgTx.data[0] = 'E';
  int msg;
  for (int i = 1, j = 0; i < 7; i += 2, j++)
  {
    msg = static_cast<int>(lux[j] * 100);
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
      Serial.printf("Not the same message\n");
      return;
    }
  }
  Serial.printf("SAME MESSAGE\n");
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
    // TODO : CHANGE THE HUB IF THE ONE DISCONNECTED WAS THE HUB
    last_msg_counter = 0;
    for (const int &element : missing_ack)
    {
      desks_connected.erase(element);
      Serial.printf("Node %d removed from the connected nodes do to inactivity\n", element);
    }
  }
  err = can0.sendMessage(&last_msg_sent);
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

/*
 * Start consensus algorithm
 */
void communication::start_consensus()
{
  struct can_frame canMsgTx;
  canMsgTx.can_id = *(desks_connected.begin());
  Serial.printf("Sending T message to %d\n", canMsgTx.can_id);
  canMsgTx.can_dlc = 8;
  canMsgTx.data[0] = 'T';
  canMsgTx.data[1] = int_to_char_msg(my_desk->getDeskNumber());
  for (int i = 2; i < 8; i++)
  {
    canMsgTx.data[i] = ' ';
  }
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

/*
 * Function to send the consensus data
 * @param part: char to know if it is the first or second part of the consensus
 * @param node: Node object
 * @param destination: int to know the destination of the message
 */
void communication::send_consensus_data(char part, Node *node, int destination)
{
  struct can_frame canMsgTx;
  canMsgTx.can_id = destination;
  canMsgTx.can_dlc = 8;
  canMsgTx.data[0] = part;
  canMsgTx.data[1] = int_to_char_msg(my_desk->getDeskNumber());
  if (part == '1')
  {
    for (int i = 2, j = 0; i < 8; i += 2, j++)
    {
      int msg = static_cast<int>(node->getKIndex(j) * 1000);
      canMsgTx.data[i] = static_cast<unsigned char>(msg & 255);    // Same as msg0 % 256, but more efficient
      canMsgTx.data[i + 1] = static_cast<unsigned char>(msg >> 8); // Same as msg0 / 256, but more efficient
    }
    err = can0.sendMessage(&canMsgTx);
    if (err != MCP2515::ERROR_OK)
    {
      Serial.printf("Error sending message: %s\n", err);
    }
  }
  else
  {
    int msg = static_cast<int>(node->getO() * 1000);
    canMsgTx.data[2] = static_cast<unsigned char>(msg & 255); // Same as msg0 % 256, but more efficient
    canMsgTx.data[3] = static_cast<unsigned char>(msg >> 8);  // Same as msg0 / 256, but more efficient
    msg = static_cast<int>(node->getCost() * 100);
    canMsgTx.data[4] = static_cast<unsigned char>(msg & 255); // Same as msg0 % 256, but more efficient
    canMsgTx.data[5] = static_cast<unsigned char>(msg >> 8);  // Same as msg0 / 256, but more efficient
    msg = static_cast<int>(node->getCurrentLowerBound() * 100);
    canMsgTx.data[6] = static_cast<unsigned char>(msg & 255); // Same as msg0 % 256, but more efficient
    canMsgTx.data[7] = static_cast<unsigned char>(msg >> 8);  // Same as msg0 / 256, but more efficient
    err = can0.sendMessage(&canMsgTx);
    if (err != MCP2515::ERROR_OK)
    {
      Serial.printf("Error sending message \n");
    }
  }
  missing_ack.insert(destination);
  Serial.printf("Sent message %c to %d\n", part, destination);
  time_ack = millis();
  last_msg_sent = canMsgTx;
  last_msg_counter = 0;
}