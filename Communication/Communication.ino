#include <hardware/flash.h> //for flash_get_unique_id
#include "mcp2515.h"
uint8_t this_pico_flash_id[8], node_address;
struct can_frame canMsgTx, canMsgRx;
unsigned long counterTx{0}, counterRx{0};
MCP2515::ERROR err;
unsigned long time_to_write;
unsigned long write_delay{1000};
const byte interruptPin{20};
volatile byte data_available{false};
MCP2515 can0{spi0, 17, 19, 16, 18, 10000000};
// the interrupt service routine
void read_interrupt(uint gpio, uint32_t events)
{
    data_available = true;
}

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
        canMsgTx.can_id = node_address;
        canMsgTx.can_dlc = 8;
        unsigned long div = counterTx * 10;
        for (int i = 0; i < 8; i++)
        {
            canMsgTx.data[7 - i] = '0' + ((div /= 10) % 10);
        }
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
            Serial.print(counterTx);
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
            for (int i = 0; i < canMsgRx.can_dlc; i++)
                Serial.print((char)canMsgRx.data[i]);
            Serial.println(" ");
            data_available = false;
        }
    }
}