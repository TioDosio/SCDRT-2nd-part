#include <string>

inline void communicationLoop()
{
    if (data_available)
    {
        data_available = false;
        if (!comm.isMissingAckEmpty()) // Check if there are any missing acks
        {
            comm.acknowledge_loop(&node);
        }
        else
        {
            while (comm.IsMsgAvailable()) // Check if something has been received
            {
                can_frame canMsgRx;
                comm.ReadMsg(&canMsgRx);
                if (canMsgRx.can_id == my_desk.getDeskNumber() || canMsgRx.can_id == 0) // Check if the message is for this desk (0 is for all the desks)
                {
                    comm.add_msg_queue(canMsgRx); // Put all the messages in the queue
                }
            }

            while (!comm.isQueueEmpty())
            {
                can_frame canMsgRx;
                canMsgRx = comm.get_msg_queue();
                if (canMsgRx.can_dlc == 2) // char char (gets)
                {
                    String command;
                    command.concat(canMsgRx.data[0]);
                    command.concat(" ");
                    command.concat(canMsgRx.data[1]);
                    command.concat(" ");
                    command.concat(canMsgRx.can_id);

                    read_command(command, 1);
                }
                else if (canMsgRx.can_dlc == 3) // char char char (g b d or g b l)
                {
                    String command;
                    command.concat(canMsgRx.data[0]);
                    command.concat(" ");
                    command.concat(canMsgRx.data[1]);
                    command.concat(" ");
                    command.concat(canMsgRx.data[2]);
                    command.concat(" ");
                    command.concat(canMsgRx.can_id);

                    read_command(command, 1);
                }
                else if (canMsgRx.can_dlc == 5) // char int float (sets)
                {
                    String command;
                    float value;
                    memcpy(&value, &canMsgRx.data[1], sizeof(float));

                    command.concat(canMsgRx.data[0]);
                    command.concat(" ");
                    if (canMsgRx.data[0] == 'o' || canMsgRx.data[0] == 'a' || canMsgRx.data[0] == 'k')
                    {
                        command.concat(static_cast<int>(value));
                    }
                    else
                    {
                        command.concat(value);
                    }
                    read_command(command, 1);
                }
                else
                {
                    Serial.printf("Message received:");
                    for (int i = 0; i < canMsgRx.can_dlc; i++)
                    {
                        Serial.printf(" %c", canMsgRx.data[i]);
                    }
                    Serial.println();
                    switch (canMsgRx.data[0])
                    {
                    case 'W':
                        comm.msg_received_connection(canMsgRx);
                        break;
                    case 'C':
                        comm.msg_received_calibration(canMsgRx);
                        break;
                    case 'T':
                    {
                        comm.ack_msg(canMsgRx);
                        Serial.printf("Received T from desk %d\n", comm.char_msg_to_int(canMsgRx.data[1]));
                        comm.delay_manual(3);
                        comm.send_consensus_data('1', &node, comm.char_msg_to_int(canMsgRx.data[1]));
                    }
                    break;
                    case '1':
                    {
                        Serial.printf("ENTrei no '1'\n");
                        comm.ack_msg(canMsgRx);
                        Serial.printf("Passei do ack do '1'\n");
                        int index = std::distance(comm.getDesksConnected().begin(), comm.getDesksConnected().find(canMsgRx.data[1]));
                        OtherLuminaires Lums{};
                        for (int i = 0; i < 3; i++)
                        {
                            Lums.setKIndex(0, (static_cast<int>(canMsgRx.data[2 * i + 2]) + (static_cast<int>(canMsgRx.data[2 * i + 3]) << 8)) / 1000.0);
                            Serial.printf("Desk: %d -> KIndex %d: %f\n", canMsgRx.can_id, i, Lums.getKIndex(i));
                            node.setLums(Lums, index);
                        }
                    }
                    break;
                    case '2':
                    {
                        comm.ack_msg(canMsgRx);
                        int index = std::distance(comm.getDesksConnected().begin(), comm.getDesksConnected().find(canMsgRx.data[1]));
                        OtherLuminaires Lums = node.getLums(index);
                        Lums.setO((static_cast<int>(canMsgRx.data[2]) + (static_cast<int>(canMsgRx.data[3]) << 8)) / 1000.0);
                        Lums.setC((static_cast<int>(canMsgRx.data[4]) + (static_cast<int>(canMsgRx.data[5]) << 8)) / 100.0);
                        Lums.setL((static_cast<int>(canMsgRx.data[6]) + (static_cast<int>(canMsgRx.data[7]) << 8)) / 100.0);
                        if (node.receivedAllLums())
                        {
                            double newLuminance[3];
                            mainConsensus(newLuminance);
                            comm.consensus_msg_lux(newLuminance);
                        }
                    }
                    break;
                    case 'E':
                    {
                        int i = my_desk.getDeskNumber() - 1;
                        double l = (static_cast<int>(canMsgRx.data[2 * i + 1]) + (static_cast<int>(canMsgRx.data[2 * i + 2]) << 8)) / 100.0;
                        comm.ack_msg(canMsgRx);
                        Serial.printf("Luminance DESK %d: %f\n", i + 1, l);
                        my_desk.setRef(l);
                    }
                    break;
                    case 'e':
                    {
                        if (my_desk.getHub())
                        {
                            Serial.println("err");
                        }
                    }
                    break;
                    case 'a':
                    {
                        if (my_desk.getHub())
                        {
                            Serial.println("ack");
                        }
                    }
                    break;
                    case 'l': // streaming
                    {
                        if (my_desk.getHub())
                        {
                            float lux = (static_cast<int>(canMsgRx.data[1]) + (static_cast<int>(canMsgRx.data[2]) << 8)) / 100.0;
                            unsigned int time;
                            memcpy(&time, &canMsgRx.data[3], sizeof(unsigned int));
                            Serial.printf("s l %f %u", lux, time);
                        }
                    }
                    break;
                    case 'd': // streaming
                    {
                        if (my_desk.getHub())
                        {
                            float duty_cycle = (static_cast<int>(canMsgRx.data[1]) + (static_cast<int>(canMsgRx.data[2]) << 8)) / 100.0;
                            unsigned int time;
                            memcpy(&time, &canMsgRx.data[3], sizeof(unsigned int));
                            Serial.printf("s l %f %u", duty_cycle, time);
                        }
                    }
                    break;
                    case 'L': // last minute buffer
                    {
                        float lux;
                        for (int i = 0; i < 3; i++)
                        {
                            lux = (static_cast<int>(canMsgRx.data[2 * i + 1]) + (static_cast<int>(canMsgRx.data[2 * i + 2]) << 8)) / 100.0;
                            my_desk.store_buffer_l(canMsgRx.can_id - 1, lux);
                        }
                    }
                    break;
                    case 'D': // last minute buffer
                    {
                        float duty_cycle;
                        for (int i = 0; i < 3; i++)
                        {
                            duty_cycle = (static_cast<int>(canMsgRx.data[2 * i + 1]) + (static_cast<int>(canMsgRx.data[2 * i + 2]) << 8)) / 100.0;
                            my_desk.store_buffer_d(canMsgRx.can_id - 1, duty_cycle);
                        }
                    }
                    break;
                    default:
                        // passar de canMsgRx.data[] para string e mandar para o read_command()
                        break;
                    }
                }
            }
        }
    }
}

void wakeUp()
{
    if (!comm.isConnected())
    {
        static unsigned long timer_new_node = 0;
        long time_now = millis();
        if (time_now - comm.getConnectTime() > comm.getTimeToConnect())
        {
            comm.setConnected(true);
            my_desk.setDeskNumber(comm.find_desk());
            if (my_desk.getDeskNumber() == 1)
            {
                my_desk.setHub();
            }
            comm.connection_msg('A');
            flag_temp = true; // TODO CONECTAR PARA QUALQUER NUM DE DESKS
                              // TODO Confirmar mensagens de e N antes de correr o new_calibration
        }
        else
        {
            if (time_now - timer_new_node > 200) // Send a new connection message every 200ms
            {
                comm.connection_msg('N');
                timer_new_node = millis();
            }
        }
    }
}

void resendAck() // Resend the last message received
{
    long time_now = millis();

    // RESEND LAST MESSAGE IF NO ACK RECEIVED
    if ((time_now - comm.time_ack_get()) > TIME_ACK && !comm.isMissingAckEmpty())
    {
        Serial.printf("RESEND PORQUE NAO RECEBI ACK das desks: ");
        for (const int &element : comm.getMissingAck())
        {

            Serial.printf("%d, ", element);
        }
        Serial.println();
        comm.resend_last_msg();
        comm.time_ack_set(millis());
    }
}

void start_calibration()
{
    if (flag_temp && (comm.getNumDesks()) == 3) // Initialize the calibration when all the desks are connected
    {
        if (my_desk.getDeskNumber() == 3)
        {
            comm.new_calibration();
        }
        flag_temp = false;
    }
}