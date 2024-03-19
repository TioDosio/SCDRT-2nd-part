#include <iostream>
#include <boost/asio.hpp>

using namespace boost::asio;

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <IP address> <port>" << std::endl;
        return 1;
    }

    io_context io;
    ip::tcp::socket socket(io);

    try
    {
        // Parse command-line arguments for IP address and port
        std::string ip_address = argv[1];
        unsigned short port = std::stoi(argv[2]);

        // Connect to the server
        socket.connect(ip::tcp::endpoint(ip::address::from_string(ip_address), port));

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
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
