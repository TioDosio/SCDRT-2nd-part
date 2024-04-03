#include "command.h"
#include "Communication.h"

void read_command(const String &buffer, bool fromCanBus)
{
  int i, flag;
  float val;
  int this_desk = my_desk.getDeskNumber();
  char command = buffer.charAt(0), secondCommand, x;

  switch (command)
  {
  case 'd':
    sscanf(buffer.c_str(), "d %d %f", &i, &val);
    if (i == this_desk)
    {
      if (val >= 0 and val <= 100) // DESLIGAR TUDO
      {
        my_desk.setDutyCycle(val);
        analogWrite(LED_PIN, val * dutyCycle_conv);
        my_desk.setIgnoreReference(true);
        if (fromCanBus) // if from can bus and is my desk, process message and send to others
        {
          send_ack_err(1);
        }
        else // if from serial and is my desk, print
        {
          Serial.println("ack");
        }
      }
      else // if the value was incorrect
      {
        if (fromCanBus) // if from can bus and is my desk, process message and send to others
        {
          send_ack_err(0);
        }
        else // if from serial and is my desk, print
        {
          Serial.println("err");
        }
      }
    }
    else // if the desk is not mine
    {
      if (fromCanBus && my_desk.getHub()) // if from can bus and is not my desk, print
      {
        Serial.println(buffer);
      }
      else if (!fromCanBus) // if from serial and is not my desk, send to others
      {
        send_to_others(i, "d", val, 1);
      }
    }
    break;
  case 'r':
    if (buffer.length() > 1)
    {
      sscanf(buffer.c_str(), "r %d %f", &i, &val);
      if (i == this_desk)
      {
        if (val >= 0)
        {
          Serial.printf("r %d %f\n", i, val);
          ref_change(val);
          if (fromCanBus) // if from can bus and is my desk, process message and send to others
          {
            send_ack_err(1);
          }
          else // if from serial and is my desk, print
          {
            Serial.println("ack");
          }
        }
        else
        {
          if (fromCanBus) // if from can bus and is my desk, process message and send to others
          {
            send_ack_err(0);
          }
          else // if from serial and is my desk, print
          {
            Serial.println("err");
          }
        }
      }
      else
      {
        send_to_others(i, "r", val, 1);
      }
    }
    else
    { // TODO RESET and recalibrate the system
      if (fromCanBus)
      {
        send_ack_err(0);
      }
      else
      {
        Serial.println("err");
      }
    }
    break;
  case 'o':
    sscanf(buffer.c_str(), "o %d %d", &i, &flag);
    Serial.printf("%s\n", buffer.c_str());
    if (i == this_desk)
    {
      if (flag > 0 || flag < 3)
      {
        my_desk.setIgnoreReference(false);
        node.setOccupancy(flag);
        comm.start_consensus();
        if (fromCanBus)
        {
          send_ack_err(1);
        }
        else
        {
          Serial.println("ack");
        }
      }
      else
      {
        if (fromCanBus)
        {
          send_ack_err(0);
        }
        else
        {
          Serial.println("err");
        }
      }
    }
    else
    {
      send_to_others(i, "o", static_cast<float>(flag), 1);
    }
    break;
  case 'a':
    sscanf(buffer.c_str(), "a %d %d", &i, &flag);

    if (i == this_desk)
    {
      if (flag == 0 or flag == 1)
      {
        my_pid.set_antiwindup(flag);
        if (fromCanBus)
        {
          send_ack_err(1);
        }
        else
        {
          Serial.println("ack");
        }
      }
      else
      {
        if (fromCanBus)
        {
          send_ack_err(0);
        }
        else
        {
          Serial.println("err");
        }
      }
    }
    else
    {
      send_to_others(i, "a", static_cast<float>(flag), 1);
    }
    break;
  case 'k':
    sscanf(buffer.c_str(), "k %d %d", &i, &flag);
    if (i == this_desk)
    {
      if (flag == 0 or flag == 1)
      {
        my_pid.set_feedback(flag);
        if (fromCanBus)
        {
          send_ack_err(1);
        }
        else
        {
          Serial.println("ack");
        }
      }
      else
      {
        if (fromCanBus)
        {
          send_ack_err(0);
        }
        else
        {
          Serial.println("err");
        }
      }
    }
    else
    {
      send_to_others(i, "k", static_cast<float>(flag), 1);
    }
    break;
  case 's':
    if (buffer.length() == 5)
    {
      sscanf(buffer.c_str(), "s %c %d", &x, &i);
      if (i == this_desk)
      {
        if (x == 'l')
        {
          if (fromCanBus)
          {
            // TODO send streaming data
          }
          else
          {
            my_desk.setLuxFlag(true);
          }
        }
        else if (x == 'd')
        {
          if (fromCanBus)
          {
            // TODO send streaming data
          }
          else
          {
            my_desk.setDutyFlag(true);
          }
        }
        else
        {
          if (fromCanBus)
          {
            send_ack_err(0);
          }
          else
          {
            Serial.println("err");
          }
        }
      }
      else
      {
        String result;
        result.concat('s'); // Add 's'
        result.concat(x);   // Add 'd' or 'l'

        send_to_others(i, result, 0, 0);
      }
    }
    else
    {
      unsigned int time;
      sscanf(buffer.c_str(), "s %c %d %f", &x, &i, &val, &time);
      if (fromCanBus && my_desk.getHub())
      {
        Serial.printf("s %c %d %f %d\n", x, i, val, time);
      }
    }
    break;
  case 'S':
    sscanf(buffer.c_str(), "S %c %d", &x, &i);
    if (i == this_desk)
    {
      if (x == 'l')
      {
        my_desk.setLuxFlag(false);
        if (fromCanBus)
        {
          send_ack_err(1);
        }
        else
        {
          Serial.println("ack");
        }
      }
      else if (x == 'd')
      {
        my_desk.setDutyFlag(false);
        if (fromCanBus)
        {
          send_ack_err(1);
        }
        else
        {
          Serial.println("ack");
        }
      }
      else
      {
        if (fromCanBus)
        {
          send_ack_err(0);
        }
        else
        {
          Serial.println("err");
        }
      }
    }
    else if (my_desk.getHub())
    {
      String result;
      result.concat('S'); // Add 'S'
      result.concat(x);   // Add 'd' or 'l'

      send_to_others(i, result, 0, 0);
    }
    break;
  case 'O':
    sscanf(buffer.c_str(), "O %d %f", &i, &val);
    if (i == this_desk)
    {
      if (val > 0)
      {
        node.setLowerBoundOccupied(val);
        if (node.getOccupancy() == 1)
        {
          comm.start_consensus();
        }
        if (fromCanBus)
        {
          send_ack_err(1);
        }
        else
        {
          Serial.println("ack");
        }
      }
      else
      {
        if (fromCanBus)
        {
          send_ack_err(0);
        }
        else
        {
          Serial.println("err");
        }
      }
    }
    else
    {
      send_to_others(i, "O", val, 1);
    }
    break;
  case 'U':
    sscanf(buffer.c_str(), "U %d %f", &i, &val);
    if (i == this_desk)
    {
      if (val > 0)
      {
        node.setLowerBoundUnoccupied(val);
        if (node.getOccupancy() == 0)
        {
          comm.start_consensus();
        }
        if (fromCanBus)
        {
          send_ack_err(1);
        }
        else
        {
          Serial.println("ack");
        }
      }
      else
      {
        if (fromCanBus)
        {
          send_ack_err(0);
        }
        else if (!fromCanBus)
        {
          Serial.println("err");
        }
      }
    }
    else
    {
      send_to_others(i, "U", val, 1);
    }
    break;
  case 'c':
    sscanf(buffer.c_str(), "c %d %f", &i, &val);
    if (i == this_desk)
    {
      node.setCost(val);
      comm.start_consensus();
      if (fromCanBus)
      {
        send_ack_err(1);
      }
      else
      {
        Serial.println("ack");
      }
    }
    else
    {
      send_to_others(i, "c", val, 1);
    }
    break;
  case 'D':
    debbuging = !debbuging;
    if (fromCanBus)
    {
      send_ack_err(1);
    }
    else
    {
      Serial.println("ack");
    }
    break;
  case 'g':
    secondCommand = buffer.charAt(2);
    switch (secondCommand)
    {
    case 'd':
      sscanf(buffer.c_str(), "g d %d", &i);
      if (i == this_desk) // if the desk is mine
      {
        if (fromCanBus) // if from can bus and is my desk, process message and send to others
        {
          response_msg(my_desk.getDeskNumber(), "d", my_desk.getDutyCycle());
        }
        else // if from serial and is my desk, print
        {
          Serial.printf("d %d %f\n", i, my_desk.getDutyCycle());
        }
      }
      else if (my_desk.getHub()) // if the desk is not mine
      {
        send_to_others(i, "gd", 0, 0);
      }
      break;
    case 'r':
      sscanf(buffer.c_str(), "g r %d", &i);
      if (i == this_desk)
      {
        if (fromCanBus)
        {
          response_msg(my_desk.getDeskNumber(), "r", my_desk.getRef());
        }
        else
        {
          Serial.printf("r %d %f\n", i, my_desk.getRef());
        }
      }
      else if (my_desk.getHub())
      {
        send_to_others(i, "gr", 0, 0);
      }
      break;
    case 'l':
      sscanf(buffer.c_str(), "g l %d", &i);
      if (i == this_desk)
      {
        int read_adc_new;
        float Lux;
        read_adc_new = digital_filter(20.0);
        Lux = adc_to_lux(read_adc_new);
        if (fromCanBus)
        {
          response_msg(my_desk.getDeskNumber(), "l", Lux);
        }
        else
        {
          Serial.printf("l %d %f\n", this_desk, Lux);
        }
      }
      else if (my_desk.getHub())
      {
        send_to_others(i, "gl", 0, 0);
      }
      break;
    case 'o':
      sscanf(buffer.c_str(), "g o %d", &i);
      if (i == this_desk)
      {
        if (fromCanBus)
        {
          response_msg(my_desk.getDeskNumber(), "o", node.getOccupancy());
        }
        else
        {
          Serial.printf("o %d %d\n", i, node.getOccupancy());
        }
      }
      else if (my_desk.getHub())
      {
        send_to_others(i, "go", 0, 0);
      }
      break;
    case 'a':
      sscanf(buffer.c_str(), "g a %d", &i);
      if (i == this_desk)
      {
        if (fromCanBus)
        {
          response_msg(my_desk.getDeskNumber(), "a", my_pid.get_antiwindup());
        }
        else
        {
          Serial.printf("a %d %d\n", i, my_pid.get_antiwindup());
        }
      }
      else if (my_desk.getHub())
      {
        send_to_others(i, "ga", 0, 0);
      }
      break;
    case 'k':
      sscanf(buffer.c_str(), "g k %d", &i);
      if (i == this_desk)
      {
        if (fromCanBus)
        {
          response_msg(my_desk.getDeskNumber(), "k", my_pid.get_feedback());
        }
        else
        {
          Serial.printf("k %d %d\n", i, my_pid.get_feedback());
        }
      }
      else if (my_desk.getHub())
      {
        send_to_others(i, "gk", 0, 0);
      }
      break;
    case 'x':
      sscanf(buffer.c_str(), "g x %d", &i);
      if (i == this_desk)
      {
        if (fromCanBus)
        {
          response_msg(my_desk.getDeskNumber(), "x", comm.getExternalLight());
        }
        else
        {
          // TODO: verificar se podemos usar o valor do getExternalLight()
          // float Lux = adc_to_lux(read_adc);
          // Lux = max(0, Lux - (volt_to_lux(my_desk.getDutyCycle() * dutyCycle_conv * my_desk.getGain())));
          Serial.printf("x %d %f\n", this_desk, comm.getExternalLight());
        }
      }
      else if (my_desk.getHub())
      {
        send_to_others(i, "gx", 0, 0);
      }
      break;
    case 'p':
      sscanf(buffer.c_str(), "g p %d", &i);
      if (i == this_desk)
      {
        float power = my_desk.getPmax() * my_desk.getDutyCycle() / 100.0;
        if (fromCanBus)
        {
          response_msg(my_desk.getDeskNumber(), "p", power);
        }
        else
        {
          Serial.printf("p %d %f\n", this_desk, power);
        }
      }
      else if (my_desk.getHub())
      {
        send_to_others(i, "gp", 0, 0);
      }
      break;
    case 't':
      sscanf(buffer.c_str(), "g t %d", &i);
      if (i == this_desk)
      {
        unsigned long final_time = millis();
        if (fromCanBus)
        {
          response_msg(my_desk.getDeskNumber(), "t", final_time / 1000);
        }
        else
        {
          Serial.printf("t %d %ld\n", this_desk, final_time / 1000);
        }
      }
      else if (my_desk.getHub())
      {
        send_to_others(i, "gt", 0, 0);
      }
      break;
    case 'e':
      sscanf(buffer.c_str(), "g e %d", &i);
      if (i == this_desk)
      {
        if (fromCanBus)
        {
          response_msg(my_desk.getDeskNumber(), "e", my_desk.getEnergyAvg());
        }
        else
        {
          Serial.printf("e %d %f\n", this_desk, my_desk.getEnergyAvg());
        }
      }
      else if (my_desk.getHub())
      {
        send_to_others(i, "ge", 0, 0);
      }
      break;
    case 'v':
      sscanf(buffer.c_str(), "g v %d", &i);
      if (i == this_desk)
      {
        if (fromCanBus)
        {
          response_msg(my_desk.getDeskNumber(), "v", my_desk.getVisibilityErr());
        }
        else
        {
          Serial.printf("v %d %f\n", this_desk, my_desk.getVisibilityErr());
        }
      }
      else if (my_desk.getHub())
      {
        send_to_others(i, "gv", 0, 0);
      }
      break;
    case 'f':
      sscanf(buffer.c_str(), "g f %d", &i);
      if (i == this_desk)
      {
        if (fromCanBus)
        {
          response_msg(my_desk.getDeskNumber(), "f", my_desk.getFlickerErr());
        }
        else
        {
          Serial.printf("f %d %f\n", this_desk, my_desk.getFlickerErr());
        }
      }
      else if (my_desk.getHub())
      {
        send_to_others(i, "gf", 0, 0);
      }
      break;
    case 'O':
      sscanf(buffer.c_str(), "g O %d", &i);
      if (i == this_desk)
      {
        if (fromCanBus)
        {
          response_msg(my_desk.getDeskNumber(), "O", node.getLowerBoundOccupied());
        }
        else
        {
          Serial.printf("O %d %f\n", this_desk, node.getLowerBoundOccupied());
        }
      }
      else if (my_desk.getHub())
      {
        send_to_others(i, "gO", 0, 0);
      }
      break;
    case 'U':
      sscanf(buffer.c_str(), "g U %d", &i);
      if (i == this_desk)
      {
        if (fromCanBus)
        {
          response_msg(my_desk.getDeskNumber(), "U", node.getLowerBoundUnoccupied());
        }
        else
        {
          Serial.printf("U %d %f\n", this_desk, node.getLowerBoundUnoccupied());
        }
      }
      else if (my_desk.getHub())
      {
        send_to_others(i, "gU", 0, 0);
      }
      break;
    case 'L':
      sscanf(buffer.c_str(), "g L %d", &i);
      if (i == this_desk)
      {
        if (fromCanBus)
        {
          response_msg(my_desk.getDeskNumber(), "L", node.getCurrentLowerBound());
        }
        else
        {
          Serial.printf("L %d %f\n", this_desk, node.getCurrentLowerBound());
        }
      }
      else if (my_desk.getHub())
      {
        send_to_others(i, "gL", 0, 0);
      }
      break;
    case 'c':
      sscanf(buffer.c_str(), "g c %d", &i);
      if (i == this_desk)
      {
        if (fromCanBus)
        {
          response_msg(my_desk.getDeskNumber(), "c", node.getCost());
        }
        else
        {
          Serial.printf("c %d %f\n", this_desk, node.getCost());
        }
      }
      else if (my_desk.getHub())
      {
        send_to_others(i, "gc", 0, 0);
      }
      break;
    case 'b': // TODO: implementar last minute buffer stream
      char x;
      sscanf(buffer.c_str(), "g b %c %d", &x, &i);
      if (i == this_desk)
      {
        if (x == 'l')
        {
          unsigned short head = my_desk.getIdxBuffer_l(i);
          Serial.printf("b l %d ", this_desk);
          // Se o buffer estiver cheio começar a partir dos valores mais antigos para os mais recentes
          if (my_desk.isBufferFull())
          {
            for (int j = head; j < buffer_size; j++)
            {
              Serial.printf("%f, ", my_desk.getLastMinuteBufferL(i, j));
            }
            for (int j = 0; j < head - 1; j++)
            {
              Serial.printf("%f, ", my_desk.getLastMinuteBufferL(i, j));
            }
          }
          // Se o buffer não estiver cheio ir até onde há dados
          else
          {
            for (int j = 0; j < head - 1; j++)
            {
              Serial.printf("%f, ", my_desk.getLastMinuteBufferL(i, j));
            }
          }
          Serial.printf("%f\n", my_desk.getLastMinuteBufferL(i, head - 1));
        }
        else if (x == 'd')
        {
          unsigned short head = my_desk.getIdxBuffer_d(i);
          Serial.printf("b d %d ", this_desk);
          // Se o buffer estiver cheio começar a partir dos valores mais antigos para os mais recentes
          if (my_desk.isBufferFull())
          {
            for (int j = head; j < buffer_size; j++)
            {
              Serial.printf("%f, ", my_desk.getLastMinuteBufferD(i, j)); // tirar o \n e meter ,
            }
            for (int j = 0; j < head - 1; j++)
            {
              Serial.printf("%f, ", my_desk.getLastMinuteBufferD(i, j)); // tirar o \n
            }
          }
          // Se o buffer não estiver cheio ir até onde há dados
          else
          {
            for (int j = 0; j < head - 1; j++)
            {
              Serial.printf("%f, ", my_desk.getLastMinuteBufferD(i, j)); // tirar o \n
            }
          }
          Serial.printf("%f\n", my_desk.getLastMinuteBufferD(i, head - 1));
        }
        else if (my_desk.getHub())
        {
          Serial.println("err");
        }
        else
        {
          send_ack_err(0);
        }
      }
      else
      {
        String result;
        result.concat("bg"); // Add "g b"
        result.concat(x);    // Add 'd' or 'l'

        send_to_others(i, result, 0, 2);
      }
      break;
    default:
      if (my_desk.getHub())
      {
        Serial.println("err");
      }
      else
      {
        send_ack_err(0);
      }
      break;
    }
    break;
  default:
    if (my_desk.getHub())
    {
      Serial.println("err");
    }
    else
    {
      send_ack_err(0);
    }
    break;
  }

  if (buffer.startsWith("F"))
  {
    Serial.printf("Desk Number: %d\n", my_desk.getDeskNumber());
    Serial.printf("Conectadas a mim:");
    for (const int &elem : comm.getDesksConnected())
    {
      Serial.printf(" %d,", elem);
    }
    Serial.printf("\nTime to connect -> %d\n", comm.getTimeToConnect());

    if (comm.getIsCalibrated())
    {
      for (int i = 1; i <= comm.getNumDesks(); i++)
      {
        Serial.printf("Desk %d -> %f\n", i, comm.getCouplingGain(i - 1));
      }
      Serial.printf("external light: %f\n", comm.getExternalLight());
    }
  }
  return;
}

void real_time_stream_of_data(unsigned long time, float lux)
{
  int this_desk = my_desk.getDeskNumber();
  if (my_desk.getHub())
  {
    if (my_desk.isLuxFlag())
    {
      Serial.printf("s l %d %f %ld\n", this_desk, lux, time);
    }
    if (my_desk.isDutyFlag())
    {
      Serial.printf("s d %d %f %ld \n", this_desk, my_desk.getDutyCycle(), time);
    }
  }
  else
  {
    if (my_desk.isLuxFlag())
    {
      send_stream(0, time, lux);
    }
    if (my_desk.isDutyFlag())
    {
      send_stream(1, time, 0);
    }
  }

  if (debbuging)
  {
    Serial.printf("0 40 %f %f %f\n", lux, my_desk.getRef(), my_desk.getDutyCycle());
  }
}

void send_stream(int type, unsigned long time, float lux) // type 0 -> lux, type 1 -> duty
{
  struct can_frame canMsgTx;
  int msg;
  canMsgTx.can_id = 1;
  canMsgTx.can_dlc = 8;
  if (type == 0)
  {
    canMsgTx.data[0] = 'l';
    canMsgTx.data[1] = comm.int_to_char_msg(my_desk.getDeskNumber());
    msg = static_cast<int>(lux * 100);
    canMsgTx.data[2] = static_cast<unsigned char>(msg & 255); // Same as msg0 % 256, but more efficient
    canMsgTx.data[3] = static_cast<unsigned char>(msg >> 8);  // Same as msg0 / 256, but more efficient
  }
  else
  {
    canMsgTx.data[0] = 'd';
    canMsgTx.data[1] = comm.int_to_char_msg(my_desk.getDeskNumber());
    msg = static_cast<int>(my_desk.getDutyCycle() * 100);
    canMsgTx.data[2] = static_cast<unsigned char>(msg & 255); // Same as msg0 % 256, but more efficient
    canMsgTx.data[3] = static_cast<unsigned char>(msg >> 8);  // Same as msg0 / 256, but more efficient
  }
  // TIME
  unsigned int time_aux = static_cast<unsigned int>(time);
  memcpy(&canMsgTx.data[4], &time_aux, sizeof(unsigned int));

  comm.sendMsg(&canMsgTx);
  if (comm.getError() != MCP2515::ERROR_OK)
  {
    Serial.printf("Error sending message \n");
  }
}

void send_to_all(char type) //
{
  struct can_frame canMsgTx;
  canMsgTx.can_id = my_desk.getDeskNumber(); // Replace this_desk with the correct value obtained from my_desk.getDeskNumber()
  canMsgTx.can_dlc = 8;
  canMsgTx.data[0] = type;
  canMsgTx.data[1] = ' ';
  canMsgTx.data[2] = comm.int_to_char_msg(0);
  for (int i = 3; i < 8; i++)
  {
    canMsgTx.data[i] = ' ';
  }
  comm.sendMsg(&canMsgTx);
  if (comm.getError() != MCP2515::ERROR_OK)
  {
    Serial.printf("Error sending message \n");
  }
}

void send_to_others(const int desk, const String &commands, const float value, int type)
{
  struct can_frame canMsgTx;
  if (type == 0) // to send get messages <char> <char>
  {
    canMsgTx.can_dlc = 2;
    canMsgTx.data[0] = commands.charAt(0);
    canMsgTx.data[1] = commands.charAt(1);
  }
  else if (type == 1) // to send set messages <char> <float>
  {
    canMsgTx.can_dlc = 5;
    canMsgTx.data[0] = commands.charAt(0);
    memcpy(&canMsgTx.data[1], &value, sizeof(float));
    // TODO: ao ler o valor do buffer, ler como float e passar para int nos casos necessários
  }
  else // to send get messages <char> <char> <char> only for "g b l" and "g b d"
  {
    canMsgTx.can_dlc = 3;
    canMsgTx.data[0] = commands.charAt(0);
    canMsgTx.data[1] = commands.charAt(1);
    canMsgTx.data[2] = commands.charAt(2);
  }
  canMsgTx.can_id = desk;

  comm.sendMsg(&canMsgTx);
  if (comm.getError() != MCP2515::ERROR_OK)
  {
    Serial.printf("Error sending message \n");
  }
}

void response_msg(const int desk, const String &commands, const float value)
{
  struct can_frame canMsgTx; // r 1 value
  canMsgTx.can_id = 1;
  canMsgTx.can_dlc = 8;

  canMsgTx.data[0] = commands.charAt(0);
  canMsgTx.data[1] = comm.int_to_char_msg(desk);
  memcpy(&canMsgTx.data[1], &value, sizeof(float));

  comm.sendMsg(&canMsgTx);
  if (comm.getError() != MCP2515::ERROR_OK)
  {
    Serial.printf("Error sending message \n");
  }
}