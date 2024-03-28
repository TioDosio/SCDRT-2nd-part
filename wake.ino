


// MESSAGES
MCP2515::ERROR err;
MCP2515 can0{spi0, 17, 19, 16, 18, 10000000};
std::set<int> missing_ack;
std::queue<can_frame> command_queue;
can_frame last_msg_sent;
int last_msg_counter = 0;
long time_ack = 0, last_write = 0;

// CALIBRATION
bool is_calibrated{false};
float *desk_array = NULL;
float light_off, light_on;
bool flag_temp = false;
// Start
void setup()
{
  Serial.begin(115200);
  analogReadResolution(12);    // default is 10
  analogWriteFreq(60000);      // 60KHz, about max
  analogWriteRange(DAC_RANGE); // 100% duty cycle
  add_repeating_timer_ms(-10, my_repeating_timer_callback, NULL, &timer);
  // ref_volt = lux_to_volt(ref);  //TODO : passar para a classe e depois fazer lá isto
  flash_get_unique_id(this_pico_flash_id);
  node_address = this_pico_flash_id[5];
  can0.reset();
  can0.setBitrate(CAN_1000KBPS);
  can0.setNormalMode();
  gpio_set_irq_enabled_with_callback(interruptPin, GPIO_IRQ_EDGE_FALL, true, &read_interrupt);

  // Connection timers
  randomSeed(node_address);
  time_to_connect += (random(21) * 100); // To ensure that the nodes don't connect at the same time
  connect_time = millis();
}

void loop()
{
  static unsigned long timer_new_node = 0;
  long time_now = millis();
  if (!is_connected)
  {
    if (time_now - connect_time > time_to_connect)
    {
      is_connected = true;
      desk = find_desk();
      connection_msg('A');
      flag_temp = true;
      // TODO Confirmar mensagens de W N antes de correr o new_calibration
      // TODO CORRER O NEW CALIBRATION
    }
    else
    {
      if (time_now - timer_new_node > 200) // Send a new connection message every 200ms
      {
        connection_msg('N');
        timer_new_node = millis();
      }
    }
  }
  if (data_available)
  {
    data_available = false;
    if (!missing_ack.empty()) // Check if something has been received
    {
      while (can0.checkReceive() && !missing_ack.empty())
      {
        can_frame canMsgRx;
        can0.readMessage(&canMsgRx);

        if (canMsgRx.data[0] != 'A')
        {
          command_queue.push(canMsgRx);
        }
        else
        {
          confirm_msg(canMsgRx);
          if (missing_ack.empty())
          {
            switch (last_msg_sent.data[0]) // Trigger of some actions after receiving the ack
            {
            case 'C':
              switch (last_msg_sent.data[1])
              {
              case 'B':
              {
                light_off = adc_to_lux(digital_filter(50.0));
                calibration_msg(0, 'E');
              }
              break;
              case 'E':
              {
                calibration_msg(1, 'S');
              }
              break;
              case 'R':
              {
                if (desk != desks_connected.size() + 1)
                {
                  analogWrite(LED_PIN, 0);
                  calibration_msg(desk + 1, 'S');
                }
                else
                {
                  is_calibrated = true;
                  calibration_msg(0, 'F');
                  analogWrite(LED_PIN, 0);
                }
              }
              break;
              default:
                break;
              }
            default:
              break;
            }
          }
        }
      }
    }
    else
    {
      while (can0.checkReceive())
      {
        can_frame canMsgRx;
        can0.readMessage(&canMsgRx);
        if (canMsgRx.data[2] == int_to_char_msg(desk) || canMsgRx.data[2] == int_to_char_msg(0))
        {
          command_queue.push(canMsgRx);
        }
      }
      while (!command_queue.empty())
      {
        can_frame canMsgRx;
        canMsgRx = command_queue.front();
        switch (canMsgRx.data[0])
        {
        case 'W':
          msg_received_connection(canMsgRx);
          break;
        case 'C':
          msg_received_calibration(canMsgRx);
          break;
        case 'S':
          msg_received_consensus(canMsgRx);
          break;
        default:
          break;
        }
        command_queue.pop();
      }
    }
  }

  time_now = millis();
  // RESEND LAST MESSAGE IF NO ACK RECEIVED
  if ((time_now - time_ack) > TIME_ACK && !missing_ack.empty())
  {

    Serial.printf("Se %lu %lu\n", time_now, time_ack);
    resend_last_msg();
    time_ack = millis();
  }
  if (flag_temp && (desks_connected.size() + 1) == 3)
  {
    if (desk == 3)
    {
      new_calibration();
    }
    flag_temp = false;
  }
  if (Serial.available() > 0) // TO DELETE
  {
    char c = Serial.read();
    if (is_connected)
    {
      Serial.printf("Desk Number: %d\n", desk);
      Serial.printf("Conectadas a mim:");
      for (const int &elem : desks_connected)
      {
        Serial.printf(" %d,", elem);
      }
      Serial.printf("\nTime to connect -> %d\n", time_to_connect);
      if (is_calibrated)
      {
        for (int i = 1; i <= desks_connected.size() + 1; i++)
        {
          Serial.printf("Desk %d -> %f\n", i, desk_array[i - 1]);
        }
      }
    }
    else
    {
      Serial.printf("Not connected yet\n");
    }
  }
}

/************CONNECTION FUNCTIONS********************/

// Message -> "W A/N/R {desk_number}" (Wake Ack/New/Received)
void connection_msg(char type)
{
  struct can_frame canMsgTx;
  canMsgTx.can_id = desk;
  canMsgTx.can_dlc = 8;
  canMsgTx.data[0] = 'W';
  canMsgTx.data[1] = type;
  canMsgTx.data[2] = int_to_char_msg(0); // DESK TO WHICH THE MESSAGE IS DIRECTED, in messages that require it
  for (int i = 3; i < 8; i++)
  {
    canMsgTx.data[i] = ' ';
  }
  err = can0.sendMessage(&canMsgTx);
  if (err != MCP2515::ERROR_OK)
  {
    Serial.printf("Error sending message: %s\n", err);
  }
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

void msg_received_connection(can_frame canMsgRx)
{

  switch (canMsgRx.data[1])
  {
  case 'N':
  {
    if (is_connected)
    {
      connection_msg('R');
    }
  }
  break;
  case 'R':
  {
    if (!is_connected)
    {
      desks_connected.insert(canMsgRx.can_id);
    }
  }
  break;
  case 'A':
  {
    if (is_connected)
    {
      desks_connected.insert(canMsgRx.can_id);
    }
  }
  break;
  default:
    Serial.printf("ERROR DURING WAKE UP. Message W %c received.\n", canMsgRx.data[1]);
    break;
  }
}

/************CALIBRATION FUNCTIONS********************/
// Message -> "C B/E/F/R/S {desk_number}" (Calibration Beginning/External/Finished/Read/Start)
void calibration_msg(int dest_desk, char type)
{
  struct can_frame canMsgTx;
  canMsgTx.can_id = desk;
  canMsgTx.can_dlc = 8;
  canMsgTx.data[0] = 'C';
  canMsgTx.data[1] = type;
  canMsgTx.data[2] = int_to_char_msg(dest_desk); // DESK TO WHICH THE MESSAGE IS DIRECTED, in messages that require it
  for (int i = 3; i < 8; i++)
  {
    canMsgTx.data[i] = ' ';
  }
  err = can0.sendMessage(&canMsgTx);
  if (err != MCP2515::ERROR_OK)
  {
    Serial.printf("Error sending message: %s\n", err);
  }
  if (type != 'F' && type != 'S')
  {
    missing_ack = desks_connected;
    time_ack = millis();
    last_msg_sent = canMsgTx;
    last_msg_counter = 0;
  }
}

void new_calibration()
{
  desk_array = (float *)malloc((desks_connected.size() + 1) * sizeof(float)); // array of desks
  if (desks_connected.empty())
  {
    Gain();
    is_calibrated = true;
  }
  else
  {
    calibration_msg(0, 'B');
    analogWrite(LED_PIN, 0);
  }
}

void msg_received_consensus(can_frame canMsgRx)
{
  switch (canMsgRx.data[1])
  {
  case 'D':
    otherD;
    break;

  default:
    break;
  }
}

void msg_received_calibration(can_frame canMsgRx)
{
  switch (canMsgRx.data[1])
  {
  case 'B': // Quando lerem o begin desligam as luzes
  {
    // TODO : LIMPAR TODOS OS OUTRAS MENSAGENS DE CALIBRATIONS
    if (desk_array != NULL)
    {
      free(desk_array);
    }
    desk_array = (float *)malloc((desks_connected.size() + 1) * sizeof(float)); // array of desks
    ack_msg(canMsgRx);
    analogWrite(LED_PIN, 0);
  }
  break;
  case 'E':
  {
    light_off = adc_to_lux(digital_filter(50.0));
    ack_msg(canMsgRx);
  }
  break;
  case 'F':
  {
    is_calibrated = true;
    Serial.printf("Calibration Finished through message\n");
  }
  break;
  case 'R':
  {
    light_on = adc_to_lux(digital_filter(50.0));
    ack_msg(canMsgRx);
    Serial.printf("Desk %d -> %f %f\n", canMsgRx.can_id, light_on, light_off);
    desk_array[canMsgRx.can_id - 1] = light_on - light_off;
  }
  break;
  case 'S':
  {
    if (char_msg_to_int(canMsgRx.data[2]) == desk)
    {
      cross_gains();
    }
    else
    {
      analogWrite(LED_PIN, 0);
    }
  }
  break;
  default:
    Serial.printf("ERROR DURING CALIBRATION. Message C %c received.\n", canMsgRx.data[1]);
    break;
  }
}

void cross_gains()
{
  analogWrite(LED_PIN, 4095);
  delay_manual(2000);
  calibration_msg(0, 'R');
  light_on = adc_to_lux(digital_filter(50.0));
  Serial.printf("Desk %d -> %f %f\n", desk, light_on, light_off);
  desk_array[desk - 1] = light_on - light_off;
}

void Gain()
{
  Serial.println("Calibrating the gain of the system:");
  analogWrite(LED_PIN, 0);
  delay_manual(2500);
  light_off = adc_to_lux(digital_filter(50.0));
  analogWrite(LED_PIN, 4095);
  delay_manual(2500);
  light_on = adc_to_lux(digital_filter(50.0));
  analogWrite(LED_PIN, 0);
  delay_manual(2500);
  desk_array[0] = (light_on - light_off);
  Serial.printf("Calibration Finished\n");
}

/************General Messages********************/

void ack_msg(can_frame orig_msg)
{
  struct can_frame canMsgTx;
  canMsgTx.can_id = desk;
  canMsgTx.can_dlc = 8;
  canMsgTx.data[0] = 'A';
  for (int i = 0; i < 6; i++)
  {
    canMsgTx.data[i + 1] = orig_msg.data[i];
  }
  err = can0.sendMessage(&canMsgTx);
  if (err != MCP2515::ERROR_OK)
  {
    Serial.printf("Error sending message: %s\n", err);
  }
}

void confirm_msg(can_frame ack_msg)
{
  for (int i = 0; i < 6; i++)
  {
    if (ack_msg.data[i + 1] != last_msg_sent.data[i])
    {
      return;
    }
  }
  missing_ack.erase(ack_msg.can_id);
  return;
}

void resend_last_msg()
{
  last_msg_counter++;
  if (last_msg_counter == 5) // After 5 tries, remove the nodes that didn't ack
  {
    // TODO : CHANGE THE HUB IF THE ONE DISCONNECTED WAS THE HUB
    last_msg_counter = 0;
    for (const int &element : missing_ack)
    {
      desks_connected.erase(element);
      Serial.printf("Node %d removed from the connected nodes do to inactivity\n", element);
    }
    missing_ack = desks_connected;
  }
  err = can0.sendMessage(&last_msg_sent);
  if (err != MCP2515::ERROR_OK)
  {
    Serial.printf("Error sending message: %s\n", err);
  }
}

/************UTILS********************/

void delay_manual(unsigned long delay)
{
  unsigned long delay_start = millis();
  unsigned long delay_end = millis();
  while (delay_end - delay_start < delay)
  {
    delay_end = millis();
  }
}

float digital_filter(float value)
{
  float total_adc;
  int j;
  for (j = 0, total_adc = 0; j < value; j += 1)
  {
    total_adc += analogRead(A0);
  }
  return total_adc / value;
}

int char_msg_to_int(char msg)
{
  return msg - '0';
}

char int_to_char_msg(int msg)
{
  return msg + '0';
}

float adc_to_lux(int read)
{
  float LDR_volt;
  LDR_volt = read / adc_conv;
  return volt_to_lux(LDR_volt);
}

float volt_to_lux(float volt)
{
  float LDR_resistance = (VCC * 10000.0) / volt - 10000.0;
  return pow(10, (log10(LDR_resistance) - (log10(225000) - (-0.89))) / (-0.89));
}