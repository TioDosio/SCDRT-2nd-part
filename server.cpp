#include <iostream>
#include <memory>
#include <boost/asio.hpp>

using namespace boost::asio;

constexpr size_t MAX_SZ{128};

class session : public std::enable_shared_from_this<session>
{
    ip::tcp::socket sock;
    char rbuf[MAX_SZ] = {0}, tbuf[MAX_SZ] = {0};

public:
    session(ip::tcp::socket s) : sock(std::move(s)) {}

    void start()
    {
        auto self{shared_from_this()};
        sock.async_read_some(buffer(rbuf, MAX_SZ),
                             [this, self](boost::system::error_code ec, size_t sz)
                             {
                                 if (!ec)
                                 {
                                     rbuf[sz] = 0;
                                     std::cout << "Received: " << rbuf << std::endl;
                                     async_write(sock, buffer(rbuf, sz),
                                                 [this, self](boost::system::error_code ec, std::size_t /* sz */) {});
                                     start();
                                 }
                             });
    }
};

class server
{
    ip::tcp::acceptor acc;

    void start_accept()
    {
        acc.async_accept(
            [this](boost::system::error_code ec, ip::tcp::socket sock)
            {
                if (!ec)
                {
                    std::make_shared<session>(std::move(sock))->start();
                }
                start_accept();
            });
    }

public:
    server(io_context &io, unsigned short port)
        : acc{io, ip::tcp::endpoint{ip::tcp::v4(), port}}
    {
        start_accept();
    }
};

int main()
{
    io_context io;
    server s(io, 10000);
    io.run();
    return 0;
}
