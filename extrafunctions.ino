#include "extrafunctions.h"

inline void ChangeLEDValue(int value) {
  analogWrite(LED_PIN, value);
}

// Conversões
inline float adc_to_volt(int read_adc)
{
  return read_adc / adc_conv;
}

inline int volt_to_adc(float input_volt)
{
  return input_volt * adc_conv;
}

inline float adc_to_lux(int read_adc)
{
  float LDR_volt;
  LDR_volt = read_adc / adc_conv;
  return volt_to_lux(LDR_volt);
}

inline float volt_to_lux(float volt)
{
  float LDR_resistance = (VCC * 10000.0) / volt - 10000.0;
  return pow(10, (log10(LDR_resistance) - my_desk.getOffset_R_Lux()) / (my_desk.getM()));
}
