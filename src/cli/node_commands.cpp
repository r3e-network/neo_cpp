#include <neo/cli/console_helper.h>
#include <neo/cli/node_commands.h>

#include <iostream>

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
    service_.OnShowPool();
    return true;
}

bool NodeCommands::HandleShowPeers(const std::vector<std::string>& args)
{
    service_.OnShowPeers();
    return true;
}
}  // namespace neo::cli
