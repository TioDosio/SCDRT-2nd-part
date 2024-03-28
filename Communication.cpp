#include "Communication.h"

communication::communication() : is_connected{false}, time_to_connect{500}
{
}

int find_desk()
{
  int i = 1;
  while (desks_connected.find(i) != desks_connected.end())
  {
    i++;
  }
  return i;
}
