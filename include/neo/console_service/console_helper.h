/**
 * @file console_helper.h
 * @brief Helper utilities
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <iostream>
#include <string>
#include <vector>

namespace neo::console_service
{
/**
 * @brief Console color enumeration.
 */
enum class ConsoleColor
{
    Black = 0,
    DarkBlue = 1,
    DarkGreen = 2,
    DarkCyan = 3,
    DarkRed = 4,
    DarkMagenta = 5,
    DarkYellow = 6,
    Gray = 7,
    DarkGray = 8,
    Blue = 9,
    Green = 10,
    Cyan = 11,
    Red = 12,
    Magenta = 13,
    Yellow = 14,
    White = 15
};

/**
 * @brief Console color set for managing foreground and background colors.
 */
class ConsoleColorSet
{
   public:
    /**
     * @brief Default constructor using current console colors.
     */
    ConsoleColorSet();

    /**
     * @brief Constructor with foreground color.
     * @param foreground The foreground color.
     */
    explicit ConsoleColorSet(ConsoleColor foreground);

    /**
     * @brief Constructor with foreground and background colors.
     * @param foreground The foreground color.
     * @param background The background color.
     */
    ConsoleColorSet(ConsoleColor foreground, ConsoleColor background);

    /**
     * @brief Applies the color set to the console.
     */
    void Apply() const;

   private:
    ConsoleColor foreground_;
    ConsoleColor background_;
};

/**
 * @brief Helper class for console operations.
 */
class ConsoleHelper
{
   public:
    /**
     * @brief Gets whether currently reading a password.
     * @return True if reading password, false otherwise.
     */
    static bool IsReadingPassword();

    /**
     * @brief Info handles message in the format of "[tag]:[message]".
     * Avoid using Info if the `tag` is too long.
     * @param values The log message in pairs of (tag, message).
     */
    static void Info(const std::vector<std::string>& values);

    /**
     * @brief Info with tag and message.
     * @param tag The tag.
     * @param message The message.
     */
    static void Info(const std::string& tag, const std::string& message);

    /**
     * @brief Use warning if something unexpected happens
     * or the execution result is not correct.
     * Also use warning if you just want to remind user of doing something.
     * @param msg Warning message.
     */
    static void Warning(const std::string& msg);

    /**
     * @brief Use Error if the verification or input format check fails
     * or exception that breaks the execution of interactive command throws.
     * @param msg Error message.
     */
    static void Error(const std::string& msg);

    /**
     * @brief Reads user input with optional prompt and password masking.
     * @param prompt The prompt to display.
     * @param password Whether to mask input as password.
     * @return The user input.
     */
    static std::string ReadUserInput(const std::string& prompt = "", bool password = false);

    /**
     * @brief Reads a secure string (password) with masking.
     * @param prompt The prompt to display.
     * @return The secure input.
     */
    static std::string ReadSecureString(const std::string& prompt = "");

    /**
     * @brief Sets the console foreground color.
     * @param color The color to set.
     */
    static void SetForegroundColor(ConsoleColor color);

    /**
     * @brief Sets the console background color.
     * @param color The color to set.
     */
    static void SetBackgroundColor(ConsoleColor color);

    /**
     * @brief Resets console colors to default.
     */
    static void ResetColor();

    /**
     * @brief Clears the console screen.
     */
    static void Clear();

    /**
     * @brief Gets the current console foreground color.
     * @return The current foreground color.
     */
    static ConsoleColor GetCurrentForegroundColor();

    /**
     * @brief Gets the current console background color.
     * @return The current background color.
     */
    static ConsoleColor GetCurrentBackgroundColor();

   private:
    static bool reading_password_;
    static ConsoleColorSet info_color_;
    static ConsoleColorSet warning_color_;
    static ConsoleColorSet error_color_;

    /**
     * @brief Internal logging function.
     * @param tag The log tag.
     * @param color_set The color set to use.
     * @param msg The message.
     */
    static void Log(const std::string& tag, const ConsoleColorSet& color_set, const std::string& msg);
};
}  // namespace neo::console_service
