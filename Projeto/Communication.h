#ifndef COMMUNICATION_H
#define COMMUNICATION_H
#include <hardware/flash.h> //for flash_get_unique_id
#include "mcp2515.h"
#include <set>   // std::set
#include <queue> // std::queue
#include "extrafunctions.h"
#include "consensus.h"
#include "pid.h"
#include "luminaire.h"

class communication
{
    std::queue<can_frame> command_queue;
    unsigned long connect_time;
    std::set<int> desks_connected, missing_ack;
    bool is_connected;
    long time_to_connect;
    can_frame last_msg_sent;
    int last_msg_counter;
    double light_off, light_on;
    double *coupling_gains;
    bool is_calibrated;
    long time_ack;
    MCP2515::ERROR err;
    MCP2515 can0{spi0, 17, 19, 16, 18, 10000000};
    luminaire *my_desk;
    bool core1_reading;
    bool core0_reading;

public:
    explicit communication(luminaire *_my_desk);
    ~communication(){};
    int find_desk();
    void acknowledge_loop(Node *node, pid *my_pid);
    void calibration_msg(int dest_desk, char type);
    void msg_received_connection(can_frame canMsgRx);
    void msg_received_calibration(can_frame canMsgRx, Node *node, pid *my_pid);
    void msg_received_ack(can_frame canMsgRx, Node *node, pid *my_pid);
    void consensus_msg_duty(double duty[3]);
    void consensus_msg_switch(int dest_desk, char type);
    void msg_received_consensus(can_frame canMsgRx, Node *node);
    void confirm_msg(can_frame ack_msg);
    int char_msg_to_int(char msg);
    char int_to_char_msg(int msg);
    void resend_last_msg();
    void connection_msg(char type);
    void ack_msg(can_frame orig_msg);
    void cross_gains();
    void Gain();
    void new_calibration();
    void delay_manual(unsigned long time);
    void send_consensus_data(char part, Node *node, int destination);
    void reset_values(Node *node);

    // Getters

    void can0BufferReset()
    {
        while (can0.checkReceive())
        {
        }
        Serial.println("Buffer reseted");
    }

    double getExternalLight() const
    {
        return light_off;
    }

    double getCouplingGain(int index)
    {
        return coupling_gains[index];
    }

    double *getCouplingGains()
    {
        return coupling_gains;
    }
    long time_ack_get()
    {
        return time_ack;
    }

    void resetCan0()
    {
        can0.reset();
    }

    void setCan0Bitrate()
    {
        can0.setBitrate(CAN_1000KBPS);
    }

    void setCan0NormalMode()
    {
        can0.setNormalMode();
    }

    unsigned long getConnectTime() const
    {
        return connect_time;
    }

    inline int getNumDesks() const
    {
        return desks_connected.size() + 1;
    }

    bool isConnected() const
    {
        return is_connected;
    }

    int getTimeToConnect() const
    {
        return time_to_connect;
    }

    inline bool isMissingAckEmpty() const
    {
        return missing_ack.empty();
    }

    std::set<int> getMissingAck()
    {
        return missing_ack;
    }

    bool isCore1Reading()
    {
        return core1_reading;
    }

    bool isCore0Reading()
    {
        return core0_reading;
    }

    // Setters
    void setConnectTime(unsigned long time)
    {
        connect_time = time;
    }

    void setConnected(bool connected)
    {
        is_connected = connected;
    }

    void add2TimeToConnect(long time)
    {
        time_to_connect += time;
    }

    bool getIsCalibrated()
    {
        return is_calibrated;
    }

    void setIsCalibrated(bool cal)
    {
        is_calibrated = cal;
    }

    void time_ack_set(long time)
    {
        time_ack = time;
    }

    // Queue
    void add_msg_queue(can_frame msg)
    {
        command_queue.push(msg);
    }

    can_frame get_msg_queue()
    {
        can_frame msg = command_queue.front();
        command_queue.pop();
        return msg;
    }

    bool isQueueEmpty()
    {
        return command_queue.empty();
    }

    void setCore1Reading(bool reading)
    {
        core1_reading = reading;
    }

    void setCore0Reading(bool reading)
    {
        core0_reading = reading;
    }

    // Can0

    bool IsMsgAvailable()
    {
        return can0.checkReceive();
    }

    void ReadMsg(can_frame *msg)
    {
        err = can0.readMessage(msg);
    }

    void sendMsg(can_frame *msg)
    {
        err = can0.sendMessage(msg);
    }

    MCP2515::ERROR getError()
    {
        return err;
    }

    // getDeskconnected

    std::set<int> getDesksConnected()
    {
        return desks_connected;
    }
};

#endif // COMMUNICATION_H