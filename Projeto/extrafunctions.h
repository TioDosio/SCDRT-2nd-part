void ChangeLEDValue(int value);
float adc_to_volt(int read_adc);
int volt_to_adc(float input_volt);
float adc_to_lux(int read_adc);
float volt_to_lux(float volt);
float digital_filter(float value);
void ref_change(float value);
void send_arrays_buff(float array[3], int flag);
