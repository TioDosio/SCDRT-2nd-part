#include <hardware/flash.h>
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

void read_command(const String &command)
{
    int i;
    float val;
    int this_desk = 2;

    if (command.startsWith("d ")) // Set directly the duty cycle of luminaire i.
    {
        sscanf(command.c_str(), "d %d %f", &i, &val);
        if (i == this_desk)
        {
            if (val >= 0 && val <= 5000) // DESLIGAR TUDO
            {
                Serial.println("Recebido AQUI");
            }
            else
            {
                Serial.println("err");
            }
        }
        else
        {
            Serial.println("enviado");
            send_to_others(i, "d", val, 1);
        }
    }
    else if (command.startsWith("g ")) // Get the duty cycle of luminaire i.
    {
        sscanf(command.c_str(), "g %d", &i);
        if (i == this_desk)
        {
            Serial.println("Recebido AQUI");
        }
        else
        {
            Serial.println("enviado");
            send_to_others(i, "g a", 0, 0);
        }
    }
}

void send_to_others(const int desk, const String &commands, const float value, int type)
{
    struct can_frame canMsgTx;
    if (type == 0)
    {
        canMsgTx.can_dlc = 2;
        canMsgTx.data[0] = commands.charAt(0);
        canMsgTx.data[1] = commands.charAt(2);
    }
    else
    {
        canMsgTx.can_dlc = 5;
        canMsgTx.data[0] = commands.charAt(0);
        Serial.printf("Value: %f\n", value);
        Serial.printf("Size: %d -- %d\n", sizeof(unsigned char), sizeof(int));
        memcpy(&canMsgTx.data[1], &value, sizeof(float));
    }
    canMsgTx.can_id = desk;

    can0.sendMessage(&canMsgTx);
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
    if (Serial.available() > 0)
    {
        String command = Serial.readStringUntil('\n');
        read_command(command);
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
            int desk = canMsgRx.can_id;
            // Determine the type of message based on the data payload length
            if (canMsgRx.can_dlc == 2) // g a for example
            {
                // Handle "char char float" message
                char char1 = canMsgRx.data[0];
                char char2 = canMsgRx.data[1];

                Serial.println("Received a 'char char' message  ");
                Serial.print("char1: ");
                Serial.println(char1);
                Serial.print("char2: ");
                Serial.println(char2);
            }
            else if (canMsgRx.can_dlc == 5) // r 10 for example
            {
                // Handle "char float" message
                char char1 = canMsgRx.data[0];
                float value;
                memcpy(&value, &canMsgRx.data[1], sizeof(float));

                Serial.print("Received a 'char int' message  ");
                Serial.print("char1: ");
                Serial.println(char1);
                Serial.print("value: ");
                Serial.println(value);
            }
            else
            {
                // Handle invalid message length
                Serial.println("Received an invalid message");
            }
            data_available = false;
        }
    }
}
