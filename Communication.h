#ifndef COMMUNICATION_H
#define COMMUNICATION_H
#include <set>   // std::set

class communication
{
    unsigned long connect_time;
    std::set<int> desks_connected;
    bool is_connected;
    int time_to_connect;
    uint8_t this_pico_flash_id[8], node_address;

public:
    explicit communication();
    ~communication(){};
    int find_desk();

    // Getters
    unsigned long getConnectTime() const {
        return connect_time;
    }

    // std::set<int> getDesksConnected() const {
    //     return desks_connected;
    // }

    bool isConnected() const {
        return is_connected;
    }

    int getTimeToConnect() const {
        return time_to_connect;
    }

    uint8_t getThisPicoFlashId(int value) {
        return this_pico_flash_id[value];
    }

    uint8_t getNodeAddress() const {
        return node_address;
    }

    // Setters
    void setConnectTime(unsigned long time) {
        connect_time = time;
    }

    // void setDesksConnected(const std::set<int>& desks) {
    //     desks_connected = desks;
    // }

    void setConnected(bool connected) {
        is_connected = connected;
    }

    void setTimeToConnect(int time) {
        time_to_connect = time;
    }

    void setNodeAddress(uint8_t address) {
        node_address = address;
    }
};

#endif // COMMUNICATION_H