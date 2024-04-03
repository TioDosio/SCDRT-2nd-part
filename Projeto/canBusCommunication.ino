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
                if (canMsgRx.data[0] == 'Q' || canMsgRx.data[2] == comm.int_to_char_msg(my_desk.getDeskNumber()) || canMsgRx.data[2] == comm.int_to_char_msg(0)) // Check if the message is for this desk (0 is for all the desks)
                {
                    comm.add_msg_queue(canMsgRx); // Put all the messages in the queue
                }
            }

            while (!comm.isQueueEmpty() && comm.isMissingAckEmpty())
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
                    switch (canMsgRx.data[0])
                    {
                    case 'W':
                        comm.msg_received_connection(canMsgRx);
                        break;
                    case 'C':
                    {
                        if (comm.isConnected())
                        {
                            comm.msg_received_calibration(canMsgRx);
                        }
                    }
                    break;
                    case 'Q': // TODO: escolher outro
                    {
                        if (comm.getIsCalibrated())
                        {
                            comm.msg_received_consensus(canMsgRx, &node);
                        }
                    }
                    break;
                    case 'q': // TODO: escolher outro / ACho que precisa do getIsCalibrated
                        runConsensus();
                        break;
                    case 'T':
                    {
                        if (node.getConsensusRunning())
                        {
                            if (node.getConsensusReady()) // Check if all the calculations are done and ready to send
                            {
                                comm.consensus_msg_duty(node.getD());
                                node.setConsensusReady(false);
                            }
                            else // If not, add the message to the queue again
                            {
                                comm.add_msg_queue(canMsgRx);
                            }
                        }
                    }
                    break;
                    case 'E':
                    {
                        node.setConsensusRunning(false);
                        double l = node.getKIndex(0) * node.getDavIndex(0) + node.getKIndex(1) * node.getDavIndex(1) + node.getKIndex(2) * node.getDavIndex(2) + node.getO();
                        Serial.printf("Luminance: %f\n", l);
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
                    case 'l':
                    {
                        float lux = (static_cast<int>(canMsgRx.data[1]) + (static_cast<int>(canMsgRx.data[2]) << 8)) / 100.0;
                        unsigned int time;
                        memcpy(&time, &canMsgRx.data[3], sizeof(unsigned int));
                        String command;
                        command.concat("s ");
                        command.concat(canMsgRx.data[0]);
                        command.concat(" ");
                        command.concat(lux);
                        command.concat(" ");
                        command.concat(time);

                        read_command(command, 1);
                    }
                    break;
                    case 'd':
                    {
                        float duty_cycle = (static_cast<int>(canMsgRx.data[1]) + (static_cast<int>(canMsgRx.data[2]) << 8)) / 100.0;
                        unsigned int time;
                        memcpy(&time, &canMsgRx.data[3], sizeof(unsigned int));
                        String command;
                        command.concat("s ");
                        command.concat(canMsgRx.data[0]);
                        command.concat(" ");
                        command.concat(duty_cycle);
                        command.concat(" ");
                        command.concat(time);

                        read_command(command, 1);
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
            newCalibration = true;
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
