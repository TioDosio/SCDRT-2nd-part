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
                if (canMsgRx.data[0] == 'S' || canMsgRx.data[2] == comm.int_to_char_msg(my_desk.getDeskNumber()) || canMsgRx.data[2] == comm.int_to_char_msg(0)) // Check if the message is for this desk (0 is for all the desks)
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
                    comm.msg_received_calibration(canMsgRx);
                    break;
                case 'S':
                    comm.msg_received_consensus(canMsgRx, &node);
                    break;
                case 's':
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
                    Serial.printf("Luminance: %f\n", l);
                    my_desk.setRef(l);
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
    if (flag_temp && (comm.getNumDesks()) == 3) // Initialize the calibration when all the desks are connected
    {
        if (my_desk.getDeskNumber() == 3)
        {
            comm.new_calibration();
        }
        flag_temp = false;
    }
}
