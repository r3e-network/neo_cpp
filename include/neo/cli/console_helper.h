#pragma once

#include <iostream>
#include <string>

namespace neo::cli
{
/**
 * @brief Console helper utilities for CLI operations
 */
class ConsoleHelper
{
   public:
    /**
     * @brief Print an informational message
     * @param message The message to print
     */
    static void Info(const std::string& message);

    /**
     * @brief Print an error message
     * @param message The error message to print
     */
    static void Error(const std::string& message);

    /**
     * @brief Print a warning message
     * @param message The warning message to print
     */
    static void Warning(const std::string& message);

    /**
     * @brief Print a success message
     * @param message The success message to print
     */
    static void Success(const std::string& message);

    /**
     * @brief Read a line from console with prompt
     * @param prompt The prompt to display
     * @return The input line
     */
    static std::string ReadLine(const std::string& prompt = "");

    /**
     * @brief Read a password from console (hidden input)
     * @param prompt The prompt to display
     * @return The password
     */
    static std::string ReadPassword(const std::string& prompt = "Password: ");

    /**
     * @brief Clear the console screen
     */
    static void Clear();

    /**
     * @brief Set console text color
     * @param color The color code
     */
    static void SetColor(int color);

    /**
     * @brief Reset console text color to default
     */
    static void ResetColor();

   private:
    // Console color codes
    static constexpr int COLOR_DEFAULT = 0;
    static constexpr int COLOR_RED = 1;
    static constexpr int COLOR_GREEN = 2;
    static constexpr int COLOR_YELLOW = 3;
    static constexpr int COLOR_BLUE = 4;
    static constexpr int COLOR_MAGENTA = 5;
    static constexpr int COLOR_CYAN = 6;
    static constexpr int COLOR_WHITE = 7;
};
}  // namespace neo::cli
