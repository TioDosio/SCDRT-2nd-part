#include "pid.h"

using namespace std;

pid::pid(float _h, float _K, float Tt_,
         float b_, float Ti_, float Td_, float N_)
  : h{ _h }, K{ _K }, b{ b_ }, Ti{ Ti_ }, Td{ Td_ }, Tt{ Tt_ },
    N{ N_ }, I{ 0.0 }, D{ 0.0 }, y_old{ 0.0 }, b_old{ b_ }, K_old{ _K }, anti_windup{ true }, feedback{ true }, bumpless{ true } {
}

float pid::compute_control(float r, float y) {
  if (bumpless) {
    I = I + K_old * (b_old * r - y) - K * (b * r - y);  // Bumpless
  }
  b_old = b;
  K_old = K;
  P = K * (-y);
  float ad = Td / (Td + N * h);
  float bd = Td * K * N / (Td + N * h);
  D = ad * D - bd * (y - y_old);
  u_fb = P + I + D;
  v = (u_ff + u_fb);
  u = v;
  if (u > 1) {
    u = 1.0;
  } else if (u < 0) {
    u = 0.0;
  }
  return u;
}

float pid::get_u() {
  v = (u_ff + u_fb);
  u = v;
  if (u > 1) {
    u = 1.0;
  } else if (u < 0.0) {
    u = 0.0;
  }
  return u;
}
void pid::compute_feedforward(float r) {
  u_fb = 0;
  u_ff = (r * b * K);
}

void pid::set_antiwindup(bool set) {
  anti_windup = set;
  return;
}

bool pid::get_antiwindup() {
  return anti_windup;
}

void pid::set_feedback(bool set) {
  if (feedback == true && set == false) {  // Quando se desativa o feedback, meter o integral a 0
    I = 0.0;
  }
  feedback = set;
  return;
}

void pid::set_bumpless(bool set) {
  bumpless = set;
  return;
}

bool pid::get_feedback() {
  return feedback;
}

void pid::set_b(float H, float Gain) {
  b = 1 / (K * H * Gain);
}

void pid::set_b(float _b) {
  b = _b;
}
void pid::set_k(float k_) {
  K = k_;
}

void pid::set_Ti(float Ti_) {
  Ti = Ti_;
}

void pid::set_Tt(float Tt_) {
  Tt = Tt_;
}

float pid::get_k() {
  return K;
}
float pid::get_b() {
  return b;
}

float pid::get_h() {
  return h;
}

float pid::get_Ti() {
  return Ti;
}

float pid::get_Tt() {
  return Tt;
}

float pid::get_Td() {
  return Td;
}

float pid::get_N() {
  return N;
}


float pid::get_u_ff() {
  return u_ff;
}

float pid::get_u_fb() {
  return u_fb;
}

float pid::get_I() {
  return I;
}

float pid::get_P() {
  return P;
}