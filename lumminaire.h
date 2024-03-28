#ifndef LUMMINAIRE_H
#define LUMMINAIRE_H
#define buffer_size 6000
#include <cmath>

class lumminaire
{
  float m, offset_R_Lux, Pmax, DutyCycle, G, ref_unoccupied, ref_occupied;
  float last_minute_buffer_d[buffer_size], last_minute_buffer_l[buffer_size];
  bool occupied, lux_flag, duty_flag, ignore_reference, buffer_full, on;
  unsigned short desk_number, idx_buffer;
  double Energy_avg, visibility_err, flicker_err;
  unsigned long counter_avg;

public:
  explicit lumminaire(float _m, float _offset_R_Lux, float _Pmax, unsigned short _desk_number);
  ~lumminaire(){};
  void store_buffer(float lux);
  void Compute_avg(float h, float lux, float reference);

  // Setters

  void setOccupied(bool value)
  {
    occupied = value;
  }

  void setON(bool value)
  {
    on = value;
  }

  void setDutyFlag(bool value)
  {
    duty_flag = value;
  }

  void setIgnoreReference(bool value)
  {
    ignore_reference = value;
  }

  void setDutyCycle(float value)
  {
    DutyCycle = value;
  }

  void setLuxFlag(bool value)
  {
    lux_flag = value;
  }
  void setGain(float value)
  {
    G = value;
  }

  void addAvgs(float energy, float visibility, float flicker)
  {
    Energy_avg += energy;
    visibility_err += visibility;
    flicker_err += flicker;
    counter_avg++;
  }

  // Getters

  bool isOccupied() const
  {
    return occupied;
  }

  bool isON() const
  {
    return on;
  }

  bool isDutyFlag() const
  {
    return duty_flag;
  }

  bool isLuxFlag() const
  {
    return lux_flag;
  }

  bool isBufferFull() const
  {
    return buffer_full;
  }

  float getPmax() const
  {
    return Pmax;
  }

  float getOffset_R_Lux() const
  {
    return offset_R_Lux;
  }

  float getM() const
  {
    return m;
  }

  float getRefOccupied() const
  {
    return ref_occupied;
  }

  float getRefUnoccupied() const
  {
    return ref_unoccupied;
  }


  unsigned short getIdxBuffer() const
  {
    return idx_buffer;
  }

  float getGain() const
  {
    return G;
  }

  float getLastMinuteBufferD(unsigned short index) const
  {
    return last_minute_buffer_d[index];
  }

  float getLastMinuteBufferL(unsigned short index) const
  {
    return last_minute_buffer_l[index];
  }

  bool isIgnoreReference() const
  {
    return ignore_reference;
  }

  unsigned short getDeskNumber() const
  {
    return desk_number;
  }

  float getDutyCycle() const
  {
    return DutyCycle;
  }

  float getEnergyAvg() const
  {
    return Energy_avg / counter_avg;
  }

  float getVisibilityErr() const
  {
    return visibility_err / counter_avg;
  }

  float getFlickerErr() const
  {
    return flicker_err / counter_avg;
  }
};

#endif // LUMMINAIRE_H