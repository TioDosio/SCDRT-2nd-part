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
        if (elements[1] == "d" && led_max > isNumber(elements[2]) && 0 < isNumber(elements[2]))
        {
            flag = 1;
        }
        else if (elements[1] == "r")
        {
            flag = 1;
        }
        else if (elements[1] == "l")
        {
            flag = 1;
        }
        else if (elements[1] == "o")
        {
            flag = 1;
        }
        else if (elements[1] == "a")
        {
            flag = 1;
        }
        else if (elements[1] == "k")
        {
            flag = 1;
        }
        else if (elements[1] == "x")
        {
            flag = 1;
        }
        else if (elements[1] == "p")
        {
            flag = 1;
        }
        else if (elements[1] == "t")
        {
            flag = 1;
        }
        else if (elements[1] == "e")
        {
            flag = 1;
        }
        else if (elements[1] == "v")
        {
            flag = 1;
        }
        else if (elements[1] == "f")
        {
            flag = 1;
        }
        else if (elements[1] == "O")
        {
            flag = 1;
        }
        else if (elements[1] == "U")
        {
            flag = 1;
        }
        else if (elements[1] == "L")
        {
            flag = 1;
        }
        else if (elements[1] == "c")
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
    else if (numElements == 3 && (elements[0] == "s" || elements[0] == "S"))
    {
        flag = 1;
    }
    else if (numElements == 4 && elements[0] == "g" && elements[1] == "b")
    {
        flag = 1;
    }
    else if (numElements == 1 && elements[0] == "r")
    {
        flag = 1;
    }
    else if (numElements == 1 && elements[0] == "-h")
    {
        flag = 1;
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
