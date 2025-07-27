#pragma once

#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace neo::console_service
{
/**
 * @brief Represents a console command method with execution capabilities.
 *
 * ## Overview
 * The ConsoleCommandMethod class encapsulates a command that can be executed
 * in the Neo console, including its metadata and execution function.
 *
 * ## API Reference
 * - **Metadata**: Command key, help category, help message
 * - **Execution**: Function pointer for command execution
 * - **Validation**: Parameter validation and help display
 *
 * ## Usage Examples
 * ```cpp
 * // Create a command method
 * auto cmd = ConsoleCommandMethod("exit", "System", "Exit the application",
 *     [](const std::vector<std::string>& args) -> std::string {
 *         return "Exiting...";
 *     });
 *
 * // Execute the command
 * auto result = cmd.Execute({"exit"});
 * ```
 */
class ConsoleCommandMethod
{
  public:
    using CommandFunction = std::function<std::string(const std::vector<std::string>&)>;

    /**
     * @brief Command key/name
     */
    std::string key;

    /**
     * @brief Help category for grouping commands
     */
    std::string help_category;

    /**
     * @brief Help message describing the command
     */
    std::string help_message;

    /**
     * @brief Default constructor
     */
    ConsoleCommandMethod() = default;

    /**
     * @brief Constructor with all parameters
     * @param cmd_key The command key
     * @param category The help category
     * @param message The help message
     * @param func The command function
     */
    ConsoleCommandMethod(const std::string& cmd_key, const std::string& category, const std::string& message,
                         CommandFunction func)
        : key(cmd_key), help_category(category), help_message(message), function_(func)
    {
    }

    /**
     * @brief Execute the command with given arguments
     * @param args Command arguments
     * @return Command execution result
     */
    std::string Execute(const std::vector<std::string>& args) const
    {
        if (function_)
        {
            return function_(args);
        }
        throw std::runtime_error("Command '" + key + "' has no implementation");
    }

    /**
     * @brief Check if the command has an implementation
     * @return True if command can be executed
     */
    bool IsImplemented() const
    {
        return static_cast<bool>(function_);
    }

    /**
     * @brief Get formatted help string
     * @return Formatted help text
     */
    std::string GetHelp() const
    {
        return key + " - " + help_message + " (Category: " + help_category + ")";
    }

    /**
     * @brief Set the command function
     * @param func The command function
     */
    void SetFunction(CommandFunction func)
    {
        function_ = func;
    }

  private:
    CommandFunction function_;
};
}  // namespace neo::console_service
