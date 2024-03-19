#include <iostream>
#include <boost/asio.hpp>

using namespace boost::asio;

int main()
{
    io_context io;
    ip::tcp::socket socket(io);

    // Connect to the server
    socket.connect(ip::tcp::endpoint(ip::address::from_string("127.0.0.1"), 10000));

    std::string message;
    while (true)
    {
        std::cout << "Enter message to send (or type 'exit' to quit): ";
        std::getline(std::cin, message);

        if (message == "exit")
            break;

        // Append newline character to the message
        message += '\n';

        // Send the message to the server
        boost::system::error_code error;
        write(socket, buffer(message), error);
        if (error)
        {
            std::cerr << "Error sending message: " << error.message() << std::endl;
            break;
        }

        // Receive response from the server
        char buf[128];
        size_t len = socket.read_some(buffer(buf, sizeof(buf)), error);
        if (error)
        {
            std::cerr << "Error receiving response: " << error.message() << std::endl;
            break;
        }
        std::cout << "Received: " << std::string(buf, len) << std::endl;
    }

    return 0;
}
