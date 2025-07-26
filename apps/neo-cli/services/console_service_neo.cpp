#include "console_service_neo.h"
#include "../cli_service.h"
#include "../commands/command_registry.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <termios.h>
#include <unistd.h>

namespace neo::cli {

ConsoleServiceNeo::ConsoleServiceNeo(CLIService* cli_service) 
    : cli_service_(cli_service) {
    // Get command registry from CLI service
    // command_registry_ will be set when available
}

ConsoleServiceNeo::~ConsoleServiceNeo() = default;

void ConsoleServiceNeo::ProcessCommands() {
    std::string input = ReadLine();
    
    if (!input.empty()) {
        AddToHistory(input);
        ExecuteCommand(input);
    }
}

void ConsoleServiceNeo::ExecuteCommand(const std::string& input) {
    auto [command, args] = ParseCommandLine(input);
    
    if (command.empty()) {
        return;
    }
    
    // Special handling for built-in commands that don't go through registry
    if (command == "clear" || command == "cls") {
        Clear();
        return;
    }
    
    if (command == "history") {
        for (size_t i = 0; i < history_.size(); ++i) {
            std::cout << std::setw(4) << i + 1 << "  " << history_[i] << std::endl;
        }
        return;
    }
    
    // Execute through command registry
    if (command_registry_) {
        command_registry_->ExecuteCommand(command, args);
    } else {
        // Fallback for basic commands when registry not available
        if (command == "help") {
            cli_service_->DisplayHelp();
        } else if (command == "status") {
            cli_service_->DisplayStatus();
        } else if (command == "exit" || command == "quit") {
            cli_service_->Stop();
        } else {
            WriteError("Unknown command: " + command);
        }
    }
}

void ConsoleServiceNeo::WriteLine(const std::string& text) {
    std::cout << text << std::endl;
}

void ConsoleServiceNeo::Write(const std::string& text) {
    std::cout << text << std::flush;
}

void ConsoleServiceNeo::WriteError(const std::string& text) {
    std::cout << ColorText("Error: " + text, RED) << std::endl;
}

void ConsoleServiceNeo::WriteWarning(const std::string& text) {
    std::cout << ColorText("Warning: " + text, YELLOW) << std::endl;
}

void ConsoleServiceNeo::WriteInfo(const std::string& text) {
    std::cout << ColorText(text, CYAN) << std::endl;
}

void ConsoleServiceNeo::WriteSuccess(const std::string& text) {
    std::cout << ColorText(text, GREEN) << std::endl;
}

std::string ConsoleServiceNeo::ReadLine() {
    Write(prompt_);
    
    std::string line;
    std::getline(std::cin, line);
    
    // Trim whitespace
    line.erase(0, line.find_first_not_of(" \t\n\r"));
    line.erase(line.find_last_not_of(" \t\n\r") + 1);
    
    return line;
}

std::string ConsoleServiceNeo::ReadPassword() {
    Write("Password: ");
    
    // Disable echo for password input
    termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
    termios newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    std::string password;
    std::getline(std::cin, password);
    
    // Restore terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    std::cout << std::endl;
    
    return password;
}

bool ConsoleServiceNeo::Confirm(const std::string& prompt) {
    Write(prompt + " (y/n): ");
    
    std::string response;
    std::getline(std::cin, response);
    
    // Convert to lowercase and check
    std::transform(response.begin(), response.end(), response.begin(), ::tolower);
    return response == "y" || response == "yes";
}

void ConsoleServiceNeo::Clear() {
    // Clear screen using ANSI escape codes
    std::cout << "\033[2J\033[1;1H" << std::flush;
}

void ConsoleServiceNeo::SetPrompt(const std::string& prompt) {
    prompt_ = prompt;
}

std::vector<std::string> ConsoleServiceNeo::GetCompletions(const std::string& partial) {
    std::vector<std::string> completions;
    
    if (!command_registry_) {
        return completions;
    }
    
    // Get all command names
    auto commands = command_registry_->GetCommandNames();
    
    // Filter by prefix
    for (const auto& cmd : commands) {
        if (cmd.find(partial) == 0) {
            completions.push_back(cmd);
        }
    }
    
    std::sort(completions.begin(), completions.end());
    return completions;
}

std::pair<std::string, std::vector<std::string>> ConsoleServiceNeo::ParseCommandLine(const std::string& line) {
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;
    
    // Simple tokenization (could be enhanced to handle quotes)
    while (ss >> token) {
        tokens.push_back(token);
    }
    
    if (tokens.empty()) {
        return {"", {}};
    }
    
    std::string command = tokens[0];
    std::vector<std::string> args(tokens.begin() + 1, tokens.end());
    
    return {command, args};
}

void ConsoleServiceNeo::AddToHistory(const std::string& command) {
    // Don't add empty commands or duplicates of the last command
    if (!command.empty() && (history_.empty() || history_.back() != command)) {
        history_.push_back(command);
        
        // Limit history size
        if (history_.size() > 1000) {
            history_.erase(history_.begin());
        }
    }
    
    history_index_ = history_.size();
}

std::string ConsoleServiceNeo::GetFromHistory(int offset) {
    if (history_.empty()) {
        return "";
    }
    
    int index = static_cast<int>(history_index_) + offset;
    if (index < 0) {
        index = 0;
    } else if (index >= static_cast<int>(history_.size())) {
        index = history_.size() - 1;
    }
    
    history_index_ = index;
    return history_[index];
}

std::string ConsoleServiceNeo::ColorText(const std::string& text, const std::string& color_code) {
    // Check if terminal supports colors (simplified check)
    const char* term = getenv("TERM");
    if (!term || std::string(term) == "dumb") {
        return text;
    }
    
    return color_code + text + RESET;
}

} // namespace neo::cli