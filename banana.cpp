#include <iostream>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

int main()
{
    // Get a list of available serial ports
    boost::system::error_code ec;

    std::cout << "Available serial ports:" << std::endl;

    namespace fs = boost::filesystem;
    for (fs::directory_iterator it("/dev"); it != fs::directory_iterator(); ++it)
    {
        const fs::path &port = it->path();
        if (fs::is_character_file(port, ec) && !ec)
        {
            std::cout << port << std::endl;
        }
    }

    return 0;
}
