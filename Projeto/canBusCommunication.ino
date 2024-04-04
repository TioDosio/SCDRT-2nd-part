#include <string>

/*
 * Manage all the communication received and send responses
 */
inline void communicationLoop()
{
    if (data_available)
    {
        data_available = false;
        if (!comm.isMissingAckEmpty()) // Check if there are any missing acks
        {
            comm.acknowledge_loop(&node, &my_pid);
        }
        else
        {
            while (comm.IsMsgAvailable()) // Check if something has been received
            {
                can_frame canMsgRx;
                comm.ReadMsg(&canMsgRx);
                if ((canMsgRx.can_id == my_desk.getDeskNumber() || canMsgRx.can_id == 0) && canMsgRx.data[0] != 'A') // Check if the message is for this desk (0 is for all the desks)
                {
                    comm.add_msg_queue(canMsgRx); // Put all the messages in the queue
                }
            }

            while (!comm.isQueueEmpty())
            {
                can_frame canMsgRx;
                canMsgRx = comm.get_msg_queue();

                switch (canMsgRx.data[0])
                {
                case 'W':
                    comm.msg_received_connection(canMsgRx);
                    break;
                case 'C':
                    comm.msg_received_calibration(canMsgRx, &node, &my_pid);
                    break;
                case 'Q':
                    comm.msg_received_consensus(canMsgRx, &node);
                    break;
                case 'q':
                    runConsensus();
                    break;
                case 'T':
                {
                    if (node.getConsensusReady())
                    {
                        comm.consensus_msg_duty(node.getD());
                        node.setConsensusReady(false);
                    }
                    else
                    {
                        if (node.getConsensusRunning())
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
                    ref_change(l);
                }
                break;
                case 'Y':
                {
                    if (my_desk.getHub())
                    {
                        Serial.println("err");
                    }
                }
                break;
                case 'Z':
                {
                    if (my_desk.getHub())
                    {
                        Serial.println("ack");
                    }
                }
                break;
                case 's': // streaming
                {
                    if (my_desk.getHub())
                    {
                        float value = (static_cast<int>(canMsgRx.data[3]) + (static_cast<int>(canMsgRx.data[4]) << 8)) / 100.0;
                        unsigned int time = (static_cast<int>(canMsgRx.data[5]) + (static_cast<int>(canMsgRx.data[6]) << 8)) / 100.0;

                        Serial.printf("s %c %c %f %u\n", canMsgRx.data[1], canMsgRx.data[2], value, time);
                    }
                    else
                    {
                        String command;
                        command = String((char)canMsgRx.data[0]) + " " + String((char)canMsgRx.data[1]) + " " + String(canMsgRx.can_id);
                        read_command(command.c_str(), 1);
                    }
                }
                break;
                case 'b':
                {
                    float lux;
                    int desk = comm.char_msg_to_int(canMsgRx.data[1]);
                    for (int i = 0; i < 3; i++)
                    {
                        lux = (static_cast<int>(canMsgRx.data[2 * i + 2]) + (static_cast<int>(canMsgRx.data[2 * i + 3]) << 8)) / 100.0;
                        my_desk.store_buffer_l(desk, lux);
                        my_desk.Compute_avg(my_pid.get_h(), lux, my_desk.getRef(), my_desk.getDeskNumber());
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
                    command_msgs(canMsgRx);
                    //  passar de canMsgRx.data[] para string e mandar para o read_command()
                    break;
                }
            }
        }
    }
    if (comm.isCore0Reading())
    {
        comm.setCore0Reading(0);
        comm.calibration_msg(0, 'r');
    }
}

/*
 * When turn one node on,notify the others and start calibrate
 */
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
                my_desk.setHub(true);
            }
            comm.connection_msg('A');
            flag_temp = true;
        }
        else
        {
            if (time_now - timer_new_node > 300) // Send a new connection message every 300ms
            {
                comm.connection_msg('N');
                timer_new_node = millis();
            }
        }
    }
}

/*
 * Resend the last message received if no ack was received
 */
void resendAck()
{
    long time_now = millis();

    if ((time_now - comm.time_ack_get()) > TIME_ACK && !comm.isMissingAckEmpty())
    {
        Serial.printf("Resend ack: ");
        for (const int &element : comm.getMissingAck())
        {

            Serial.printf("%d, ", element);
        }
        Serial.println();
        comm.resend_last_msg();
        comm.time_ack_set(millis());
    }
}

/*
 * Start the calibration of the nodes in the system, only starts when all desks are connected
 */
inline void start_calibration()
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

void command_msgs(can_frame canMsgRx)
{
    if (canMsgRx.can_id == 1 && my_desk.getHub())
    {
        float value;
        memcpy(&value, &canMsgRx.data[2], sizeof(float));

        Serial.printf("%c %d %f", canMsgRx.data[0], comm.char_msg_to_int(canMsgRx.data[1]), value);
        return;
    }
    switch (canMsgRx.data[0])
    {
    case 'S':
    case 'g':
    {
        String command;
        if (canMsgRx.data[1] == 'b') // g b desk
        {
            command = String((char)canMsgRx.data[0]) + " " + String((char)canMsgRx.data[1]) + " " + String((char)canMsgRx.data[2]) + " " + String(canMsgRx.can_id);
        }
        else // g a desk | S l/d desk
        {
            command = String((char)canMsgRx.data[0]) + " " + String((char)canMsgRx.data[1]) + " " + String(canMsgRx.can_id);
        }
        read_command(command.c_str(), 1);
    }
    break;
    case 'd':
    case 'r':
    case 'o':
    case 'a':
    case 'k':
    case 'O':
    case 'U':
    case 'c':
    { // set's
        String command;
        float value;
        memcpy(&value, &canMsgRx.data[1], sizeof(float));

        if (canMsgRx.data[0] == 'o' || canMsgRx.data[0] == 'a' || canMsgRx.data[0] == 'k')
        {
            int aux = static_cast<int>(value);
            command = String((char)canMsgRx.data[0]) + " " + String(canMsgRx.can_id) + " " + String(aux);
        }
        else
        {
            command = String((char)canMsgRx.data[0]) + " " + String(canMsgRx.can_id) + " " + String(value);
        }
        read_command(command, 1);
    }
    break;
    default:
        break;
    }
}