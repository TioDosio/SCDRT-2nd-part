// MESSAGES
MCP2515::ERROR err;
MCP2515 can0{spi0, 17, 19, 16, 18, 10000000};
std::queue<can_frame> command_queue;
can_frame last_msg_sent;
long time_ack = 0, last_write = 0;

// CALIBRATION
bool is_calibrated{false};
float *desk_array = NULL;
float light_off, light_on;
bool flag_temp = false;
// Start
