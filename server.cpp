#include <iostream>
#include <memory>
#include <boost/asio.hpp>
#include "Serial.cpp" // Assuming this file contains additional functionality related to serial communication.

using namespace boost::asio; // Import Boost.Asio symbols into the global namespace.

constexpr size_t MAX_SZ{128}; // Maximum size for read/write buffers.

class session : public std::enable_shared_from_this<session>
{
    ip::tcp::socket sock;                        // TCP socket for this session.
    char rbuf[MAX_SZ] = {0}, tbuf[MAX_SZ] = {0}; // Read and write buffers.

public:
    session(ip::tcp::socket s) : sock(std::move(s)) {} // Constructor taking a socket and moving it into the session.

    void start() // Method to start reading from the socket.
    {
        auto self{shared_from_this()};                                             // Capture a shared pointer to this session.
        sock.async_read_some(buffer(rbuf, MAX_SZ),                                 // Asynchronously read data into rbuf.
                             [this, self](boost::system::error_code ec, size_t sz) // Callback when data is read.
                             {
                                 if (!ec) // If no error.
                                 {
                                     rbuf[sz] = 0;                                                                     // Null-terminate received data.
                                     std::cout << "ack" << std::endl;                                                  // Print acknowledgment.
                                     async_write(sock, buffer(rbuf, sz),                                               // Asynchronously write back the received data.
                                                 [this, self](boost::system::error_code ec, std::size_t /* sz */) {}); // Empty callback.
                                     start();                                                                          // Restart reading.
                                 }
                             });
    }
};

class server
{
    ip::tcp::acceptor acc; // Acceptor for incoming connections.

    void start_accept() // Method to start accepting connections.
    {
        acc.async_accept(
            [this](boost::system::error_code ec, ip::tcp::socket sock) // Asynchronously accept connections.
            {
                if (!ec) // If no error.
                {
                    std::make_shared<session>(std::move(sock))->start(); // Create a session for the accepted connection and start it.
                }
                start_accept(); // Restart accepting connections.
            });
    }

public:
    server(io_context &io, unsigned short port)
        : acc{io, ip::tcp::endpoint{ip::tcp::v4(), port}} // Constructor initializing the acceptor with IPv4 and given port.
    {
        start_accept(); // Start accepting connections.
    }
};

int main()
{
    io_context io;       // Create an IO context.
    server s(io, 10000); // Create a server listening on port 10000.
    io.run();            // Run the IO context's event loop.
    return 0;
}
