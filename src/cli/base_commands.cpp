/**
 * @file base_commands.cpp
 * @brief Base Commands
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/cli/base_commands.h>
#include <neo/cli/console_helper.h>

#include <algorithm>
#include <iostream>

namespace neo::cli
{
BaseCommands::BaseCommands(MainService& service) : service_(service) {}

void BaseCommands::RegisterCommands()
{
    service_.RegisterCommand("help", [this](const std::vector<std::string>& args) { return HandleHelp(args); }, "Base");

    service_.RegisterCommand("exit", [this](const std::vector<std::string>& args) { return HandleExit(args); }, "Base");

    service_.RegisterCommand(
        "clear", [this](const std::vector<std::string>& args) { return HandleClear(args); }, "Base");

    service_.RegisterCommand(
        "version", [this](const std::vector<std::string>& args) { return HandleVersion(args); }, "Base");
}

bool BaseCommands::HandleHelp(const std::vector<std::string>& args)
{
    if (args.empty())
    {
        service_.OnHelp();
    }
    else
    {
        service_.OnHelp(args[0]);
    }
    return true;
}

bool BaseCommands::HandleExit(const std::vector<std::string>& args)
{
    service_.OnExit();
    return true;
}

bool BaseCommands::HandleClear(const std::vector<std::string>& args)
{
    service_.OnClear();
    return true;
}

bool BaseCommands::HandleVersion(const std::vector<std::string>& args)
{
    service_.OnVersion();
    return true;
}
}  // namespace neo::cli
