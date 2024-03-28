#include "command.h"

void read_command() {
  char command;
  int lumminaire;
  if (Serial.available() > 0) {
    command = Serial.read();
    Serial.read();
    switch (command) {
      case 'r':
        {
          float temp_reference;
          lumminaire = Serial.parseInt();
          if (lumminaire != my_desk.getDeskNumber()) {
            clean_buffer();
            break;
          }
          Serial.read();
          temp_reference = Serial.parseFloat();
          if (Serial.read() != '\n') {
            Serial.println("err");
            break;
          }
          ref_change(temp_reference);
          Serial.println("ack");
        }
        break;
      case 'd':
        {
          float DutyC;
          lumminaire = Serial.parseInt();
          if (lumminaire != my_desk.getDeskNumber()) {
            clean_buffer();
            break;
          }
          Serial.read();
          DutyC = Serial.parseFloat();
          if (Serial.read() != '\n' || DutyC > 100 || DutyC < 0) {
            Serial.println("err");
            break;
          }
          my_desk.setDutyCycle(DutyC);
          analogWrite(LED_PIN, DutyC * dutyCycle_conv);
          Serial.println("ack");
          my_desk.setIgnoreReference(true);
          my_desk.setON(true);
        }
        break;
      case 'o':
        {
          int temp_occupied;
          lumminaire = Serial.parseInt();
          if (lumminaire != my_desk.getDeskNumber()) {
            clean_buffer();
            break;
          }
          Serial.read();
          temp_occupied = Serial.parseInt();
          if (Serial.read() != '\n' || (temp_occupied != 1 && temp_occupied != 0 && temp_occupied != 2)) {
            Serial.println("err");
            break;
          }
          if (temp_occupied == 2) {
            my_desk.setON(false);
            analogWrite(LED_PIN, 0);
          } else {
            my_desk.setOccupied(temp_occupied);
            my_desk.setDutyCycle(0);
            temp_occupied == 0 ? ref_change(my_desk.getRefUnoccupied()) : ref_change(my_desk.getRefOccupied());
          }
          Serial.println("ack");
        }
        break;
      case 'a':
        {
          int temp_antiwindup;
          lumminaire = Serial.parseInt();
          if (lumminaire != my_desk.getDeskNumber()) {
            clean_buffer();
            break;
          }
          Serial.read();
          temp_antiwindup = Serial.parseInt();
          if (Serial.read() != '\n' || (temp_antiwindup != 1 && temp_antiwindup != 0)) {
            Serial.println("err");
            break;
          }
          my_pid.set_antiwindup(temp_antiwindup);
          Serial.println("ack");
        }
        break;
      case 'k':
        {
          int temp_feedback;
          lumminaire = Serial.parseInt();
          if (lumminaire != my_desk.getDeskNumber()) {
            clean_buffer();
            break;
          }
          Serial.read();
          temp_feedback = Serial.parseInt();
          if (Serial.read() != '\n' || (temp_feedback != 1 && temp_feedback != 0)) {
            Serial.println("err");
            break;
          }
          my_pid.set_feedback(temp_feedback);
          Serial.println("ack");
        }
        break;
      case 's':
        {
          char flag = Serial.read();
          Serial.read();
          lumminaire = Serial.parseInt();
          if (lumminaire != my_desk.getDeskNumber()) {
            clean_buffer();
            break;
          }
          if (Serial.read() != '\n') {
            Serial.println("err");
            return;
          }
          switch (flag) {
            case 'l':
              my_desk.setLuxFlag(true);
              break;
            case 'd':
              my_desk.setDutyFlag(true);
              break;
            default:
              Serial.println("err");
          }
        }
        break;
      case 'S':
        {
          char flag = Serial.read();
          Serial.read();
          lumminaire = Serial.parseInt();
          if (lumminaire != my_desk.getDeskNumber()) {
            clean_buffer();
            break;
          }
          if (Serial.read() != '\n') {
            Serial.println("err");
            return;
          }
          switch (flag) {
            case 'l':
              my_desk.setLuxFlag(false);
              break;
            case 'd':
              my_desk.setDutyFlag(false);
              break;
            default:
              Serial.println("err");
          }
        }
        break;
      case 'D':
        {
          lumminaire = Serial.parseInt();
          if (lumminaire != my_desk.getDeskNumber()) {
            clean_buffer();
            break;
          }
          Serial.read();
          debbuging == true ? debbuging = false : debbuging = true;
        }
        break;
      case 'K':
        {
          float K;
          lumminaire = Serial.parseInt();
          if (lumminaire != my_desk.getDeskNumber()) {
            clean_buffer();
            break;
          }
          Serial.read();
          K = Serial.parseFloat();
          if (Serial.read() != '\n') {
            Serial.println("err");
            break;
          }
          my_pid.set_k(K);
          my_pid.set_b(lux_to_volt(ref) / ref, my_desk.getGain());
          Serial.println("ack");
        }
        break;
      case 'b':
        {
          float b;
          lumminaire = Serial.parseInt();
          if (lumminaire != my_desk.getDeskNumber()) {
            clean_buffer();
            break;
          }
          Serial.read();
          b = Serial.parseFloat();
          if (Serial.read() != '\n') {
            Serial.println("err");
            break;
          }
          my_pid.set_b(b);
          Serial.println("ack");
        }
        break;

      case 'l':
        {
          int temp_bump;
          lumminaire = Serial.parseInt();
          if (lumminaire != my_desk.getDeskNumber()) {
            clean_buffer();
            break;
          }
          Serial.read();
          temp_bump = Serial.parseInt();
          if (Serial.read() != '\n' || (temp_bump != 1 && temp_bump != 0)) {
            Serial.println("err");
            break;
          }
          my_pid.set_bumpless(temp_bump);
          Serial.println("ack");
        }
        break;
      //apenas para teste
      case 't':
        {
          float t;
          lumminaire = Serial.parseInt();
          if (lumminaire != my_desk.getDeskNumber()) {
            clean_buffer();
            break;
          }
          Serial.read();
          t = Serial.parseFloat();
          if (Serial.read() != '\n') {
            Serial.println("err");
            break;
          }
          my_pid.set_Tt(t);
          Serial.println("ack");
        }
        break;
      //apenas para teste
      case 'i':
        {
          float i;
          lumminaire = Serial.parseInt();
          if (lumminaire != my_desk.getDeskNumber()) {
            clean_buffer();
            break;
          }
          Serial.read();
          i = Serial.parseFloat();
          if (Serial.read() != '\n') {
            Serial.println("err");
            break;
          }
          my_pid.set_Ti(i);
          Serial.println("ack");
        }
        break;
      case 'g':
        command = Serial.read();
        Serial.read();
        if (command != 'b') {
          lumminaire = Serial.parseInt();
          if (lumminaire != my_desk.getDeskNumber()) {
            clean_buffer();
            break;
          }
          Serial.read();
        }
        switch (command) {
          case 'd':
            Serial.printf("d %d %f\n", lumminaire, my_desk.getDutyCycle());
            break;
          case 'l':
            {
              int total_adc, read_adc, j;
              float Lux;
              for (j = 0, total_adc = 0; j < 20; j += 1) {
                read_adc = analogRead(A0);
                total_adc += read_adc;
              }
              read_adc = total_adc / 20.0;
              Lux = adc_to_lux(read_adc);
              Serial.printf("l %d %f\n", lumminaire, Lux);
            }
            break;
          case 'r':
            Serial.printf("r %d %f\n", lumminaire, ref);
            break;
          case 'o':
            Serial.printf("o %d %d\n", lumminaire, my_desk.isOccupied());
            break;
          case 'a':
            Serial.printf("a %d %d\n", lumminaire, my_pid.get_antiwindup());
            break;
          case 'k':
            Serial.printf("k %d %d\n", lumminaire, my_pid.get_feedback());
            break;
          case 'x':
            {
              float Lux = adc_to_lux(read_adc);
              Lux = max(0, Lux - (volt_to_lux(my_desk.getDutyCycle() * dutyCycle_conv * my_desk.getGain())));
              Serial.printf("x %d %f\n", lumminaire, Lux);
            }
            break;
          case 'p':
            {
              float power = my_desk.getPmax() * my_desk.getDutyCycle() / 100.0;
              Serial.printf("p %d %f\n", lumminaire, power);
            }
            break;
          case 't':
            {
              unsigned long final_time = millis();
              Serial.printf("t %d %ld\n", lumminaire, final_time / 1000);
            }
            break;
          case 'b':
            {
              char flag = Serial.read();
              unsigned short head = my_desk.getIdxBuffer();
              unsigned short i;
              Serial.read();
              lumminaire = Serial.parseInt();
              if (lumminaire != my_desk.getDeskNumber()) {
                clean_buffer();
                break;
              }
              if (Serial.read() != '\n') {
                Serial.println("err");
                return;
              }
              switch (flag) {
                case 'l':
                  Serial.printf("b l %d ", lumminaire);
                  // Se o buffer estiver cheio começar a partir dos valores mais antigos para os mais recentes
                  if (my_desk.isBufferFull()) {
                    for (i = head; i < buffer_size; i++) {
                      Serial.printf("%f, ", my_desk.getLastMinuteBufferL(i));
                    }
                    for (i = 0; i < head - 1; i++) {
                      Serial.printf("%f, ", my_desk.getLastMinuteBufferL(i));
                    }
                  }
                  // Se o buffer não estiver cheio ir até onde há dados
                  else {
                    for (i = 0; i < head - 1; i++) {
                      Serial.printf("%f, ", my_desk.getLastMinuteBufferL(i));
                    }
                  }
                  Serial.printf("%f\n", my_desk.getLastMinuteBufferL(head - 1));
                  break;
                case 'd':
                  Serial.printf("b d %d ", lumminaire);
                  // Se o buffer estiver cheio começar a partir dos valores mais antigos para os mais recentes
                  if (my_desk.isBufferFull()) {
                    for (i = head; i < buffer_size; i++) {
                      Serial.printf("%f, ", my_desk.getLastMinuteBufferD(i));  //tirar o \n e meter ,
                    }
                    for (i = 0; i < head - 1; i++) {
                      Serial.printf("%f, ", my_desk.getLastMinuteBufferD(i));  //tirar o \n
                    }
                  }
                  // Se o buffer não estiver cheio ir até onde há dados
                  else {
                    for (i = 0; i < head - 1; i++) {
                      Serial.printf("%f, ", my_desk.getLastMinuteBufferD(i));  //tirar o \n
                    }
                  }
                  Serial.printf("%f\n", my_desk.getLastMinuteBufferD(head - 1));
                  break;
                default:
                  Serial.println("err");
              }
            }
            break;
          case 'e':
            Serial.printf("e %d %f\n", lumminaire, my_desk.getEnergyAvg());
            break;
          case 'v':
            Serial.printf("v %d %f\n", lumminaire, my_desk.getVisibilityErr());
            break;
          case 'f':
            Serial.printf("f %d %f\n", lumminaire, my_desk.getFlickerErr());
            break;
          case 'i':
            Serial.printf("i %d - h = %f, K = %f, b = %f, Ti = %f, Tt = %f, Td = %f, N = %f\n", lumminaire,
                          my_pid.get_h(), my_pid.get_k(), my_pid.get_b(), my_pid.get_Ti(), my_pid.get_Tt(), my_pid.get_Td(), my_pid.get_N());
            break;
          case 'g':
            Serial.printf("g %d %f\n", lumminaire, my_desk.getGain());
            break;
          //apenas para gráficos
          case 'y':
            {
              unsigned short head = my_desk.getIdxBuffer();
              unsigned short i;
              Serial.printf("b l %d ", lumminaire);
              // Se o buffer estiver cheio começar a partir dos valores mais antigos para os mais recentes
              if (my_desk.isBufferFull()) {
                for (i = head; i < buffer_size; i++) {
                  Serial.printf("%f\n ", my_desk.getLastMinuteBufferL(i));
                }
                for (i = 0; i < head - 1; i++) {
                  Serial.printf("%f\n ", my_desk.getLastMinuteBufferL(i));
                }
              }
              // Se o buffer não estiver cheio ir até onde há dados
              else {
                for (i = 0; i < head - 1; i++) {
                  Serial.printf("%f\n ", my_desk.getLastMinuteBufferL(i));
                }
              }
              Serial.printf("%f\n", my_desk.getLastMinuteBufferL(head - 1));
            }
            break;
            //apenas para gráficos
          case 'z':
            {
              unsigned short head = my_desk.getIdxBuffer();
              unsigned short i;
              Serial.printf("b d %d ", lumminaire);
              // Se o buffer estiver cheio começar a partir dos valores mais antigos para os mais recentes
              if (my_desk.isBufferFull()) {
                for (i = head; i < buffer_size; i++) {
                  Serial.printf("%f\n ", my_desk.getLastMinuteBufferD(i));  //tirar o \n e meter ,
                }
                for (i = 0; i < head - 1; i++) {
                  Serial.printf("%f\n ", my_desk.getLastMinuteBufferD(i));  //tirar o \n
                }
              }
              // Se o buffer não estiver cheio ir até onde há dados
              else {
                for (i = 0; i < head - 1; i++) {
                  Serial.printf("%f\n ", my_desk.getLastMinuteBufferD(i));  //tirar o \n
                }
              }
              Serial.printf("%f\n", my_desk.getLastMinuteBufferD(head - 1));
            }
            break;
          default:
            Serial.println("err");
            clean_buffer();
        }
        break;

      default:
        Serial.println("err");
        clean_buffer();
    }
    return;
  }
}

void clean_buffer() {
  while (Serial.available() > 0) {
    Serial.read();
  }
}

void real_time_stream_of_data(unsigned long time, float lux) {
  if (my_desk.isLuxFlag()) {

    Serial.printf("s l %d %f %ld\n", my_desk.getDeskNumber(), lux, time);
  }
  if (my_desk.isDutyFlag()) {
    Serial.printf("s d %d %f %ld \n", my_desk.getDeskNumber(), my_desk.getDutyCycle(), time);
  }
  if (debbuging) {
    Serial.printf("0 40 %f %f %f\n", lux, ref, my_desk.getDutyCycle());
  }
}
