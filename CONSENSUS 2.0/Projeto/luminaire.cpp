#include "luminaire.h"

luminaire::luminaire(float _m, float _offset_R_Lux, float _Pmax, double ref_value, int _desk_number)
    : m{_m}, offset_R_Lux{_offset_R_Lux}, Pmax{_Pmax}, G{0}, desk_number{_desk_number},
      lux_flag{false}, duty_flag{false}, ignore_reference{false}, buffer_full{false},
      Energy_avg{0.0}, counter_avg{0}, hub{false}
{
  setRef(ref_value);
}

void luminaire::store_buffer_l(int flag, float lux)
{
  last_minute_buffer_l[flag][idx_buffer_l[flag]] = lux;
  idx_buffer_l[flag]++;
  if (idx_buffer_l[flag] == buffer_size)
  {
    idx_buffer_l[flag] = 0;
    buffer_full_l[flag] = true; // Sinalizar que o buffer foi preenchido completamente
  }
}

void luminaire::store_buffer_d(int flag, float duty_cycle)
{
  last_minute_buffer_d[flag][idx_buffer_d[flag]] = duty_cycle;
  idx_buffer_d[flag]++;
  if (idx_buffer_d[flag] == buffer_size)
  {
    idx_buffer_d[flag] = 0;
    buffer_full_d[flag] = true; // Sinalizar que o buffer foi preenchido completamente
  }
}

void luminaire::Compute_avg(float h, float lux, float reference, int desk)
{
  // visibility
  float visibility;
  float diff = reference - lux;
  if (diff > 0)
  {
    visibility = diff;
  }
  else
  {
    visibility = 0;
  }

  // flicker
  float d_k_1, d_k_2, d_k, flicker;
  int idx = idx_buffer_l[desk - 1] - 1;
  if (buffer_full == false && idx_buffer_l[desk - 1] < 2)
  {
    flicker = 0;
  }
  else
  {
    d_k = DutyCycle / 100.0;
    if (idx == -1)
    {
      idx = buffer_size - 1;
    }
    d_k_1 = last_minute_buffer_d[desk][idx] / 100.0;
    idx--;
    if (idx == -1)
    {
      idx = buffer_size - 1;
    }
    d_k_2 = last_minute_buffer_d[desk][idx] / 100.0;
    if ((d_k - d_k_1) * (d_k_1 - d_k_2) < 0)
    {
      flicker = std::abs(d_k - d_k_1) + std::abs(d_k_1 - d_k_2);
    }
    else
    {
      flicker = 0;
    }
  }

  // energy
  float energy = Pmax * d_k_1 * h;
  addAvgs(energy, visibility, flicker);
}

float luminaire::lux_to_volt(float lux)
{
  float resistance = pow(10, (m * log10(lux) + offset_R_Lux));
  return (3.3 * 10000.0) / (resistance + 10000.0); // Mudar vcc para variavel
}
