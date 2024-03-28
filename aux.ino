/*
 * Function to process the commands received from the serial port
 * @param command: the command received from the serial port
 */
void processCommand(const String &command, int this_desk)
{
    float val;
    char x;
    double time;

    if (command.startsWith("d ")) // Set directly the duty cycle of luminaire <i>
    {
        sscanf(command.c_str(), "d %d %f", &i, &val);
        if (i == this_desk && val >= 0)
        {
            //  DO SOMETHING
            Serial.println("ack");
        }
        else if (val > 0)
        {
            // SENT TO CAN-BUS
        }
        else
        {
            Serial.println("err");
        }
    }
    else if (command.startsWith("g d ")) // Get current duty cycle of luminaire <i>
    {
        sscanf(command.c_str(), "g d %d", &i);
        if (i == this_desk &&)
        {
            Serial.print("d ");
            Serial.print(this_desk); //  DO SOMETHING
            Serial.print(" ");
            Serial.println("DUTY CYCLE"); //  DO SOMETHING
        }
        else
        {
            // SENT TO CAN-BUS
        }
    }
    else if (command.startsWith("r ")) // Set the illuminance reference of luminaire <i>
    {
        sscanf(command.c_str(), "r %d %f", &i, &val);
        if (i == this_desk)
        {
            //  DO SOMETHING
            Serial.println("ack");
        }
        else
        {
            // SENT TO CAN-BUS
        }
    }
    else if (command.startsWith("g r ")) // Get current illuminance reference of luminaire <i>
    {
        sscanf(command.c_str(), "g r %d", &i);
        if (i == this_desk)
        {
            Serial.print("r ");
            Serial.print(this_desk);
            Serial.print(" ");
            Serial.println("Reference"); //  DO SOMETHING
        }
        else
        {
            // SENT TO CAN-BUS
        }
    }
    else if (command.startsWith("g l ")) // Measure the illuminance of luminaire <i>
    {
        sscanf(command.c_str(), "g l %d", &i);
        if (i == this_desk)
        {
            Serial.print("l ");
            Serial.print(i);
            Serial.print(" ");
            Serial.println("ILLUMINANCE"); //  DO SOMETHING
        }
        else
        {
            // SENT TO CAN-BUS
        }
    }
    else if (command.startsWith("o ")) // Set the current occupancy state of desk <i>
    {
        sscanf(command.c_str(), "o %d %f", &i, &val);
        if (i == this_desk)
        {
            occupied = val;
            Serial.println("ack");
        }
    }
    else if (command.startsWith("g o ")) // Get the current occupancy state of desk <i>
    {
        sscanf(command.c_str(), "g o %d", &i);
        if (i == this_desk)
        {
            Serial.print("o ");
            Serial.print(i);
            Serial.print(" ");
            Serial.println(occupied);
        }
    }
    else if (command.startsWith("a ")) // Set anti-windup state of desk <i>
    {
        sscanf(command.c_str(), "a %d %f", &i, &val);

        if (i == this_desk and (val == 0 or val == 1))
        {
            anti_windup = val;
            Serial.println("ack");
        }
    }
    else if (command.startsWith("g a ")) // Get anti-windup state of desk <i>
    {
        sscanf(command.c_str(), "g a %d", &i);
        if (i == this_desk)
        {
            Serial.print("a ");
            Serial.print(i);
            Serial.print(" ");
            Serial.println(anti_windup);
        }
    }
    else if (command.startsWith("k ")) // Set feedback on/off of desk <i>
    {
        sscanf(command.c_str(), "k %d %f", &i, &val);
        if (i == this_desk and (val == 0 or val == 1))
        {
            Serial.println("ack");
            feedback = val;
        }
    }
    else if (command.startsWith("g k ")) // Get feedback state of desk <i>
    {
        sscanf(command.c_str(), "g k %d", &i);
        if (i == this_desk)
        {
            Serial.print("k ");
            Serial.print(i);
            Serial.print(" ");
            Serial.println(feedback);
        }
    }
    else if (command.startsWith("g x ")) // Get current external illuminance of desk <i>
    {
        sscanf(command.c_str(), "g x %d", &i);
        if (i == this_desk)
        {
            float ext = y - ganho * duty_cycle;
            if (ext < 0)
            {
                ext = 0;
            }
            Serial.print("x ");
            Serial.print(i);
            Serial.print(" ");
            Serial.println(ext);
        }
    }
    else if (command.startsWith("g p ")) // Get instantaneous power consumption of desk <i>
    {
        if (i == this_desk)
        {
            Serial.print("p ");
            Serial.print(i);
            Serial.print(" ");
            Serial.println(imediate_power_cons());
        }
    }
    else if (command.startsWith("g t ")) // Get the elapsed time since the last restart
    {
        sscanf(command.c_str(), "g t %d", &i);
        if (i == this_desk)
        {
            Serial.print("t ");
            Serial.print(i);
            Serial.print(" ");
            Serial.println(time_since_restart());
        }
    }
    else if (command.startsWith("s ")) // Start the stream of the real-time variable <x> of desk <i>. <x> can be 'l' or 'd'.
    {
        sscanf(command.c_str(), "s %c %d", &x, &i);
        if (i == this_desk and x == 'l')
        {
            flag_stream_lux = i;
        }
        else if (i == this_desk and x == 'd')
        {
            flag_stream_duty_c = i;
        }
    }
    else if (command.startsWith("S ")) // Stop the stream of the real-time variable <x> of desk <i>. <x> can be 'l' or 'd'.
    {
        sscanf(command.c_str(), "S %c %d", &x, &i);
        if (i == this_desk and x == 'l')
        {
            flag_stream_lux = 0;
            Serial.println("ack");
        }
        else if (i == this_desk and x == 'd')
        {
            flag_stream_duty_c = 0;
            Serial.println("ack");
        }
    }
    else if (command.startsWith("g b ")) // Get the last minute buffer of the variable <x> of the desk <i>. <x> can be 'l' or 'd'.
    {
        sscanf(command.c_str(), "g b %c %d", &x, &i);
        if (i == this_desk and x == 'l')
        {
            Serial.print("b ");
            Serial.print(x);
            Serial.print(" ");
            Serial.print(i);
            Serial.print(" ");
            printBuffer('l');
        }
        else if (i == this_desk and x == 'd')
        {
            Serial.print("b ");
            Serial.print(x);
            Serial.print(" ");
            Serial.print(i);
            Serial.print(" ");
            printBuffer('d');
        }
    }
    else if (command.startsWith("g e ")) // Get the average energy consumption at the desk <i> since the last system restart.
    {
        sscanf(command.c_str(), "g e %d", &i);
        if (i == this_desk)
        {
            Serial.print("e ");
            Serial.print(i);
            Serial.print(" ");
            Serial.println(mean_energy);
        }
    }
    else if (command.startsWith("g v ")) // Get the average visibility error at desk <i> since the last system restart.
    {
        sscanf(command.c_str(), "g v %d", &i);

        Serial.print("v ");
        Serial.print(i);
        Serial.print(" ");
        Serial.println(mean_visibility);
    }
    else if (command.startsWith("g f ")) // Get the average flicker error on desk <i> since the last system restart.
    {
        sscanf(command.c_str(), "g f %d", &i);

        Serial.print("f ");
        Serial.print(i);
        Serial.print(" ");
        Serial.println(mean_flicker);
    }
    else if (command.startsWith("mudar "))
    {
        sscanf(command.c_str(), "mudar %f", &K);
        B = 1 / (K * ganho * (lux_volt(r, b, m, vcc) / r));
    }
    else if (command.startsWith("m "))
    {
        sscanf(command.c_str(), "m %f", &m);
    }
    else
    {
        Serial.println("err"); // Command not recognized
    }
}