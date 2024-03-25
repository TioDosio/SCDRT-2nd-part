#include <iostream>
#include <chrono>
#include <boost/asio.hpp>

// Namespace declarations
using namespace boost::system;
// Create an IO context object
boost::asio::io_context io_serial;
// Create a serial port object
boost::asio::serial_port sp{io_serial};
// Create a timer object
boost::asio::steady_timer tim{io_serial};
// Define maximum buffer size
constexpr int MAX_SZ = 128;
// Define read and write buffers
char wbuf[MAX_SZ], rbuf[MAX_SZ];
// Define a counter variable
int count = 0;
// Forward declaration of functions
void write_handler(const error_code ec, std::size_t nbytes);
void timer_handler(const error_code ec);
void read_handler(const error_code ec, std::size_t nbytes);

// Main write handler function
void write_handler(const error_code ec, std::size_t nbytes)
{
    // Set timer to expire after 2 seconds
    tim.expires_after(std::chrono::seconds{2});
    // Asynchronously wait for timer to expire and call timer_handler
    tim.async_wait(timer_handler);
}

// Timer handler function
void timer_handler(const error_code ec)
{
    // Prepare data to write to serial port
    size_t n = snprintf(wbuf, MAX_SZ, "%s%d\n", "Count = ", count++);
    // Asynchronously write data to serial port and call write_handler on completion
    sp.async_write_some(boost::asio::buffer(wbuf, n), write_handler);
}

// Read handler function
void read_handler(const error_code ec, std::size_t nbytes)
{
    // Check if read was successful
    if (nbytes < MAX_SZ)
    {
        // Null-terminate the received data to treat it as a string
        rbuf[nbytes] = 0;
        // Print the received data
        std::cout << rbuf;
    }
    else
        // Handle buffer overflow
        std::cout << "Buffer overflow\n";

    // Asynchronously wait for more data to be received and call read_handler on completion
    sp.async_read_some(boost::asio::buffer(rbuf, MAX_SZ), read_handler);
}

// Main function
int main(int argc, char *argv[])
{
    // Check if port name is provided as a command line argument
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <portname>" << std::endl;
        return 0;
    }

    // Open the serial port
    boost::system::error_code ec;
    sp.open(argv[1], ec);

    // Check for errors while opening port
    if (ec)
    {
        std::cout << "Could not open serial port \n";
        return 0;
    }

    // Set the baud rate for serial communication
    sp.set_option(boost::asio::serial_port_base::baud_rate{115200}, ec); // normally 9600??

    // Program timer for write operations
    tim.expires_after(std::chrono::seconds{2});
    tim.async_wait(timer_handler);

    // Program chain of read operations
    sp.async_read_some(boost::asio::buffer(rbuf, MAX_SZ), read_handler);

    // Run the IO context to start asynchronous operations
    io_serial.run();

    return 0;
}
