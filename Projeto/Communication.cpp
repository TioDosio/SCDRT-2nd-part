#include "Communication.h"
#include <cstring>
#include <Arduino.h>

communication::communication(luminaire *_my_desk) : my_desk{_my_desk}, is_connected{false}, time_to_connect{500}, consensus_acknoledged{false}, light_off{0.0}, light_on{0.0}, is_calibrated{false}, coupling_gains{NULL}
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
    // Serial.printf("Error sending message: %s\n", err);
  }
}

/************CONNECTION FUNCTIONS********************/

// Message -> "W A/N/R {desk_number}" (Wake Ack/New/Received)
void communication::connection_msg(char type)
{
  struct can_frame canMsgTx;
  canMsgTx.can_id = my_desk->getDeskNumber();
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
    // TODO MESSAGE NOT SENT MENSAGENS DE ERRO TODAS MAL
  }
}

/************CALIBRATION FUNCTIONS********************/
// Message -> "C B/E/F/R/S {desk_number}" (Calibration Beginning/External/Finished/Read/Start)
void communication::calibration_msg(int dest_desk, char type)
{
  struct can_frame canMsgTx;
  canMsgTx.can_id = my_desk->getDeskNumber();
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

void communication::msg_received_calibration(can_frame canMsgRx)
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
  }
  break;
  case 'R':
  {
    light_on = adc_to_lux(digital_filter(50.0));
    ack_msg(canMsgRx);
    coupling_gains[canMsgRx.can_id - 1] = light_on - light_off;
  }
  break;
  case 'S':
  {
    if (char_msg_to_int(canMsgRx.data[2]) == my_desk->getDeskNumber())
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

void communication::cross_gains()
{
  ChangeLEDValue(4095);
  delay_manual(3000);
  calibration_msg(0, 'R');
  light_on = adc_to_lux(digital_filter(50.0));
  coupling_gains[my_desk->getDeskNumber() - 1] = light_on - light_off;
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
    // TODO Substituir o serial
    break;
  }
}

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
void communication::msg_received_consensus(can_frame canMsgRx)
{
  double d[3];
  // memcpy(&d, canMsgRx.data + (2 * sizeof(char) + sizeof(int)), sizeof(d)); // TODO ver se isto está correto

  // otherD[0][0] = d[0];
  // otherD[0][1] = d[1]; // TODO ver de que desk veio o valor
  // otherD[0][2] = d[2];
  //  Send ack
  ack_msg(canMsgRx);
}

void communication::consensus_msg(double d[3])
{
  struct can_frame canMsgTx;
  canMsgTx.can_id = my_desk->getDeskNumber();
  canMsgTx.can_dlc = 8;
  canMsgTx.data[0] = 'S';
  canMsgTx.data[1] = 'D';
  int msg0, msg1, msg2;
  msg0 = static_cast<int>(d[0] * 100);
  msg1 = static_cast<int>(d[1] * 100);
  msg2 = static_cast<int>(d[2] * 100);
  canMsgTx.data[2] = msg0 % 256;
  canMsgTx.data[3] = msg0 / 256;
  canMsgTx.data[4] = msg1 % 256;
  canMsgTx.data[5] = msg1 / 256;
  canMsgTx.data[6] = msg2 % 256;
  canMsgTx.data[7] = msg2 / 256;
  float duty = (canMsgTx.data[2] + canMsgTx.data[3] * 256) / 100;
  Serial.printf("orig->%f new->%f\n", d[0], duty);
  err = can0.sendMessage(&canMsgTx);
  if (err != MCP2515::ERROR_OK)
  {
    // MESSAGE NOT SENT
  }
  // wich desk sent the message resets
}

//------------------------UTILS------------------------
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
  // / if (err != MCP2515::ERROR_OK)
  // {
  //   // MESSAGE NOT SENT
  // }
}

void communication::delay_manual(unsigned long delay)
{
  unsigned long delay_start = millis();
  unsigned long delay_end = millis();
  while (delay_end - delay_start < delay)
  {
    delay_end = millis();
  }
}