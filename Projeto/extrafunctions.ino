#include "extrafunctions.h"

void ChangeLEDValue(int value)
{
  analogWrite(LED_PIN, value);
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

void send_ack_err(int cmd) // 0-err    1-ack
{
  struct can_frame canMsgTx;
  canMsgTx.can_id = 1;
  if (cmd == 0)
  {
    canMsgTx.can_dlc = 1;
    canMsgTx.data[0] = 'e';
  }
  else if (cmd == 1)
  {
    canMsgTx.can_dlc = 1;
    canMsgTx.data[0] = 'a';
  }

  comm.sendMsg(&canMsgTx);
  if (comm.getError() != MCP2515::ERROR_OK)
  {
    Serial.printf("Error sending message \n");
  }
}

void send_arrays_buff(float array[3], int flag)
{
  struct can_frame canMsgTx;
  canMsgTx.can_id = 1;
  canMsgTx.can_dlc = 8;
  if (flag == 0)
  {
    canMsgTx.data[0] = 'L';
    canMsgTx.data[1] = comm.int_to_char_msg(my_desk.getDeskNumber());
  }
  else
  {
    canMsgTx.data[0] = 'D';
    canMsgTx.data[1] = comm.int_to_char_msg(my_desk.getDeskNumber());
  }
  int msg;
  for (int i = 2, j = 0; i < 8; i += 2, j++)
  {
    msg = static_cast<int>(array[j] * 100);
    canMsgTx.data[i] = static_cast<unsigned char>(msg & 255);    // Same as msg0 % 256, but more efficient
    canMsgTx.data[i + 1] = static_cast<unsigned char>(msg >> 8); // Same as msg0 / 256, but more efficient
  }
  comm.sendMsg(&canMsgTx);
  if (comm.getError() != MCP2515::ERROR_OK)
  {
    Serial.printf("Error sending message \n");
  }
}
