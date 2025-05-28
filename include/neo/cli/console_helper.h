#pragma once

#include <string>
#include <iostream>

namespace neo::cli
{
    /**
     * @brief Helper class for console output.
     */
    class ConsoleHelper
    {
    public:
        /**
         * @brief Writes an info message to the console.
         * @param message The message.
         */
        static void Info(const std::string& message)
        {
            std::cout << message << std::endl;
        }

        /**
         * @brief Writes an info message to the console with a prefix.
         * @param prefix The prefix.
         * @param message The message.
         */
        static void Info(const std::string& prefix, const std::string& message)
        {
            std::cout << prefix << message << std::endl;
        }

        /**
         * @brief Writes a warning message to the console.
         * @param message The message.
         */
        static void Warning(const std::string& message)
        {
            std::cout << "WARNING: " << message << std::endl;
        }

        /**
         * @brief Writes an error message to the console.
         * @param message The message.
         */
        static void Error(const std::string& message)
        {
            std::cerr << "ERROR: " << message << std::endl;
        }

        /**
         * @brief Reads a line from the console.
         * @param prompt The prompt.
         * @return The line.
         */
        static std::string ReadLine(const std::string& prompt = "")
        {
            if (!prompt.empty())
            {
                std::cout << prompt;
            }

            std::string line;
            std::getline(std::cin, line);
            return line;
        }

        /**
         * @brief Reads a password from the console.
         * @param prompt The prompt.
         * @return The password.
         */
        static std::string ReadPassword(const std::string& prompt = "")
        {
            if (!prompt.empty())
            {
                std::cout << prompt;
            }

            std::string password;
            
            // Hide input on Unix-like systems
#ifndef _WIN32
            system("stty -echo");
#endif

            std::getline(std::cin, password);

            // Restore normal terminal behavior
#ifndef _WIN32
            system("stty echo");
#endif

            std::cout << std::endl;
            return password;
        }
    };
}
