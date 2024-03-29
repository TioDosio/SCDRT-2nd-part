#ifndef PID_H
#define PID_H


class pid {
  float I, D, K, P, Ti, Td, Tt, b, h, y_old, K_old, b_old, N, u_fb, u_ff, u, v;
  bool anti_windup, feedback, bumpless;

public:
  explicit pid(float _h, float _K = 1, float Tt_ = 1, float b_ = 1, float Ti_ = 1, float Td_ = 0, float N_ = 10);
  ~pid(){};
  float compute_control(float r, float y);
  void compute_feedforward(float r);
  void housekeep(float r, float y);
  void set_antiwindup(bool set);
  void set_feedback(bool set);
  void set_bumpless(bool set);
  void set_b(float H, float Gain);
  void set_b(float b);
  void set_k(float k_);
  void set_Ti(float Ti_);
  void set_Tt(float Tt_);
  float get_u();
  float get_k();
  float get_h();
  bool get_feedback();
  bool get_antiwindup();
  float get_b();
  float get_Ti();
  float get_Tt();
  float get_Td();
  float get_N();
  float get_u_ff();
  float get_u_fb();
  float get_I();
  float get_P();
};

inline void pid::housekeep(float r, float y) {
  float e = r - y;
  const float ao = h / Tt;
  float bi = K * h / Ti;
  I += bi * (e);
  if (anti_windup) {
    I += ao * (u - v);
  }
  y_old = y;
}
#endif  // PID_H