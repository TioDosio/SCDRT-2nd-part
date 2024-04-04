#ifndef luminaire_H
#define luminaire_H
#define buffer_size 6000
#include <cmath>

class luminaire
{
  float m, offset_R_Lux, Pmax, DutyCycle, G, ref_unoccupied, ref_occupied, ref, ref_volt;
  float last_minute_buffer_l[3][buffer_size], last_minute_buffer_d[3][buffer_size];
  bool lux_flag, duty_flag, ignore_reference, buffer_full, hub;
  int desk_number, idx_buffer_l[3], idx_buffer_d[3];
  bool buffer_full_l[3], buffer_full_d[3];
  double Energy_avg, visibility_err, flicker_err;
  unsigned long counter_avg;

public:
  explicit luminaire(float _m, float _offset_R_Lux, float _Pmax, double ref_value, int _desk_number = 1);
  ~luminaire(){};
  void store_buffer_l(int flag, float lux);
  void store_buffer_d(int flag, float duty_cycle);
  void Compute_avg(float h, float lux, float reference, int desk);
  float lux_to_volt(float lux);

  // Setters
  void setRef(double value)
  {
    ref = value;
    ref_volt = lux_to_volt(ref);
  }

  void setDeskNumber(int value)
  {
    desk_number = value;
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
  float getRef() const
  {
    return ref;
  }

  float getRefVolt() const
  {
    return ref_volt;
  }

  bool isDutyFlag() const
  {
    return duty_flag;
  }

  bool isLuxFlag() const
  {
    return lux_flag;
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

  int getIdxBuffer_l(int wich_desk) const
  {
    return idx_buffer_l[wich_desk];
  }
  int getIdxBuffer_d(int wich_desk) const
  {
    return idx_buffer_d[wich_desk];
  }

  float getGain() const
  {
    return G;
  }

  float getLastMinuteBufferD(int desk, int index) const
  {
    return last_minute_buffer_d[desk][index];
  }

  float getLastMinuteBufferL(int desk, int index) const
  {
    return last_minute_buffer_l[desk][index];
  }

  bool isIgnoreReference() const
  {
    return ignore_reference;
  }

  int getDeskNumber() const
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
  void setHub()
  {
    hub = true;
  }
  bool getHub()
  {
    return hub;
  }
};

#endif // luminaire_H