#include "Communication.h"
#include <cstring>
#include <Arduino.h>

communication::communication() : is_connected{false}, time_to_connect{500}, consensus_acknoledged{false}, light_off{0.0}, light_on{0.0}
{
}

int communication::find_desk()
{
  int i = 1;
  while (desks_connected.find(i) != desks_connected.end())
  {
    i++;
  }
  return i;
}

void communication::acknowledge_loop()
{
  while (can0.checkReceive() && !isMissingAckEmpty())
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
      if (isMissingAckEmpty())
      {
        msg_received_ack(last_msg_sent);
      }
    }
  }
}

// Trigger of some actions after receiving the ack
void communication::msg_received_ack(can_frame canMsgRx)
{
  switch (canMsgRx.data[0])
  {
  case 'C':
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
      if (desk != getNumDesks())
      {
        ChangeLEDValue(0);
        calibration_msg(desk + 1, 'S');
      }
      else
      {
        is_calibrated = true;
        calibration_msg(0, 'F');
        ChangeLEDValue(0);
      }
    }
    break;
    default:
      break;
    }
  case 'S':
  {
    consensus_acknoledged = true;
    break;
  }
  default:
    break;
  }
}

/************General Messages********************/

void communication::ack_msg(can_frame orig_msg)
{
  struct can_frame canMsgTx;
  canMsgTx.can_id = desk;
  canMsgTx.can_dlc = 8;
  canMsgTx.data[0] = 'A';
  for (int i = 0; i < 6; i++)
  {
    canMsgTx.data[i + 1] = orig_msg.data[i];
  }
  err = can0.sendMessage(&canMsgTx);
  if (err != MCP2515::ERROR_OK)
  {
    // Serial.printf("Error sending message: %s\n", err);
  }
}

/************CONNECTION FUNCTIONS********************/

// Message -> "W A/N/R {desk_number}" (Wake Ack/New/Received)
void communication::connection_msg(char type)
{
  struct can_frame canMsgTx;
  canMsgTx.can_id = desk;
  canMsgTx.can_dlc = 8;
  canMsgTx.data[0] = 'W';
  canMsgTx.data[1] = type;
  canMsgTx.data[2] = int_to_char_msg(0); // DESK TO WHICH THE MESSAGE IS DIRECTED, in messages that require it
  for (int i = 3; i < 8; i++)
  {
    canMsgTx.data[i] = ' ';
  }
  err = can0.sendMessage(&canMsgTx);
  if (err != MCP2515::ERROR_OK)
  {
    // MESSAGE NOT SENT
  }
}

/************CALIBRATION FUNCTIONS********************/
// Message -> "C B/E/F/R/S {desk_number}" (Calibration Beginning/External/Finished/Read/Start)
void communication::calibration_msg(int dest_desk, char type)
{
  struct can_frame canMsgTx;
  canMsgTx.can_id = desk; // alfa
  canMsgTx.can_dlc = 8;
  canMsgTx.data[0] = 'C';
  canMsgTx.data[1] = type;
  canMsgTx.data[2] = int_to_char_msg(dest_desk); // DESK TO WHICH THE MESSAGE IS DIRECTED, in messages that require it
  for (int i = 3; i < 8; i++)
  {
    canMsgTx.data[i] = ' ';
  }
  err = can0.sendMessage(&canMsgTx);
  if (err != MCP2515::ERROR_OK)
  {
    // MESSAGE NOT SENT
  }
  if (type != 'F' && type != 'S')
  {
    missing_ack = desks_connected;
    time_ack = millis();
    last_msg_sent = canMsgTx;
    last_msg_counter = 0;
  }
}

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
      desks_connected.insert(canMsgRx.can_id);
    }
  }
  break;
  case 'A':
  {
    if (is_connected)
    {
      desks_connected.insert(canMsgRx.can_id);
    }
  }
  break;
  default:
    // Serial.printf("ERROR DURING WAKE UP. Message W %c received.\n", canMsgRx.data[1]);
    break;
  }
}

// CONSENSUS
void communication::msg_received_consensus(can_frame canMsgRx)
{
  double d[3];
  memcpy(&d[0], canMsgRx.data + (2 * sizeof(char) + sizeof(int)), sizeof(d[0]));
  memcpy(&d[1], canMsgRx.data + (2 * sizeof(char) + sizeof(int) + sizeof(d[0])), sizeof(d[1]));
  memcpy(&d[2], canMsgRx.data + (2 * sizeof(char) + sizeof(int) + 2 * sizeof(d[0])), sizeof(d[2]));
  // otherD[0][0] = d[0];
  // otherD[0][1] = d[1]; // ver de que desk veio o valor
  // otherD[0][2] = d[2];
  //  Send ack
  ack_msg(canMsgRx);
}

void communication::consensus_msg(double d[3])
{
  struct can_frame canMsgTx;
  canMsgTx.can_id = desk;
  canMsgTx.can_dlc = 8;
  canMsgTx.data[0] = 'S';
  canMsgTx.data[1] = 'D';
  memcpy(canMsgTx.data + (2 * sizeof(char) + sizeof(int)), &d[0], sizeof(d[0]));
  memcpy(canMsgTx.data + (2 * sizeof(char) + sizeof(int) + sizeof(d[0])), &d[1], sizeof(d[1]));
  memcpy(canMsgTx.data + (2 * sizeof(char) + sizeof(int) + 2 * sizeof(d[0])), &d[2], sizeof(d[2]));
  err = can0.sendMessage(&canMsgTx);
  if (err != MCP2515::ERROR_OK)
  {
    // MESSAGE NOT SENT
  }
  // wich desk sent the message resets
}

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

int communication::char_msg_to_int(char msg)
{
  return msg - '0';
}

char communication::int_to_char_msg(int msg)
{
  return msg + '0';
}

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
      // Serial.printf("Node %d removed from the connected nodes do to inactivity\n", element);
    }
    missing_ack = desks_connected;
  }
  err = can0.sendMessage(&last_msg_sent);
  if (err != MCP2515::ERROR_OK)
  {
    // MESSAGE NOT SENT
  }
}