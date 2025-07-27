#include <iostream>
#include <neo/cli/blockchain_commands.h>
#include <neo/cli/console_helper.h>
#include <neo/io/uint256.h>
#include <neo/ledger/block.h>
#include <neo/ledger/block_header.h>
#include <neo/ledger/transaction.h>

namespace neo::cli
{
BlockchainCommands::BlockchainCommands(MainService& service) : service_(service) {}

void BlockchainCommands::RegisterCommands()
{
    service_.RegisterCommand(
        "showblock", [this](const std::vector<std::string>& args) { return HandleShowBlock(args); }, "Blockchain");

    service_.RegisterCommand(
        "showheader", [this](const std::vector<std::string>& args) { return HandleShowHeader(args); }, "Blockchain");

    service_.RegisterCommand(
        "showtx", [this](const std::vector<std::string>& args) { return HandleShowTransaction(args); }, "Blockchain");
}

bool BlockchainCommands::HandleShowBlock(const std::vector<std::string>& args)
{
    if (args.empty())
    {
        ConsoleHelper::Error("Missing argument: index or hash");
        return false;
    }

    service_.OnShowBlock(args[0]);
    return true;
}

bool BlockchainCommands::HandleShowHeader(const std::vector<std::string>& args)
{
    if (args.empty())
    {
        ConsoleHelper::Error("Missing argument: index or hash");
        return false;
    }

    service_.OnShowHeader(args[0]);
    return true;
}

bool BlockchainCommands::HandleShowTransaction(const std::vector<std::string>& args)
{
    if (args.empty())
    {
        ConsoleHelper::Error("Missing argument: hash");
        return false;
    }

    try
    {
        io::UInt256 hash = io::UInt256::Parse(args[0]);
        service_.OnShowTransaction(hash);
    }
    catch (const std::exception& ex)
    {
        ConsoleHelper::Error(ex.what());
    }

    return true;
}
}  // namespace neo::cli
