#pragma once

#include <neo/cli/main_service.h>
#include <neo/io/uint256.h>
#include <string>
#include <vector>

namespace neo::cli
{
    /**
     * @brief Blockchain commands for the CLI.
     */
    class BlockchainCommands
    {
    public:
        /**
         * @brief Constructs a BlockchainCommands.
         * @param service The main service.
         */
        explicit BlockchainCommands(MainService& service);

        /**
         * @brief Registers the commands.
         */
        void RegisterCommands();

        /**
         * @brief Handles the showblock command.
         * @param args The arguments.
         * @return True if the command was executed successfully, false otherwise.
         */
        bool HandleShowBlock(const std::vector<std::string>& args);

        /**
         * @brief Handles the showheader command.
         * @param args The arguments.
         * @return True if the command was executed successfully, false otherwise.
         */
        bool HandleShowHeader(const std::vector<std::string>& args);

        /**
         * @brief Handles the showtx command.
         * @param args The arguments.
         * @return True if the command was executed successfully, false otherwise.
         */
        bool HandleShowTransaction(const std::vector<std::string>& args);

    private:
        MainService& service_;
    };
}
