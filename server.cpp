#include <iostream>
#include <memory>
#include <boost/asio.hpp>

using namespace boost::asio;

constexpr size_t MAX_SZ{128};

class session : public std::enable_shared_from_this<session>
{
    ip::tcp::socket sock;                        // Socket for communication
    char rbuf[MAX_SZ] = {0}, tbuf[MAX_SZ] = {0}; // Buffers for receiving and transmitting data

public:
    // Constructor: Initializes the session with a socket
    session(ip::tcp::socket s) : sock(std::move(s)) {}

    // Method to start the session
    void start()
    {
        auto self{shared_from_this()}; // Capture a shared pointer to this session
        sock.async_read_some(buffer(rbuf, MAX_SZ),
                             [this, self](boost::system::error_code ec, size_t sz) // Asynchronous read operation
                             {
                                 if (!ec) // If no error occurred
                                 {
                                     rbuf[sz] = 0;                                  // Null-terminate received data
                                     std::cout << "Received " << rbuf << std::endl; // Print received data
                                     strncpy(tbuf, rbuf, sz);                       // Copy received data to transmit buffer
                                     async_write(sock, buffer(tbuf, sz),            // Asynchronous write operation to echo back received data
                                                 [this, self](boost::system::error_code ec, std::size_t sz) {});
                                     start(); // Restart the session to continue listening for data
                                 }
                             } // end async_read_some lambda arg
        );                     // end async_read call
    }                          // end start()
};

class server
{
    ip::tcp::acceptor acc; // Acceptor for accepting incoming connections

    // Method to start accepting connections
    void start_accept()
    {
        acc.async_accept(
            [this](boost::system::error_code ec, ip::tcp::socket sock) // Asynchronous accept operation
            {
                if (!ec) // If no error occurred
                {
                    std::make_shared<session>(std::move(sock))->start(); // Create a new session and start it
                    std::cout << "Created new session" << std::endl;
                }
                start_accept(); // Continue accepting connections
            }                   // end async_accept lambda arg
        );                      // end async_accept call
    }                           // end start_accept()

public:
    // Constructor: Initializes the server with an io_context and a port
    server(io_context &io, unsigned short port)
        : acc{io, ip::tcp::endpoint{ip::tcp::v4(), port}} // Initialize the acceptor with the given io_context and port
    {
        std::cout << "Receiving at: " << acc.local_endpoint() << std::endl; // Print the local endpoint
        start_accept();                                                     // Start accepting connections
    }
};

int main(int argc, char *argv[])
{
    io_context io;       // Create an io_context object
    server s{io, 10000}; // Create a server object listening on port 10000
    io.run();            // Run the io_context event loop
    return 0;
}
