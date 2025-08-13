/**
 * @file console_service_base.h
 * @brief Service implementations
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/console_service/command_token.h>
#include <neo/console_service/console_command_attribute.h>
#include <neo/console_service/console_command_method.h>
#include <neo/console_service/console_helper.h>

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace neo::console_service
{
/**
 * @brief Base class for console services.
 */
class ConsoleServiceBase
{
   public:
    /**
     * @brief Constructor.
     */
    ConsoleServiceBase();

    /**
     * @brief Virtual destructor.
     */
    virtual ~ConsoleServiceBase() = default;

    /**
     * @brief Gets the service name.
     * @return The service name.
     */
    virtual std::string GetServiceName() const = 0;

    /**
     * @brief Gets the service dependencies.
     * @return The dependencies string, or empty if none.
     */
    virtual std::string GetDepends() const;

    /**
     * @brief Gets the prompt string.
     * @return The prompt string.
     */
    virtual std::string GetPrompt() const;

    /**
     * @brief Gets whether to show the prompt.
     * @return True if prompt should be shown.
     */
    bool GetShowPrompt() const;

    /**
     * @brief Sets whether to show the prompt.
     * @param show_prompt Whether to show the prompt.
     */
    void SetShowPrompt(bool show_prompt);

    /**
     * @brief Called when the service starts.
     * @param args Command line arguments.
     * @return True if startup was successful.
     */
    virtual bool OnStart(const std::vector<std::string>& args);

    /**
     * @brief Called when the service stops.
     */
    virtual void OnStop();

    /**
     * @brief Runs the service.
     * @param args Command line arguments.
     */
    void Run(const std::vector<std::string>& args);

    /**
     * @brief Registers a command handler.
     * @param instance The instance containing the command methods.
     * @param name Optional name for the instance.
     */
    void RegisterCommand(std::shared_ptr<void> instance, const std::string& name = "");

    /**
     * @brief Registers a type handler for command parsing.
     * @tparam T The type to register a handler for.
     * @param handler The handler function.
     */
    template <typename T>
    void RegisterCommandHandler(std::function<T(std::vector<std::shared_ptr<CommandToken>>&, bool)> handler);

   protected:
    /**
     * @brief Processes a command.
     * @param command_line The command line to process.
     * @return True if the command was processed successfully.
     */
    bool OnCommand(const std::string& command_line);

    /**
     * @brief Processes the "help" command.
     * @param key Optional key to get help for specific command.
     */
    void OnHelpCommand(const std::string& key = "");

    /**
     * @brief Processes the "clear" command.
     */
    void OnClear();

    /**
     * @brief Processes the "version" command.
     */
    void OnVersion();

    /**
     * @brief Processes the "exit" command.
     */
    void OnExit();

#ifdef _WIN32
    /**
     * @brief Installs the service as a Windows service.
     * @return True if installation was successful.
     */
    bool InstallWindowsService();

    /**
     * @brief Uninstalls the Windows service.
     * @return True if uninstallation was successful.
     */
    bool UninstallWindowsService();
#endif

   private:
    static constexpr int HistorySize = 100;

    bool show_prompt_;
    std::atomic<bool> running_;
    std::vector<std::string> command_history_;

    std::map<std::string, std::vector<ConsoleCommandMethod>> verbs_;
    std::map<std::string, std::shared_ptr<void>> instances_;
    std::map<const std::type_info*, std::function<void*(std::vector<std::shared_ptr<CommandToken>>&, bool)>> handlers_;

    /**
     * @brief Runs the console loop.
     */
    void RunConsole();

    /**
     * @brief Reads a command from the console.
     * @return The command string, or empty string to exit.
     */
    std::string ReadTask();

    /**
     * @brief Tries to process a value from tokens.
     * @param parameter_type The expected parameter type.
     * @param args The token arguments.
     * @param can_consume_all Whether this parameter can consume all remaining tokens.
     * @param value Output parameter for the parsed value.
     * @return True if parsing was successful.
     */
    bool TryProcessValue(const std::type_info& parameter_type, std::vector<std::shared_ptr<CommandToken>>& args,
                         bool can_consume_all, void*& value);

    /**
     * @brief Triggers graceful shutdown.
     */
    void TriggerGracefulShutdown();

    /**
     * @brief Signal handler for SIGTERM.
     */
    static void SigTermEventHandler(int signal);

    /**
     * @brief Signal handler for SIGINT (Ctrl+C).
     */
    static void CancelHandler(int signal);

    /**
     * @brief Registers default command handlers.
     */
    void RegisterDefaultHandlers();
};

template <typename T>
void ConsoleServiceBase::RegisterCommandHandler(
    std::function<T(std::vector<std::shared_ptr<CommandToken>>&, bool)> handler)
{
    handlers_[&typeid(T)] = [handler](std::vector<std::shared_ptr<CommandToken>>& args, bool consume_all) -> void*
    {
        T result = handler(args, consume_all);
        return new T(std::move(result));
    };
}
}  // namespace neo::console_service
