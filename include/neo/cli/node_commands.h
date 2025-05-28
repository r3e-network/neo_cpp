#pragma once

#include <neo/cli/main_service.h>
#include <string>
#include <vector>

namespace neo::cli
{
    /**
     * @brief Node commands for the CLI.
     */
    class NodeCommands
    {
    public:
        /**
         * @brief Constructs a NodeCommands.
         * @param service The main service.
         */
        explicit NodeCommands(MainService& service);

        /**
         * @brief Registers the commands.
         */
        void RegisterCommands();

        /**
         * @brief Handles the showstate command.
         * @param args The arguments.
         * @return True if the command was executed successfully, false otherwise.
         */
        bool HandleShowState(const std::vector<std::string>& args);

        /**
         * @brief Handles the showpool command.
         * @param args The arguments.
         * @return True if the command was executed successfully, false otherwise.
         */
        bool HandleShowPool(const std::vector<std::string>& args);

        /**
         * @brief Handles the showpeers command.
         * @param args The arguments.
         * @return True if the command was executed successfully, false otherwise.
         */
        bool HandleShowPeers(const std::vector<std::string>& args);

    private:
        MainService& service_;
    };
}
