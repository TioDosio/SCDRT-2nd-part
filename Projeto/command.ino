#include "command.h"
#include "Communication.h"

void read_command()
{

  if (Serial.available() > 0)
  {
    String command = Serial.readStringUntil('\n');
    int i;
    float val;
    int this_desk = my_desk.getDeskNumber();

    if (command.startsWith("d ")) // Set directly the duty cycle of luminaire i.
    {
      sscanf(command.c_str(), "d %d %f", &i, &val);
      if (i == this_desk)
      {
        if (val >= 0 and val <= 100)
        {
          my_desk.setDutyCycle(val);
          analogWrite(LED_PIN, val * dutyCycle_conv);
          Serial.println("ack");
          my_desk.setIgnoreReference(true);
          my_desk.setON(true);
        }
        else
        {
          Serial.println("err");
        }
      }
      else
      {
        // NOT THIS DESK
      }
    }
    else if (command.startsWith("g d ")) // Get current duty cycle of luminaire i
    {
      sscanf(command.c_str(), "g d %d", &i);
      if (i == this_desk)
      {
        Serial.printf("d %d %f\n", i, my_desk.getDutyCycle());
      }
      else
      {
        // NOT THIS DESK
      }
    }
    else if (command.startsWith("r ")) // Set the illuminance reference of luminaire i
    {
      sscanf(command.c_str(), "r %d %f", &i, &val);
      if (i == this_desk)
      {
        if (val >= 0)
        {
          ref_change(val);
          Serial.println("ack");
        }
        else
        {
          Serial.println("err");
        }
      }
      else
      {
        // NOT THIS DESK
      }
    }
    else if (command.startsWith("g r ")) // Get current illuminance reference of luminaire i
    {
      sscanf(command.c_str(), "g r %d", &i);
      if (i == this_desk)
      {
        Serial.printf("r %d %f\n", i, my_desk.getRef());
      }
      else
      {
        // NOT THIS DESK
      }
    }
    else if (command.startsWith("g l ")) // Measure the illuminance of luminaire i
    {
      sscanf(command.c_str(), "g l %d", &i);
      if (i == this_desk)
      {
        int total_adc, read_adc, j;
        float Lux;
        for (j = 0, total_adc = 0; j < 20; j += 1)
        {
          read_adc = analogRead(A0);
          total_adc += read_adc;
        }
        read_adc = total_adc / 20.0;
        Lux = adc_to_lux(read_adc);
        Serial.printf("l %d %f\n", this_desk, Lux);
      }
      else
      {
        // NOT THIS DESK
      }
    }
    else if (command.startsWith("o ")) // Set the current occupancy state of desk <i>
    {
      sscanf(command.c_str(), "o %d %f", &i, &val);
      if (i == this_desk)
      {
        if (val == 0 or val == 1)
        {
          my_desk.setOccupied(val);
          my_desk.setDutyCycle(0);
          val == 0 ? ref_change(my_desk.getRefUnoccupied()) : ref_change(my_desk.getRefOccupied());
        }
        else if (val == 2)
        {
          my_desk.setON(false);
          analogWrite(LED_PIN, 0);
        }
        else
        {
          Serial.println("err");
        }
      }
      else
      {
        // NOT THIS DESK
      }
    }
    else if (command.startsWith("g o ")) // Get the current occupancy state of desk <i>
    {
      sscanf(command.c_str(), "g o %d", &i);
      if (i == this_desk)
      {
        Serial.printf("o %d %d\n", i, my_desk.isOccupied());
      }
      else
      {
        // NOT THIS DESK
      }
    }
    else if (command.startsWith("a ")) // Set anti-windup state of desk <i>
    {
      sscanf(command.c_str(), "a %d %f", &i, &val);

      if (i == this_desk)
      {
        if (val == 0 or val == 1)
        {
          my_pid.set_antiwindup(val);
          Serial.println("ack");
        }
        else
        {
          Serial.println("err");
        }
      }
      else
      {
        // NOT THIS DESK
      }
    }
    else if (command.startsWith("g a ")) // Get anti-windup state of desk <i>
    {
      sscanf(command.c_str(), "g a %d", &i);
      if (i == this_desk)
      {
        Serial.printf("a %d %d\n", i, my_pid.get_antiwindup());
      }
      else
      {
        // NOT THIS DESK
      }
    }
    else if (command.startsWith("k ")) // Set feedback on/off of desk <i>
    {
      sscanf(command.c_str(), "k %d %f", &i, &val);
      if (i == this_desk)
      {
        if (val == 0 or val == 1)
        {
          my_pid.set_feedback(val);
          Serial.println("ack");
        }
        else
        {
          Serial.println("err");
        }
      }
      else
      {
        // NOT THIS DESK
      }
    }
    else if (command.startsWith("g k ")) // Get feedback state of desk <i>
    {
      sscanf(command.c_str(), "g k %d", &i);
      if (i == this_desk)
      {
        Serial.printf("k %d %d\n", i, my_pid.get_feedback());
      }
      else
      {
        // NOT THIS DESK
      }
    }
    else if (command.startsWith("g x ")) // Get current external illuminance of desk <i>
    {
      sscanf(command.c_str(), "g x %d", &i);
      if (i == this_desk)
      {
        float Lux = adc_to_lux(read_adc);
        Lux = max(0, Lux - (volt_to_lux(my_desk.getDutyCycle() * dutyCycle_conv * my_desk.getGain())));
        Serial.printf("x %d %f\n", this_desk, Lux);
      }
      else
      {
        // NOT THIS DESK
      }
    }
    else if (command.startsWith("g p ")) // Get instantaneous power consumption of desk <i>
    {
      sscanf(command.c_str(), "g p %d", &i);
      if (i == this_desk)
      {
        float power = my_desk.getPmax() * my_desk.getDutyCycle() / 100.0;
        Serial.printf("p %d %f\n", this_desk, power);
      }
      else
      {
        // NOT THIS DESK
      }
    }
    else if (command.startsWith("g t ")) // Get the elapsed time since the last restart
    {
      sscanf(command.c_str(), "g t %d", &i);
      if (i == this_desk)
      {
        unsigned long final_time = millis();
        Serial.printf("t %d %ld\n", this_desk, final_time / 1000);
      }
      else
      {
        // NOT THIS DESK
      }
    }
    else if (command.startsWith("s ")) // Start the stream of the real-time variable <x> of desk <i>. <x> can be 'l' or 'd'.
    {
      char x;
      sscanf(command.c_str(), "s %c %d", &x, &i);
      if (i == this_desk)
      {
        if (x == 'l')
        {
          my_desk.setLuxFlag(true);
          Serial.println("ack");
        }
        else if (x == 'd')
        {
          Serial.println("ack");
          my_desk.setDutyFlag(true);
        }
        else
        {
          Serial.println("err");
        }
      }
      else
      {
        // NOT THIS DESK
      }
    }
    else if (command.startsWith("S ")) // Stop the stream of the real-time variable <x> of desk <i>. <x> can be 'l' or 'd'.
    {
      char x;
      sscanf(command.c_str(), "S %c %d", &x, &i);
      if (i == this_desk)
      {
        if (x == 'l')
        {
          my_desk.setLuxFlag(false);
          Serial.println("ack");
        }
        else if (x == 'd')
        {
          my_desk.setDutyFlag(false);
          Serial.println("ack");
        }
        else
        {
          Serial.println("err");
        }
      }
      else
      {
        // NOT THIS DESK
      }
    }
    else if (command.startsWith("g b ")) // Get the last minute buffer of the variable <x> of the desk <i>. <x> can be 'l' or 'd'.
    {
      char x;
      sscanf(command.c_str(), "g b %c %d", &x, &i);
      if (i == this_desk)
      {
        unsigned short head = my_desk.getIdxBuffer();
        if (x == 'l')
        {
          Serial.printf("b l %d ", this_desk);
          // Se o buffer estiver cheio começar a partir dos valores mais antigos para os mais recentes
          if (my_desk.isBufferFull())
          {
            for (i = head; i < buffer_size; i++)
            {
              Serial.printf("%f, ", my_desk.getLastMinuteBufferL(i));
            }
            for (i = 0; i < head - 1; i++)
            {
              Serial.printf("%f, ", my_desk.getLastMinuteBufferL(i));
            }
          }
          // Se o buffer não estiver cheio ir até onde há dados
          else
          {
            for (i = 0; i < head - 1; i++)
            {
              Serial.printf("%f, ", my_desk.getLastMinuteBufferL(i));
            }
          }
          Serial.printf("%f\n", my_desk.getLastMinuteBufferL(head - 1));
        }
        else if (x == 'd')
        {
          Serial.printf("b d %d ", this_desk);
          // Se o buffer estiver cheio começar a partir dos valores mais antigos para os mais recentes
          if (my_desk.isBufferFull())
          {
            for (i = head; i < buffer_size; i++)
            {
              Serial.printf("%f, ", my_desk.getLastMinuteBufferD(i)); // tirar o \n e meter ,
            }
            for (i = 0; i < head - 1; i++)
            {
              Serial.printf("%f, ", my_desk.getLastMinuteBufferD(i)); // tirar o \n
            }
          }
          // Se o buffer não estiver cheio ir até onde há dados
          else
          {
            for (i = 0; i < head - 1; i++)
            {
              Serial.printf("%f, ", my_desk.getLastMinuteBufferD(i)); // tirar o \n
            }
          }
          Serial.printf("%f\n", my_desk.getLastMinuteBufferD(head - 1));
        }
        else
        {
          Serial.println("err");
        }
      }
      else
      {
        // NOT THIS DESK
      }
    }
    else if (command.startsWith("g e ")) // Get the average energy consumption at the desk <i> since the last system restart.
    {
      sscanf(command.c_str(), "g e %d", &i);
      if (i == this_desk)
      {
        Serial.printf("e %d %f\n", this_desk, my_desk.getEnergyAvg());
      }
      else
      {
        // NOT THIS DESK
      }
    }
    else if (command.startsWith("g v ")) // Get the average visibility error at desk <i> since the last system restart.
    {
      sscanf(command.c_str(), "g v %d", &i);
      if (i == this_desk)
      {
        Serial.printf("v %d %f\n", this_desk, my_desk.getVisibilityErr());
      }
      else
      {
        // NOT THIS DESK
      }
    }
    else if (command.startsWith("g f ")) // Get the average flicker error on desk <i> since the last system restart.
    {
      sscanf(command.c_str(), "g f %d", &i);
      if (i == this_desk)
      {
        Serial.printf("f %d %f\n", this_desk, my_desk.getFlickerErr());
      }
      else
      {
        // NOT THIS DESK
      }
    }
    else if (command.startsWith("O "))
    {
      sscanf(command.c_str(), "O %d %d", &i, &val);
      if (i == this_desk)
      {
        if (val > 0)
        {
          // SET LOWER BOUND OCCUPIED
        }
        else
        {
          Serial.println("err");
          // NOT THIS DESK // Command not recognized
        }
        return;
      }
    }
    else if (command.startsWith("g O"))
    {
      sscanf(command.c_str(), "g O %d", &i);
      if (i == this_desk)
      {
        // GET LOWER BOUND OCCUPIED
      }
      else
      {
        // NOT THIS DESK
      }
    }
    else if (command.startsWith("U "))
    {
      sscanf(command.c_str(), "U %d %d", &i, &val);
      if (i == this_desk)
      {
        if (val > 0)
        {
          // SET LOWER BOUND UNOCCUPIED
        }
        else
        {
          Serial.println("err");
          // NOT THIS DESK // Command not recognized
        }
        return;
      }
    }
    else if (command.startsWith("g U"))
    {
      sscanf(command.c_str(), "g U %d", &i);
      if (i == this_desk)
      {
        // GET LOWER BOUND UNOCCUPIED
      }
      else
      {
        // NOT THIS DESK
      }
    }
    else if (command.startsWith("g L"))
    {
      sscanf(command.c_str(), "g L %d", &i);
      if (i == this_desk)
      {
        // GET CURRENT LOWER BOUND
      }
      else
      {
        // NOT THIS DESK
      }
    }
    else if (command.startsWith("c "))
    {
      sscanf(command.c_str(), "c %d %d", &i, &val);
      if (i == this_desk)
      {
        // GET CURRENT LOWER BOUND
      }
      else
      {
        // NOT THIS DESK
      }
    }
    else if (command.startsWith("g c"))
    {
      sscanf(command.c_str(), "g c %d", &i);
      if (i == this_desk)
      {
        // GET CURRENT LOWER BOUND
      }
      else
      {
        // NOT THIS DESK
      }
    }
    else if (command.startsWith("r"))
    {
      // RESET and recalibrate the system
    }
    else
    {
      // ERROR
    }
    return;
  }
}

void real_time_stream_of_data(unsigned long time, float lux)
{
  int this_desk = my_desk.getDeskNumber();
  if (my_desk.isLuxFlag())
  {

    Serial.printf("s l %d %f %ld\n", this_desk, lux, time);
  }
  if (my_desk.isDutyFlag())
  {
    Serial.printf("s d %d %f %ld \n", this_desk, my_desk.getDutyCycle(), time);
  }
  if (debbuging)
  {
    Serial.printf("0 40 %f %f %f\n", lux, my_desk.getRef(), my_desk.getDutyCycle());
  }
}

/*void send_to_others(const char *commands, int this_desk, int size)
{
  char msg[8] = {};
  char aux;
  for (int i = 0; i < size; i++)
  {
    msg[i] = commands[i];
  }
  while (Serial.available() > 0 && i < 9)
  {
    aux = Serial.read();
    if (aux == '\0')
    {
      break;
    }
    msg[i] = aux;
    i++;
  }
  struct can_frame canMsgTx;
  canMsgTx.can_id = this_desk;
  canMsgTx.can_dlc = 8;
  for (int i = 0; i < 8; i++)
  {
    canMsgTx.data[i] = msg[i];
  }

  err = can0.sendMessage(&canMsgTx);
  if (err != MCP2515::ERROR_OK)
  {
    Serial.printf("Error sending message: %s\n", err);
  }
}*/