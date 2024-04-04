#include <string>

inline void communicationLoop()
{
    if (data_available)
    {
        data_available = false;
        if (!comm.isMissingAckEmpty()) // Check if there are any missing acks
        {
            comm.acknowledge_loop(&node);
            data_available = true;
        }
        else
        {
            while (comm.IsMsgAvailable()) // Check if something has been received
            {
                can_frame canMsgRx;
                comm.ReadMsg(&canMsgRx);
                Serial.printf("Rec to %d: %c %c\n", canMsgRx.can_id, canMsgRx.data[0], canMsgRx.data[1]);
                if ((canMsgRx.can_id == my_desk.getDeskNumber() || canMsgRx.can_id == 0) && canMsgRx.data[0] != 'A') // Check if the message is for this desk (0 is for all the desks)
                {
                    comm.add_msg_queue(canMsgRx); // Put all the messages in the queue
                }
            }

            while (!comm.isQueueEmpty())
            {
                can_frame canMsgRx;
                canMsgRx = comm.get_msg_queue();
                Serial.printf("Escolhida %c %c\n", canMsgRx.data[0], canMsgRx.data[1]);
                switch (canMsgRx.data[0])
                {
                case 'W':
                    comm.msg_received_connection(canMsgRx);
                    break;
                case 'C':
                    comm.msg_received_calibration(canMsgRx, &node);
                    break;
                case 'T':
                {
                    comm.ack_msg(canMsgRx);
                    comm.send_consensus_data('1', &node, comm.char_msg_to_int(canMsgRx.data[1]));
                }
                break;
                case '1':
                {
                    comm.ack_msg(canMsgRx);
                    Serial.printf("ENTrei no '1' de %d \n", comm.char_msg_to_int(canMsgRx.data[1]));
                    comm.delay_manual(100);
                    std::set<int> desks = comm.getDesksConnected();
                    int index = std::distance(desks.begin(), desks.find(comm.char_msg_to_int(canMsgRx.data[1])));
                    OtherLuminaires Lums{};
                    for (int i = 0; i < 3; i++)
                    {
                        float value = (static_cast<int>(canMsgRx.data[2 * i + 2]) + (static_cast<int>(canMsgRx.data[2 * i + 3]) << 8)) / 1000.0;
                        Lums.setKIndex(i, value);
                        node.setLums(Lums, index);
                    }
                }
                break;
                case '2':
                {
                    comm.ack_msg(canMsgRx);
                    Serial.printf("ENTrei no '2' de %d \n", comm.char_msg_to_int(canMsgRx.data[1]));
                    std::set<int> desks = comm.getDesksConnected();
                    int index = std::distance(desks.begin(), desks.find(comm.char_msg_to_int(canMsgRx.data[1])));
                    OtherLuminaires Lums = node.getLums(index);
                    Lums.setO((static_cast<int>(canMsgRx.data[2]) + (static_cast<int>(canMsgRx.data[3]) << 8)) / 1000.0);
                    Lums.setC((static_cast<int>(canMsgRx.data[4]) + (static_cast<int>(canMsgRx.data[5]) << 8)) / 100.0);
                    Lums.setL((static_cast<int>(canMsgRx.data[6]) + (static_cast<int>(canMsgRx.data[7]) << 8)) / 100.0);
                    node.setLums(Lums, index);

                    if (node.receivedAllLums())
                    {
                        Serial.println("Received all Lums\n");
                        double newLuminance[3];
                        Serial.printf("Lixo ig -> %lf %lf %lf\n", newLuminance[0], newLuminance[1], newLuminance[2]);
                        mainConsensus(newLuminance);
                        Serial.printf("BONITO IG -> %lf %lf %lf\n", newLuminance[0], newLuminance[1], newLuminance[2]);
                        comm.consensus_msg_lux(newLuminance);
                    }
                    else
                    {
                        int dest = *(std::next((comm.getDesksConnected()).begin()));
                        comm.start_consensus(dest);
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
                    int desk = comm.char_msg_to_int(canMsgRx.data[1]);
                    for (int i = 0; i < 3; i++)
                    {
                        lux = (static_cast<int>(canMsgRx.data[2 * i + 2]) + (static_cast<int>(canMsgRx.data[2 * i + 3]) << 8)) / 100.0;
                        my_desk.store_buffer_l(desk, lux);
                        my_desk.Compute_avg(my_pid.get_h(), lux, my_desk.getRef(), my_desk.getDeskNumber()); // TODO adicionar compute average
                    }
                }
                break;
                case 'D': // last minute buffer
                {
                    float duty_cycle;
                    int desk = comm.char_msg_to_int(canMsgRx.data[1]);
                    for (int i = 0; i < 3; i++)
                    {
                        duty_cycle = (static_cast<int>(canMsgRx.data[2 * i + 2]) + (static_cast<int>(canMsgRx.data[2 * i + 3]) << 8)) / 100.0;
                        my_desk.store_buffer_d(desk, duty_cycle);
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
            Serial.printf("Time to connect %d\n", comm.getTimeToConnect());
            flag_temp = true; // TODO CONECTAR PARA QUALQUER NUM DE DESKS
                              // TODO Confirmar mensagens de e N antes de correr o new_calibration
        }
        else
        {
            if (time_now - timer_new_node > 400) // Send a new connection message every 200ms
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

// void command_msgs(can_frame canMsgRx)
// {
//     if (canMsgRx.can_id == 1 && my_desk.getHub())
//     {
//         float value;
//         memcpy(&value, &canMsgRx.data[2], sizeof(float));

//         Serial.printf("%c %d %f", canMsgRx.data[0], comm.char_msg_to_int(canMsgRx.data[1]), value);
//         Serial.printf("command received: %c %c %c %c %c %c %c %c %c\n", canMsgRx.data[0], canMsgRx.data[1], canMsgRx.data[2], canMsgRx.data[3], canMsgRx.data[4], canMsgRx.data[5], canMsgRx.data[6], canMsgRx.data[7]);
//         return;
//     }
//     switch (canMsgRx.data[0])
//     {
//     case 's':
//     case 'S':
//     case 'g':
//     {
//         String command;
//         if (canMsgRx.data[1] == 'b') // g b desk
//         {
//             command = String((char)canMsgRx.data[0]) + " " + String((char)canMsgRx.data[1]) + " " + String((char)canMsgRx.data[2]) + " " + String(canMsgRx.can_id);
//         }
//         else // g a desk || s l/d desk | S l/d desk
//         {
//             command = String((char)canMsgRx.data[0]) + " " + String((char)canMsgRx.data[1]) + " " + String(canMsgRx.can_id);

//             Serial.printf("command received: %s\n", command.c_str());
//         }
//         read_command(command.c_str(), 1);
//     }
//     break;
//     case 'd':
//     case 'r':
//     case 'o':
//     case 'a':
//     case 'k':
//     case 'O':
//     case 'U':
//     case 'c':
//     { // set's
//         if (!canMsgRx.can_id == 1)
//         {
//             String command;
//             float value;
//             memcpy(&value, &canMsgRx.data[1], sizeof(float));

//             if (canMsgRx.data[0] == 'o' || canMsgRx.data[0] == 'a' || canMsgRx.data[0] == 'k')
//             {
//                 int aux = static_cast<int>(value);
//                 command = String((char)canMsgRx.data[0]) + " " + String(canMsgRx.can_id) + " " + String(aux);
//             }
//             else
//             {
//                 command = String((char)canMsgRx.data[0]) + " " + String(canMsgRx.can_id) + " " + String(value);
//             }
//             read_command(command, 1);
//         }
//     }
//     break;
//     default:
//         break;
//     }
// }