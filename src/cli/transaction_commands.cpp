/**
 * @file transaction_commands.cpp
 * @brief Transaction types and processing
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/cli/command_handler.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/neo_token.h>

#include <iostream>
#include <sstream>

namespace neo::cli
{
bool CommandHandler::HandleTransfer(const std::vector<std::string>& args)
{
    if (!wallet_)
    {
        std::cout << "No wallet is open" << std::endl;
        return false;
    }

    if (args.size() < 3)
    {
        std::cout << "Usage: transfer <asset> <address> <amount> [from]" << std::endl;
        return false;
    }

    std::string asset = args[0];
    std::string toAddress = args[1];
    std::string amountStr = args[2];
    std::string fromAddress;

    if (args.size() >= 4)
    {
        fromAddress = args[3];
    }

    // Convert asset to script hash
    io::UInt160 assetScriptHash;

    if (asset == "neo" || asset == "NEO")
    {
        assetScriptHash = smartcontract::native::NeoToken::SCRIPT_HASH;
    }
    else if (asset == "gas" || asset == "GAS")
    {
        assetScriptHash = smartcontract::native::GasToken::SCRIPT_HASH;
    }
    else
    {
        // Try to parse as script hash
        try
        {
            assetScriptHash.FromString(asset);
        }
        catch (const std::exception&)
        {
            std::cout << "Invalid asset: " << asset << std::endl;
            return false;
        }
    }

    // Parse amount
    double amount;
    try
    {
        amount = std::stod(amountStr);
    }
    catch (const std::exception&)
    {
        std::cout << "Invalid amount: " << amountStr << std::endl;
        return false;
    }

    // Create transaction
    try
    {
        auto tx = wallet_->CreateTransferTransaction(assetScriptHash, toAddress, amount, fromAddress);

        // Sign transaction
        wallet_->SignTransaction(tx);

        // Send transaction
        auto result = node_->GetMemoryPool()->AddTransaction(tx);

        if (result)
        {
            std::cout << "Transaction sent: " << tx->GetHash().ToString() << std::endl;
            return true;
        }
        else
        {
            std::cout << "Failed to send transaction" << std::endl;
            return false;
        }
    }
    catch (const std::exception& ex)
    {
        std::cout << "Failed to create transaction: " << ex.what() << std::endl;
        return false;
    }
}

bool CommandHandler::HandleClaimGas(const std::vector<std::string>& args)
{
    if (!wallet_)
    {
        std::cout << "No wallet is open" << std::endl;
        return false;
    }

    std::string address;

    if (args.size() >= 1)
    {
        address = args[0];
    }

    try
    {
        auto tx = wallet_->CreateClaimTransaction(address);

        // Sign transaction
        wallet_->SignTransaction(tx);

        // Send transaction
        auto result = node_->GetMemoryPool()->AddTransaction(tx);

        if (result)
        {
            std::cout << "Transaction sent: " << tx->GetHash().ToString() << std::endl;
            return true;
        }
        else
        {
            std::cout << "Failed to send transaction" << std::endl;
            return false;
        }
    }
    catch (const std::exception& ex)
    {
        std::cout << "Failed to create transaction: " << ex.what() << std::endl;
        return false;
    }
}

bool CommandHandler::HandleSend(const std::vector<std::string>& args)
{
    if (args.size() < 1)
    {
        std::cout << "Usage: send <hex>" << std::endl;
        return false;
    }

    std::string hex = args[0];

    try
    {
        // Parse transaction
        auto data = io::ByteVector::FromHex(hex);

        std::istringstream stream(std::string(reinterpret_cast<const char*>(data.Data()), data.Size()));
        io::BinaryReader reader(stream);

        auto tx = std::make_shared<ledger::Transaction>();
        tx->Deserialize(reader);

        // Send transaction
        auto result = node_->GetMemoryPool()->AddTransaction(tx);

        if (result)
        {
            std::cout << "Transaction sent: " << tx->GetHash().ToString() << std::endl;
            return true;
        }
        else
        {
            std::cout << "Failed to send transaction" << std::endl;
            return false;
        }
    }
    catch (const std::exception& ex)
    {
        std::cout << "Failed to parse transaction: " << ex.what() << std::endl;
        return false;
    }
}
}  // namespace neo::cli
