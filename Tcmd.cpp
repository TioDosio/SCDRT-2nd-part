#include <iostream>
#include <vector>
#include <sstream>

// Function to parse the input message and extract the command and parameters
std::pair<std::string, std::vector<int> > parseCommand(const std::string &message)
{
    std::istringstream iss(message);
    std::vector<std::string> tokens;
    std::string token;

    // Split the message into tokens
    while (iss >> token)
    {
        tokens.push_back(token);
    }

    // Ensure the minimum number of tokens (command type is mandatory)
    if (tokens.empty())
    {
        return {"", {}};
    }

    std::string command = tokens[0];
    std::vector<int> params;

    // Analyze the command type and extract parameters accordingly
    if (command == "r" || command == "d" || command == "g" || command == "o" ||
        command == "a" || command == "k" || command == "c" || command == "S" ||
        command == "U" || command == "O")
    {
        if (tokens.size() >= 2)
        {
            for (size_t i = 1; i < tokens.size(); ++i)
            {
                try
                {
                    params.push_back(std::stoi(tokens[i]));
                }
                catch (const std::exception &e)
                {
                    std::cerr << "tudo -> Invalid parameter: " << tokens[i] << std::endl;
                    return {"", {}};
                }
            }
        }
        else
        {
            std::cerr << "Insufficient parameters for command: " << command << std::endl;
            return {"", {}};
        }
    }
    else if (command == "s")
    {
        if (tokens.size() == 3)
        {
            // Ensure the first parameter is 'l' or 'd'
            if (tokens[1] == "l" || tokens[1] == "d")
            {
                command += " " + tokens[1];
                try
                {
                    params.push_back(std::stoi(tokens[2]));
                }
                catch (const std::exception &e)
                {
                    std::cerr << "s -> Invalid parameter: " << tokens[2] << std::endl;
                    return {"", {}};
                }
            }
            else
            {
                std::cerr << "Invalid parameter for command 's': " << tokens[1] << std::endl;
                return {"", {}};
            }
        }
        else
        {
            std::cerr << "Invalid number of parameters for command 's'" << std::endl;
            return {"", {}};
        }
    }
    else if (command == "g")
    {
        // Check for 'g' commands with specific format
        if (tokens.size() >= 2 && (tokens[1] == "r" || tokens[1] == "d" || tokens[1] == "l" ||
                                   tokens[1] == "o" || tokens[1] == "a" || tokens[1] == "k" ||
                                   tokens[1] == "x" || tokens[1] == "p" || tokens[1] == "t" ||
                                   tokens[1] == "b" || tokens[1] == "e" || tokens[1] == "v" ||
                                   tokens[1] == "f" || tokens[1] == "O" || tokens[1] == "U" ||
                                   tokens[1] == "L" || tokens[1] == "c"))
        {
            command += " " + tokens[1];
            if (tokens.size() >= 3)
            {
                for (size_t i = 2; i < tokens.size(); ++i)
                {
                    try
                    {
                        params.push_back(std::stoi(tokens[i]));
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "g -> Invalid parameter: Banana" << tokens[i] << std::endl;
                        return {"", {}};
                    }
                }
            }
        }
        else
        {
            std::cerr << "Invalid parameter for command 'g': " << tokens[1] << std::endl;
            return {"", {}};
        }
    }
    else
    {
        std::cerr << "Unknown command: " << command << std::endl;
        return {"", {}};
    }

    return {command, params};
}

int main()
{
    while (true)
    {
        std::string message;

        // Prompt the user to enter a command
        std::cout << "Enter command: ";
        std::getline(std::cin, message);

        // Parse the command and extract parameters
        auto [command, params] = parseCommand(message);

        if (!command.empty())
        {
            // Display the parsed command and parameters
            std::cout << "Command: " << command;
            if (!params.empty())
            {
                std::cout << ", Parameters: ";
                for (int param : params)
                {
                    std::cout << param << " ";
                }
            }
            std::cout << std::endl;
        }
    }
    return 0;
}