#include "Communication.h"

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
        ChangeLEDValue(LED_PIN, 0);
        calibration_msg(desk + 1, 'S');
      }
      else
      {
        is_calibrated = true;
        calibration_msg(0, 'F');
        ChangeLEDValue(LED_PIN, 0);
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

// CONSENSUS
void communication::msg_received_consensus(can_frame canMsgRx)
{
  double d[3];
  memcpy(&d[0], canMsgRx.data + (2 * sizeof(char) + sizeof(int)), sizeof(d[0]));
  memcpy(&d[1], canMsgRx.data + (2 * sizeof(char) + sizeof(int) + sizeof(d[0])), sizeof(d[1]));
  memcpy(&d[2], canMsgRx.data + (2 * sizeof(char) + sizeof(int) + 2 * sizeof(d[0])), sizeof(d[2]));
  otherD[wichDesk][0] = d[0];
  otherD[wichDesk][1] = d[1];
  otherD[wichDesk][2] = d[2];
  wichDesk++;
  // Send ack
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
    Serial.printf("Error sending message: %s\n", err);
  }
  wichDesk = 0;
}
