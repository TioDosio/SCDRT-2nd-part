#include <iostream>
#include <boost/asio.hpp>
#include "functions.h"

// For POSIX-compatible systems
#include <sys/select.h>
#include <unistd.h>

using namespace boost::asio;

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <IP address> <port>" << std::endl;
        return 1;
    }

    try
    {
        io_context io;
        ip::tcp::socket socket(io);

        // Parse command-line arguments for IP address and port
        std::string ip_address = argv[1];
        unsigned short port = std::stoi(argv[2]);

        // Connect to the server
        socket.connect(ip::tcp::endpoint(ip::address::from_string(ip_address), port));

        while (true)
        {
            // Set up fd_set for select()
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(STDIN_FILENO, &read_fds);           // Add stdin to set
            FD_SET(socket.native_handle(), &read_fds); // Add socket to set

            // Timeout value for select (in this case, NULL for indefinite)
            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 0;

            // Use select() to wait for activity on file descriptors
            int select_ret = select(socket.native_handle() + 1, &read_fds, NULL, NULL, &timeout);
            if (select_ret == -1)
            {
                perror("select");
                break;
            }

            if (FD_ISSET(STDIN_FILENO, &read_fds))
            {
                // User input available
                std::string message;
                if (!std::getline(std::cin, message))
                {
                    // Handle input error
                    std::cerr << "Error reading input" << std::endl;
                    break;
                }

                int flag_msg = ver_message(message);
                if (flag_msg == 1)
                {
                    // Send the message to the server
                    boost::system::error_code error;
                    write(socket, buffer(message), error);
                    if (error)
                    {
                        std::cerr << "Error sending message: " << error.message() << std::endl;
                        break;
                    }
                }
                else if (flag_msg == -1)
                {
                    std::cout << "Good Bye" << std::endl;
                    break;
                }
            }

            if (FD_ISSET(socket.native_handle(), &read_fds))
            {
                // Data available to read from socket
                char buf[4000];
                boost::system::error_code error;
                size_t len = socket.read_some(buffer(buf, sizeof(buf)), error);
                if (error)
                {
                    std::cerr << "Error receiving response: " << error.message() << std::endl;
                    break;
                }
                if (len > 0)
                {
                    std::cout << " " << std::string(buf, len) << std::endl;
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
