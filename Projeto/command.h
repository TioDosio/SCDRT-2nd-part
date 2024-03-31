#ifndef COMMAND_H
#define COMMAND_H
#include "Arduino.h"

void read_command();
void real_time_stream_of_data(unsigned long time, float lux);
void start_consensus();
#endif // COMMAND_Hs