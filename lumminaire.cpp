#include "lumminaire.h"
lumminaire::lumminaire(float _m, float _offset_R_Lux, float _Pmax, unsigned short _desk_number)
  : m{ _m }, offset_R_Lux{ _offset_R_Lux }, Pmax{ _Pmax }, G{ 0 }, desk_number{ _desk_number },
    occupied{ false }, lux_flag{ false }, duty_flag{ false }, ignore_reference{ false }, buffer_full{ false },
    idx_buffer{ 0 }, Energy_avg{ 0.0 }, counter_avg{ 0 }, on{true}, ref_occupied{ 20.0 }, ref_unoccupied{ 5.0 } {
}

void lumminaire::store_buffer(float lux) {
  last_minute_buffer_d[idx_buffer] = DutyCycle;
  last_minute_buffer_l[idx_buffer] = lux;
  idx_buffer++;
  if (idx_buffer == buffer_size) {
    idx_buffer = 0;
    buffer_full = true;  //Sinalizar que o buffer foi preenchido completamente
  }
}

void lumminaire::Compute_avg(float h, float lux, float reference) {

  // visibility
  float visibility;
  float diff = reference-lux;
  if(diff > 0){
    visibility = diff;
  }
  else {
  visibility = 0;
  }

  // flicker
  float d_k_1, d_k_2, d_k, flicker;
  int idx = idx_buffer - 1;
  if (buffer_full == false && idx_buffer < 2) {
    flicker = 0;
  } else {
    d_k = DutyCycle / 100.0;
    if (idx == -1) {
      idx = buffer_size - 1;
    }
    d_k_1 = last_minute_buffer_d[idx] / 100.0;
    idx--;
    if (idx == -1) {
      idx = buffer_size - 1;
    }
    d_k_2 = last_minute_buffer_d[idx] / 100.0;
    if ((d_k - d_k_1) * (d_k_1 - d_k_2) < 0) {
      flicker = std::abs(d_k - d_k_1) + std::abs(d_k_1 - d_k_2);
    } else {
      flicker = 0;
    }
  }

  // energy
  float energy = Pmax * d_k_1 * h;
  addAvgs(energy, visibility, flicker);
}