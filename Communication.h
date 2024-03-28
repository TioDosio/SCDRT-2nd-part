#ifndef COMMUNICATION_H
#define COMMUNICATION_H
#include <hardware/flash.h> //for flash_get_unique_id
#include "mcp2515.h"
#include <set>   // std::set
#include <queue> // std::queue
#include "extrafunctions.h"

class communication
{
    unsigned long connect_time;
    std::set<int> desks_connected, missing_ack;
    bool is_connected, consensus_acknoledged;
    int time_to_connect;
    float light_off, light_on;
    uint8_t this_pico_flash_id[8], node_address;
    MCP2515::ERROR err;
    MCP2515 can0{spi0, 17, 19, 16, 18, 10000000};

public:
    explicit communication();
    ~communication(){};
    int find_desk();
    void acknowledge_loop();
    void msg_received_consensus();
    void msg_received_ack(can_frame canMsgRx);
    void consensus_msg(double d[3]);

    // Getters
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

    uint8_t getThisPicoFlashId(int value)
    {
        return this_pico_flash_id[value];
    }

    uint8_t getNodeAddress() const
    {
        return node_address;
    }

    inline bool isMissingAckEmpty() const
    {
        return missing_ack.empty();
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

    void add2TimeToConnect(int time)
    {
        time_to_connect += time;
    }

    void setNodeAddress(uint8_t address)
    {
        node_address = address;
    }

    bool getConsensusAck()
    {
        return consensus_acknoledged;
    }

    void setConsensusAck(bool ack)
    {
        consensus_acknoledged = ack;
    }
};

#endif // COMMUNICATION_H