#pragma once

#include <neo/cli/main_service.h>

#include <string>
#include <vector>

namespace neo::cli
{
/**
 * @brief Base commands for the CLI.
 */
class BaseCommands
{
   public:
    /**
     * @brief Constructs a BaseCommands.
     * @param service The main service.
     */
    explicit BaseCommands(MainService& service);

    /**
     * @brief Registers the commands.
     */
    void RegisterCommands();

    /**
     * @brief Handles the help command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleHelp(const std::vector<std::string>& args);

    /**
     * @brief Handles the exit command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleExit(const std::vector<std::string>& args);

    /**
     * @brief Handles the clear command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleClear(const std::vector<std::string>& args);

    /**
     * @brief Handles the version command.
     * @param args The arguments.
     * @return True if the command was executed successfully, false otherwise.
     */
    bool HandleVersion(const std::vector<std::string>& args);

   private:
    MainService& service_;
};
}  // namespace neo::cli
