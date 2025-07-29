#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace neo::cli
{

class CLIService;
class CommandRegistry;

/**
 * @brief Console service for handling user input and command execution
 */
class ConsoleServiceNeo
{
  public:
    explicit ConsoleServiceNeo(CLIService* cli_service);
    ~ConsoleServiceNeo();

    // Main console loop
    void ProcessCommands();

    // Command handling
    void ExecuteCommand(const std::string& input);

    // Output methods
    void WriteLine(const std::string& text);
    void Write(const std::string& text);
    void WriteError(const std::string& text);
    void WriteWarning(const std::string& text);
    void WriteInfo(const std::string& text);
    void WriteSuccess(const std::string& text);

    // Input methods
    std::string ReadLine();
    std::string ReadPassword();
    bool Confirm(const std::string& prompt);

    // Utility
    void Clear();
    void SetPrompt(const std::string& prompt);
    std::string GetPrompt() const
    {
        return prompt_;
    }

    // Auto-completion support
    std::vector<std::string> GetCompletions(const std::string& partial);

  private:
    CLIService* cli_service_;
    CommandRegistry* command_registry_;
    std::string prompt_ = "neo> ";

    // Parse command line into command and arguments
    std::pair<std::string, std::vector<std::string>> ParseCommandLine(const std::string& line);

    // History management
    std::vector<std::string> history_;
    size_t history_index_ = 0;
    void AddToHistory(const std::string& command);
    std::string GetFromHistory(int offset);

    // Color output helpers
    std::string ColorText(const std::string& text, const std::string& color_code);

    // ANSI color codes
    static constexpr const char* RESET = "\033[0m";
    static constexpr const char* RED = "\033[31m";
    static constexpr const char* GREEN = "\033[32m";
    static constexpr const char* YELLOW = "\033[33m";
    static constexpr const char* BLUE = "\033[34m";
    static constexpr const char* MAGENTA = "\033[35m";
    static constexpr const char* CYAN = "\033[36m";
    static constexpr const char* WHITE = "\033[37m";
    static constexpr const char* BOLD = "\033[1m";
};

}  // namespace neo::cli