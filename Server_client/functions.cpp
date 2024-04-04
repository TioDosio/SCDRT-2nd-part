#include <iostream>
#include <string>
#include <sstream>
#include <vector>

int led_max = 3;

float isNumber(const std::string &str)
{
    std::istringstream iss(str);
    float num;
    if (iss >> num)
    {
        // std::cout << "The number is: " << num << std::endl;
        return num;
    }
    else
    {
        // std::cout << "The input is not a valid number." << std::endl;
        return -1;
    }
}

void help()
{
    // print help menu
    std::cout << "------Help menu------" << std::endl;
    std::cout << "exit          Quit" << std::endl;
    std::cout << "d <i> <val>   Set duty cycle value" << std::endl;
    std::cout << "g d <i>       Get the duty cyle value" << std::endl;
    std::cout << "r <i> <val>   Set reference value" << std::endl;
    std::cout << "g r <i>       Get reference value" << std::endl;
    std::cout << "g l <i>       Get the measured iluminance in LUX" << std::endl;
    std::cout << "o <i> <val>   Set occupancy value" << std::endl;
    std::cout << "g o <i>       Get occupancy value" << std::endl;
    std::cout << "a <i> <val>   Set anti-windup value 1-ON, 0-OFF" << std::endl;
    std::cout << "g a <i>       Get anti-windup value" << std::endl;
    std::cout << "k <i> <val>   Set feedback value 1-ON, 0-OFF" << std::endl;
    std::cout << "g k <i>       Get feedback value" << std::endl;
    std::cout << "g x <i>       Get external iluminance" << std::endl;
    std::cout << "g p <i>       Get instantaneous power consumption " << std::endl;
    std::cout << "g t <i>       Get the elapsed time since the last restart" << std::endl;
    std::cout << "s <x> <i>     Start the stream of real-time variable <x>, l-LUX, d-duty cycle" << std::endl;
    std::cout << "S <x> <i>     Stop the stream of real-time variable <x>, l-LUX, d-duty cycle" << std::endl;
    std::cout << "g b <i> <val> Get the last minute buffer of the variable <x> of the desk <i>" << std::endl;
    std::cout << "e <i> <val>   Get the average energy consumption at the desk <i> since the last system restart." << std::endl;
    std::cout << "g v <i>       Get the average visibility error at desk <i> since the last system restart" << std::endl;
    std::cout << "g f <i>       Get the average flicker error on desk <i> since the last system restart " << std::endl;
    std::cout << "O <i> <val>   Set lower bound on illuminance for the occupied state at desk <i> " << std::endl;
    std::cout << "g O <i>       Get lower bound on illuminance for the occupied state at desk <i>" << std::endl;
    std::cout << "U <i> <val>   Set lower bound on illuminance for the unoccupied state at desk <i> " << std::endl;
    std::cout << "g U <i>       Get lower bound on illuminance for the unoccupied state at desk <i>" << std::endl;
    std::cout << "g L <i>       Get the current illuminance lower bound at the desk <i> " << std::endl;
    std::cout << "c <i> <val>   Set the current energy cost at the desk <i>" << std::endl;
    std::cout << "g c <i>       Get the current energy cost at the desk <i>" << std::endl;
    std::cout << "r             Reset the system" << std::endl;
    std::cout << "" << std::endl;
}

int ver_message(const std::string &input)
{
    std::istringstream iss(input);
    std::vector<std::string> elements;
    std::string element;

    int flag = 0;

    while (std::getline(iss, element, ' '))
    {
        elements.push_back(element);
    }
    // Count the elements
    int numElements = elements.size();

    if (numElements == 3 && elements[0] == "g")
    {
        if (elements[1] == "d" && led_max >= isNumber(elements[2]) && 0 < isNumber(elements[2]))
        {
            flag = 1;
        }
        else if (elements[1] == "r" && led_max >= isNumber(elements[2]) && 0 < isNumber(elements[2]))
        {
            flag = 1;
        }
        else if (elements[1] == "l" && led_max >= isNumber(elements[2]) && 0 < isNumber(elements[2]))
        {
            flag = 1;
        }
        else if (elements[1] == "o" && led_max >= isNumber(elements[2]) && 0 < isNumber(elements[2]))
        {
            flag = 1;
        }
        else if (elements[1] == "a" && led_max >= isNumber(elements[2]) && 0 < isNumber(elements[2]))
        {
            flag = 1;
        }
        else if (elements[1] == "k" && led_max >= isNumber(elements[2]) && 0 < isNumber(elements[2]))
        {
            flag = 1;
        }
        else if (elements[1] == "x" && led_max >= isNumber(elements[2]) && 0 < isNumber(elements[2]))
        {
            flag = 1;
        }
        else if (elements[1] == "p" && led_max >= isNumber(elements[2]) && 0 < isNumber(elements[2]))
        {
            flag = 1;
        }
        else if (elements[1] == "t" && led_max >= isNumber(elements[2]) && 0 < isNumber(elements[2]))
        {
            flag = 1;
        }
        else if (elements[1] == "e" && led_max >= isNumber(elements[2]) && 0 < isNumber(elements[2]))
        {
            flag = 1;
        }
        else if (elements[1] == "v" && led_max >= isNumber(elements[2]) && 0 < isNumber(elements[2]))
        {
            flag = 1;
        }
        else if (elements[1] == "f" && led_max >= isNumber(elements[2]) && 0 < isNumber(elements[2]))
        {
            flag = 1;
        }
        else if (elements[1] == "O" && led_max >= isNumber(elements[2]) && 0 < isNumber(elements[2]))
        {
            flag = 1;
        }
        else if (elements[1] == "U" && led_max >= isNumber(elements[2]) && 0 < isNumber(elements[2]))
        {
            flag = 1;
        }
        else if (elements[1] == "L" && led_max >= isNumber(elements[2]) && 0 < isNumber(elements[2]))
        {
            flag = 1;
        }
        else if (elements[1] == "c" && led_max >= isNumber(elements[2]) && 0 < isNumber(elements[2]))
        {
            flag = 1;
        }
        else
        {
            flag = 0;
            std::cout << "Invalid command" << std::endl;
            std::cout << "-h to show help menu" << std::endl;
        }
    }
    else if (numElements == 3 && (elements[0] == "d" || elements[0] == "r" || elements[0] == "o" || elements[0] == "a" || elements[0] == "k" || elements[0] == "O" || elements[0] == "U" || elements[0] == "c"))
    {
        flag = 1;
    }
    else if (numElements == 3 && (elements[0] == "s" || elements[0] == "S") && led_max >= isNumber(elements[2]) && 0 < isNumber(elements[2]))
    {
        flag = 1;
    }
    else if (numElements == 4 && elements[0] == "g" && elements[1] == "b" && led_max >= isNumber(elements[2]) && 0 < isNumber(elements[2]))
    {
        flag = 1;
    }
    else if (numElements == 1 && elements[0] == "r")
    {
        flag = 1;
    }
    else if (numElements == 1 && elements[0] == "F")
    {
        flag = 1;
    }
    else if (numElements == 1 && elements[0] == "-h")
    {
        help();
    }
    else if (numElements == 1 && elements[0] == "exit")
    {
        flag = -1;
    }
    else
    {
        flag = 0;
        std::cout << "Invalid command\n-h to show help menu\n"
                  << std::endl;
    }
    // Output the separated elements
    /*std::cout << "Number of elements: " << numElements << std::endl;
    for (const auto &el : elements)
    {
        std::cout << el << std::endl;
    }*/
    return flag;
}
