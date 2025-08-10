#include <neo/cli/console_helper.h>

#include <iostream>
#include <string>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace neo::cli
{
void ConsoleHelper::Info(const std::string& message)
{
    SetColor(COLOR_DEFAULT);
    std::cout << message << std::endl;
    ResetColor();
}

void ConsoleHelper::Error(const std::string& message)
{
    SetColor(COLOR_RED);
    std::cerr << "ERROR: " << message << std::endl;
    ResetColor();
}

void ConsoleHelper::Warning(const std::string& message)
{
    SetColor(COLOR_YELLOW);
    std::cout << "WARNING: " << message << std::endl;
    ResetColor();
}

void ConsoleHelper::Success(const std::string& message)
{
    SetColor(COLOR_GREEN);
    std::cout << "SUCCESS: " << message << std::endl;
    ResetColor();
}

std::string ConsoleHelper::ReadLine(const std::string& prompt)
{
    if (!prompt.empty())
    {
        std::cout << prompt;
        std::cout.flush();
    }

    std::string line;
    std::getline(std::cin, line);
    return line;
}

std::string ConsoleHelper::ReadPassword(const std::string& prompt)
{
    if (!prompt.empty())
    {
        std::cout << prompt;
        std::cout.flush();
    }

    std::string password;

#ifdef _WIN32
    // Windows implementation using _getch()
    char ch;
    while ((ch = _getch()) != '\r')
    {
        if (ch == '\b')
        {
            // Handle backspace
            if (!password.empty())
            {
                password.pop_back();
                std::cout << "\b \b";
            }
        }
        else if (ch >= 32 && ch <= 126)
        {
            // Printable character
            password += ch;
            std::cout << '*';
        }
    }
#else
    // Unix/Linux implementation using termios
    struct termios oldTermios, newTermios;

    // Get current terminal settings
    tcgetattr(STDIN_FILENO, &oldTermios);
    newTermios = oldTermios;

    // Disable echo
    newTermios.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &newTermios);

    char ch;
    while ((ch = getchar()) != '\n' && ch != '\r')
    {
        if (ch == 127 || ch == '\b')
        {
            // Handle backspace
            if (!password.empty())
            {
                password.pop_back();
                std::cout << "\b \b";
                std::cout.flush();
            }
        }
        else if (ch >= 32 && ch <= 126)
        {
            // Printable character
            password += ch;
            std::cout << '*';
            std::cout.flush();
        }
    }

    // Restore terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios);
#endif

    std::cout << std::endl;
    return password;
}

void ConsoleHelper::Clear()
{
    // Use ANSI escape sequence instead of system() calls for security
    // This works on Windows 10+, macOS, and Linux terminals
    std::cout << "\033[2J\033[H" << std::flush;
}

void ConsoleHelper::SetColor(int color)
{
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    WORD colorAttribute = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;  // Default white

    switch (color)
    {
        case COLOR_RED:
            colorAttribute = FOREGROUND_RED | FOREGROUND_INTENSITY;
            break;
        case COLOR_GREEN:
            colorAttribute = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            break;
        case COLOR_YELLOW:
            colorAttribute = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            break;
        case COLOR_BLUE:
            colorAttribute = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
            break;
        case COLOR_MAGENTA:
            colorAttribute = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
            break;
        case COLOR_CYAN:
            colorAttribute = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
            break;
        case COLOR_WHITE:
            colorAttribute = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
            break;
        default:
            colorAttribute = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
            break;
    }

    SetConsoleTextAttribute(hConsole, colorAttribute);
#else
    // ANSI color codes for Unix/Linux
    switch (color)
    {
        case COLOR_RED:
            std::cout << "\033[1;31m";
            break;
        case COLOR_GREEN:
            std::cout << "\033[1;32m";
            break;
        case COLOR_YELLOW:
            std::cout << "\033[1;33m";
            break;
        case COLOR_BLUE:
            std::cout << "\033[1;34m";
            break;
        case COLOR_MAGENTA:
            std::cout << "\033[1;35m";
            break;
        case COLOR_CYAN:
            std::cout << "\033[1;36m";
            break;
        case COLOR_WHITE:
            std::cout << "\033[1;37m";
            break;
        default:
            std::cout << "\033[0m";
            break;
    }
#endif
}

void ConsoleHelper::ResetColor()
{
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
    std::cout << "\033[0m";
#endif
}
}  // namespace neo::cli