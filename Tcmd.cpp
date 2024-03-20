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

int main()
{
    std::string input; // Declare input here

    while (1)
    {
        std::cout << "Enter command: ";
        std::getline(std::cin, input);
        std::istringstream iss(input);
        std::vector<std::string> elements;
        std::string element;

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
                std::cout << "Entrou no g d" << std::endl;
            }
            else if (elements[1] == "r")
            {
                std::cout << "Entrou no g r" << std::endl;
            }
            else if (elements[1] == "l")
            {
                std::cout << "Entrou no g l" << std::endl;
            }
            else if (elements[1] == "o")
            {
                std::cout << "Entrou no g o" << std::endl;
            }
            else if (elements[1] == "a")
            {
                std::cout << "Entrou no g a" << std::endl;
            }
            else if (elements[1] == "k")
            {
                std::cout << "Entrou no g k" << std::endl;
            }
            else if (elements[1] == "x")
            {
                std::cout << "Entrou no g x" << std::endl;
            }
            else if (elements[1] == "p")
            {
                std::cout << "Entrou no g p" << std::endl;
            }
            else if (elements[1] == "t")
            {
                std::cout << "Entrou no g t" << std::endl;
            }
            else if (elements[1] == "e")
            {
                std::cout << "Entrou no g e" << std::endl;
            }
            else if (elements[1] == "v")
            {
                std::cout << "Entrou no g v" << std::endl;
            }
            else if (elements[1] == "f")
            {
                std::cout << "Entrou no g f" << std::endl;
            }
            else if (elements[1] == "O")
            {
                std::cout << "Entrou no g O" << std::endl;
            }
            else if (elements[1] == "U")
            {
                std::cout << "Entrou no g U" << std::endl;
            }
            else if (elements[1] == "L")
            {
                std::cout << "Entrou no g L" << std::endl;
            }
            else if (elements[1] == "c")
            {
                std::cout << "Entrou no g c" << std::endl;
            }
            else
            {
                std::cout << "Invalid command" << std::endl;
                std::cout << "-h to show help menu" << std::endl;
                continue;
            }
        }
        else if (numElements == 3 && (elements[0] == "d" || elements[0] == "r" || elements[0] == "o" || elements[0] == "a" || elements[0] == "k" || elements[0] == "O" || elements[0] == "U" || elements[0] == "c"))
        {
            std::cout << "Entrou no r e coisas" << std::endl;
        }
        else if (numElements == 3 && (elements[0] == "s") || elements[0] == "S")
        {
            std::cout << "Entrou no s e S" << std::endl;
        }
        else if (numElements == 4 && elements[0] == "g" && elements[1] == "b")
        {
            std::cout << "Entrou no g b coisas" << std::endl;
        }
        else if (numElements == 1 && elements[0] == "r")
        {
            std::cout << "Entrou no r (reset)" << std::endl;
        }
        else if (numElements == 1 && elements[0] == "-h")
        {
            std::cout << "Entrou no -h (help)" << std::endl;
        }
        else
        {
            std::cout << "Invalid command" << std::endl;
            std::cout << "-h to show help menu" << std::endl;
            continue;
        }

        // Output the separated elements
        std::cout << "Number of elements: " << numElements << std::endl;
        for (const auto &el : elements)
        {
            std::cout << el << std::endl;
        }
    }
    return 0;
}
