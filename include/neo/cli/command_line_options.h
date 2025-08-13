/**
 * @file command_line_options.h
 * @brief Command Line Options
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <string>
#include <vector>

namespace neo::cli
{
/**
 * @brief Command line options for the CLI.
 */
class CommandLineOptions
{
   public:
    /**
     * @brief The config file path.
     */
    std::string Config;

    /**
     * @brief The wallet file path.
     */
    std::string Wallet;

    /**
     * @brief The wallet password.
     */
    std::string Password;

    /**
     * @brief The database engine.
     */
    std::string DbEngine;

    /**
     * @brief The database path.
     */
    std::string DbPath;

    /**
     * @brief Whether to skip verification when importing blocks.
     */
    bool NoVerify = false;

    /**
     * @brief The plugins to load.
     */
    std::vector<std::string> Plugins;

    /**
     * @brief The log level.
     */
    int Verbose = 0;

    /**
     * @brief Constructs a CommandLineOptions.
     */
    CommandLineOptions() = default;

    /**
     * @brief Checks if the options are valid.
     * @return True if the options are valid, false otherwise.
     */
    bool IsValid() const
    {
        return !Config.empty() || !Wallet.empty() || !Password.empty() || !DbEngine.empty() || !DbPath.empty() ||
               !Plugins.empty() || NoVerify;
    }
};
}  // namespace neo::cli
