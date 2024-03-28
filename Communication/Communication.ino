#include <hardware/flash.h> //for flash_get_unique_id
#include "mcp2515.h"
uint8_t this_pico_flash_id[8], node_address;
struct can_frame canMsgTx, canMsgRx;
unsigned long counterTx{0}, counterRx{0};
MCP2515::ERROR err;
unsigned long time_to_write;
unsigned long write_delay{10};
const byte interruptPin{20};
volatile byte data_available{false};
MCP2515 can0{spi0, 17, 19, 16, 18, 10000000};
// the interrupt service routine
void read_interrupt(uint gpio, uint32_t events)
{
    data_available = true;
}
int i = 0;
void setup()
{
    flash_get_unique_id(this_pico_flash_id);
    node_address = this_pico_flash_id[6];
    Serial.begin();
    can0.reset();
    can0.setBitrate(CAN_1000KBPS);
    can0.setNormalMode();
    gpio_set_irq_enabled_with_callback(
        interruptPin, GPIO_IRQ_EDGE_FALL,
        true, &read_interrupt);
    time_to_write = millis() + write_delay;
}
void loop()
{
    if (millis() >= time_to_write)
    {
        i++;
        canMsgTx.can_id = node_address;
        canMsgTx.can_dlc = 8;

        // Serialize integers into bytes
        int value1 = i;     // Example value
        int value2 = i * 2; // Example value
        memcpy(canMsgTx.data, &value1, sizeof(value1));
        memcpy(canMsgTx.data + sizeof(value1), &value2, sizeof(value2));

        err = can0.sendMessage(&canMsgTx);
        if (err != MCP2515::ERROR_OK)
        {
            Serial.print("Error sending message (");
            Serial.print(err);
            Serial.println(")");
        }
        else if (err == MCP2515::ERROR_FAILTX)
        {
            Serial.println("Failed to send message");
        }
        else if (err == MCP2515::ERROR_ALLTXBUSY)
        {
            Serial.println("All TX buffers busy");
        }
        else
        {
            Serial.print("Sending message ");
            Serial.print(value1);
            Serial.print(" and ");
            Serial.print(value2);
            Serial.print(" from node ");
            Serial.println(node_address, HEX);
            counterTx++;
            time_to_write = millis() + write_delay;
        }
    }
    if (data_available)
    {
        err = can0.readMessage(&canMsgRx);
        if (err != MCP2515::ERROR_OK)
        {
            Serial.print("Error reading message (");
            Serial.print(err);
            Serial.println(")");
        }
        else if (err == MCP2515::ERROR_NOMSG)
        {
            Serial.println("No message received");
        }
        else if (err == MCP2515::ERROR_FAIL)
        {
            Serial.println("Failed to read message");
        }
        else
        {
            Serial.print("Received message number ");
            Serial.print(counterRx++);
            Serial.print(" from node ");
            Serial.print(canMsgRx.can_id, HEX);
            Serial.print(" : ");

            // Deserialize bytes into integers
            int received_value1, received_value2;
            memcpy(&received_value1, canMsgRx.data, sizeof(received_value1));
            memcpy(&received_value2, canMsgRx.data + sizeof(received_value1), sizeof(received_value2));

            Serial.print("Value 1: ");
            Serial.print(received_value1);
            Serial.print(", Value 2: ");
            Serial.println(received_value2);

            data_available = false;
        }
    }
}
