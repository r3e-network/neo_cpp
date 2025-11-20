/**
 * @file node_commands.cpp
 * @brief Node Commands
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/cli/console_helper.h>
#include <neo/cli/node_commands.h>

#include <algorithm>
#include <cctype>

namespace
{
bool IsVerboseArgument(const std::vector<std::string>& args)
{
    if (args.empty())
        return false;

    std::string normalized = args[0];
    normalized.erase(normalized.begin(),
                     std::find_if(normalized.begin(), normalized.end(), [](unsigned char c) { return c != '-'; }));
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    return normalized == "verbose" || normalized == "v" || normalized == "true" || normalized == "1";
}
}  // namespace

namespace neo::cli
{
NodeCommands::NodeCommands(MainService& service) : service_(service) {}

void NodeCommands::RegisterCommands()
{
    service_.RegisterCommand(
        "showstate", [this](const std::vector<std::string>& args) { return HandleShowState(args); }, "Node");

    service_.RegisterCommand(
        "showpool", [this](const std::vector<std::string>& args) { return HandleShowPool(args); }, "Node");

    service_.RegisterCommand(
        "showpeers", [this](const std::vector<std::string>& args) { return HandleShowPeers(args); }, "Node");
}

bool NodeCommands::HandleShowState(const std::vector<std::string>& args)
{
    service_.OnShowState();
    return true;
}

bool NodeCommands::HandleShowPool(const std::vector<std::string>& args)
{
    service_.OnShowPool(IsVerboseArgument(args));
    return true;
}

bool NodeCommands::HandleShowPeers(const std::vector<std::string>& args)
{
    service_.OnShowPeers();
    return true;
}
}  // namespace neo::cli
