#include "extrafunctions.h"
/*
 * Chenge the iluminance value of the LED
 */
void ChangeLEDValue(int value)
{
  analogWrite(LED_PIN, value);
}

// Conversões

/*
 * Convert the adc measure into volts
 * @param read_adc: measure of adc
 */
float adc_to_volt(int read_adc)
{
  return read_adc / adc_conv;
}
/*
 * Convert the volts values in adc
 * @param input_volt: value in volts
 */
int volt_to_adc(float input_volt)
{
  return input_volt * adc_conv;
}
/*
 * Convert the adc measure in lux's
 * @param read_adc: measure of adc
 */
float adc_to_lux(int read_adc)
{
  float LDR_volt;
  LDR_volt = read_adc / adc_conv;
  return volt_to_lux(LDR_volt);
}
/*
 * Convert the volts value in lux's
 * @param volt: value in volts
 */
float volt_to_lux(float volt)
{
  float LDR_resistance = (VCC * 10000.0) / volt - 10000.0;
  return pow(10, (log10(LDR_resistance) - my_desk.getOffset_R_Lux()) / (my_desk.getM()));
}

/*
 * Mean of the last "value" values to reduce the noise
 * @param value: number of values to do tge mean
 */
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
/*
 * Change the reference of iluminance of the present node
 * @param value: value to change the reference
 */
void ref_change(float value)
{
  my_desk.setRef(value);
  my_desk.setIgnoreReference(false);
  my_pid.set_b(my_desk.getRefVolt() / my_desk.getRef(), my_desk.getGain());
  my_pid.set_Ti(Tau(my_desk.getRef()));
}
/*
 * Capute the Tau value
 * @param value: reference of iluminance
 */
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
/*
 * Send "a" or "e" to Can Bus
 * @param cmd: receive 0 if err or 1 to ack
 */
void send_ack_err(int cmd) // 0-err    1-ack
{
  struct can_frame canMsgTx;
  canMsgTx.can_id = 1;
  canMsgTx.can_dlc = 8;
  if (cmd == 0)
  {
    canMsgTx.data[0] = 'Y';
  }
  else if (cmd == 1)
  {
    canMsgTx.data[0] = 'Z';
  }

  comm.sendMsg(&canMsgTx);
  if (comm.getError() != MCP2515::ERROR_OK)
  {
    Serial.printf("Error sending message \n");
  }
}

/*
 * Send 3 values of duty cycle or iluminance to CanBus
 * @param array: the three values
 * @param flag: to choose between lux or duty cycle
 */
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
