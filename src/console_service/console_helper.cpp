#include <iostream>
#include <neo/console_service/console_helper.h>
#include <sstream>
#ifdef _WIN32
#include <conio.h>  // For Windows _getch()
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace neo::console_service
{
// Static member definitions
bool ConsoleHelper::reading_password_ = false;
ConsoleColorSet ConsoleHelper::info_color_(ConsoleColor::Cyan);
ConsoleColorSet ConsoleHelper::warning_color_(ConsoleColor::Yellow);
ConsoleColorSet ConsoleHelper::error_color_(ConsoleColor::Red);

// ConsoleColorSet implementation
ConsoleColorSet::ConsoleColorSet()
    : foreground_(ConsoleHelper::GetCurrentForegroundColor()), background_(ConsoleHelper::GetCurrentBackgroundColor())
{
}

ConsoleColorSet::ConsoleColorSet(ConsoleColor foreground)
    : foreground_(foreground), background_(ConsoleHelper::GetCurrentBackgroundColor())
{
}

ConsoleColorSet::ConsoleColorSet(ConsoleColor foreground, ConsoleColor background)
    : foreground_(foreground), background_(background)
{
}

void ConsoleColorSet::Apply() const
{
    ConsoleHelper::SetForegroundColor(foreground_);
    ConsoleHelper::SetBackgroundColor(background_);
}

// ConsoleHelper implementation
bool ConsoleHelper::IsReadingPassword()
{
    return reading_password_;
}

void ConsoleHelper::Info(const std::vector<std::string>& values)
{
    ConsoleColorSet current_color;

    for (size_t i = 0; i < values.size(); ++i)
    {
        if (i % 2 == 0)
            info_color_.Apply();
        else
            current_color.Apply();
        std::cout << values[i];
    }
    current_color.Apply();
    std::cout << std::endl;
}

void ConsoleHelper::Info(const std::string& tag, const std::string& message)
{
    Info({tag, message});
}

void ConsoleHelper::Warning(const std::string& msg)
{
    Log("Warning", warning_color_, msg);
}

void ConsoleHelper::Error(const std::string& msg)
{
    Log("Error", error_color_, msg);
}

void ConsoleHelper::Log(const std::string& tag, const ConsoleColorSet& color_set, const std::string& msg)
{
    ConsoleColorSet current_color;

    color_set.Apply();
    std::cout << tag << ": ";
    current_color.Apply();
    std::cout << msg << std::endl;
}

std::string ConsoleHelper::ReadUserInput(const std::string& prompt, bool password)
{
    const std::string valid_chars =
        " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
    std::string input;

    if (!prompt.empty())
    {
        std::cout << prompt << ": ";
    }

    if (password)
        reading_password_ = true;

    ConsoleColor prev_color = GetCurrentForegroundColor();
    SetForegroundColor(ConsoleColor::Yellow);

#ifdef _WIN32
    // Windows implementation
    if (std::cin.rdbuf()->in_avail() > 0)
    {
        // Input is redirected (e.g., from neo-gui)
        std::getline(std::cin, input);
    }
    else
    {
        char ch;
        while ((ch = _getch()) != '\r')  // Enter key
        {
            if (valid_chars.find(ch) != std::string::npos)
            {
                input += ch;
                std::cout << (password ? '*' : ch);
            }
            else if (ch == '\b' && !input.empty())  // Backspace
            {
                input.pop_back();
                std::cout << "\b \b";
            }
        }
    }
#else
    // Unix/Linux implementation
    if (isatty(STDIN_FILENO))
    {
        struct termios old_termios, new_termios;
        tcgetattr(STDIN_FILENO, &old_termios);
        new_termios = old_termios;
        new_termios.c_lflag &= ~(ECHO | ICANON);
        tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

        char ch;
        while ((ch = getchar()) != '\n')
        {
            if (valid_chars.find(ch) != std::string::npos)
            {
                input += ch;
                std::cout << (password ? '*' : ch);
            }
            else if (ch == 127 && !input.empty())  // Backspace (DEL)
            {
                input.pop_back();
                std::cout << "\b \b";
            }
        }

        tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
    }
    else
    {
        std::getline(std::cin, input);
    }
#endif

    SetForegroundColor(prev_color);
    if (password)
        reading_password_ = false;
    std::cout << std::endl;

    return input;
}

std::string ConsoleHelper::ReadSecureString(const std::string& prompt)
{
    return ReadUserInput(prompt, true);
}

void ConsoleHelper::SetForegroundColor(ConsoleColor color)
{
#ifdef _WIN32
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(console, &info);
    SetConsoleTextAttribute(console, (info.wAttributes & 0xF0) | static_cast<WORD>(color));
#else
    // ANSI color codes for Unix/Linux
    int ansi_color = 30 + static_cast<int>(color) % 8;
    if (static_cast<int>(color) >= 8)
        ansi_color += 60;  // Bright colors
    std::cout << "\033[" << ansi_color << "m";
#endif
}

void ConsoleHelper::SetBackgroundColor(ConsoleColor color)
{
#ifdef _WIN32
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(console, &info);
    SetConsoleTextAttribute(console, (info.wAttributes & 0x0F) | (static_cast<WORD>(color) << 4));
#else
    // ANSI color codes for Unix/Linux
    int ansi_color = 40 + static_cast<int>(color) % 8;
    if (static_cast<int>(color) >= 8)
        ansi_color += 60;  // Bright colors
    std::cout << "\033[" << ansi_color << "m";
#endif
}

void ConsoleHelper::ResetColor()
{
#ifdef _WIN32
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(console, 7);  // Default: Gray on Black
#else
    std::cout << "\033[0m";
#endif
}

void ConsoleHelper::Clear()
{
    // Use ANSI escape sequence instead of system() calls for security
    // This works on Windows 10+, macOS, and Linux terminals
    std::cout << "\033[2J\033[H" << std::flush;

    // Fallback for older Windows versions
#ifdef _WIN32
    // Try Windows Console API first
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole != INVALID_HANDLE_VALUE)
    {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(hConsole, &csbi))
        {
            DWORD cellCount = csbi.dwSize.X * csbi.dwSize.Y;
            COORD homeCoords = {0, 0};
            DWORD count;

            // Fill with spaces
            FillConsoleOutputCharacter(hConsole, ' ', cellCount, homeCoords, &count);
            // Reset attributes
            FillConsoleOutputAttribute(hConsole, csbi.wAttributes, cellCount, homeCoords, &count);
            // Move cursor to top-left
            SetConsoleCursorPosition(hConsole, homeCoords);
        }
    }
#endif
}

ConsoleColor ConsoleHelper::GetCurrentForegroundColor()
{
#ifdef _WIN32
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(console, &info);
    return static_cast<ConsoleColor>(info.wAttributes & 0x0F);
#else
    return ConsoleColor::White;  // Default for Unix/Linux
#endif
}

ConsoleColor ConsoleHelper::GetCurrentBackgroundColor()
{
#ifdef _WIN32
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(console, &info);
    return static_cast<ConsoleColor>((info.wAttributes & 0xF0) >> 4);
#else
    return ConsoleColor::Black;  // Default for Unix/Linux
#endif
}
}  // namespace neo::console_service
