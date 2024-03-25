#include <iostream>
#include <memory>
#include <boost/asio.hpp>

using namespace boost::asio;

constexpr size_t MAX_SZ{128};

class session : public std::enable_shared_from_this<session>
{
    ip::tcp::socket sock;
    char rbuf[MAX_SZ] = {0}, tbuf[MAX_SZ] = {0};
    boost::asio::serial_port &sp; // Reference to serial port

public:
    session(ip::tcp::socket s, boost::asio::serial_port &serial_port)
        : sock(std::move(s)), sp(serial_port) {}

    void start()
    {
        auto self{shared_from_this()};
        sock.async_read_some(buffer(rbuf, MAX_SZ),
                             [this, self](boost::system::error_code ec, size_t sz)
                             {
                                 if (!ec)
                                 {
                                     rbuf[sz] = 0;
                                     std::cout << "Received command from client: " << rbuf << std::endl;
                                     async_write(sock, buffer(rbuf, sz),
                                                 [this, self](boost::system::error_code ec, std::size_t /* sz */) {});

                                     // Write the received command to the serial port
                                     boost::asio::write(sp, buffer(rbuf, sz));

                                     start();
                                 }
                             });
    }
};

class server
{
    ip::tcp::acceptor acc;
    boost::asio::serial_port &sp; // Reference to serial port

    void start_accept()
    {
        acc.async_accept(
            [this](boost::system::error_code ec, ip::tcp::socket sock)
            {
                if (!ec)
                {
                    std::make_shared<session>(std::move(sock), sp)->start();
                }
                start_accept();
            });
    }

public:
    server(io_context &io, unsigned short port, boost::asio::serial_port &serial_port)
        : acc{io, ip::tcp::endpoint{ip::tcp::v4(), port}}, sp(serial_port)
    {
        start_accept();
    }
};

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <portname>" << std::endl;
        return 1;
    }

    boost::asio::io_context io;
    boost::asio::serial_port sp{io, argv[1]};

    if (!sp.is_open())
    {
        std::cerr << "Could not open serial port\n";
        return 1;
    }

    sp.set_option(boost::asio::serial_port_base::baud_rate{115200});

    server s(io, 10000, sp);
    io.run();

    return 0;
}
