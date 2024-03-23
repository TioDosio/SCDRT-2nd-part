// File SERIAL_ASYNC.CPP
#include <iostream>
#include <chrono>
#include <boost/asio.hpp>
using namespace boost::system;
boost::asio::io_context io;
boost::asio::serial_port sp{io};
boost::asio::steady_timer tim{io};
constexpr int MAX_SZ = 128;
char wbuf[MAX_SZ], rbuf[MAX_SZ];
int count = 0;
void write_handler(const error_code ec, std::size_t nbytes);
void timer_handler(const error_code ec)
{
    size_t n = snprintf(wbuf, MAX_SZ, "%s%d\n", "Count = ", count++);
    sp.async_write_some(boost::asio::buffer(wbuf, n), write_handler);
}
void write_handler(const error_code ec, std::size_t nbytes)
{
    tim.expires_after(std::chrono::seconds{2});
    tim.async_wait(timer_handler);
}
void read_handler(const error_code ec, std::size_t nbytes)
{
    if (nbytes < MAX_SZ)
    {
        rbuf[nbytes] = 0; // string terminator
        std::cout << rbuf;
    }
    else
        std::cout << "Buffer overflow\n";
    sp.async_read_some(boost::asio::buffer(rbuf, MAX_SZ), read_handler);
}
int main(int argc, char *argv[])
{
    boost::system::error_code ec;
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <portname>" << std::endl;
        return 0;
    }
    sp.open(argv[1], ec); // connect to port
    if (ec)
    {
        std::cout << "Could not open serial port \n";
        return 0;
    }
    sp.set_option(boost::asio::serial_port_base::baud_rate{115200}, ec);
    // program timer for write operations
    tim.expires_after(std::chrono::seconds{2});
    tim.async_wait(timer_handler);
    // program chain of read operations
    sp.async_read_some(boost::asio::buffer(rbuf, MAX_SZ), read_handler);
    io.run(); // get things rolling
}