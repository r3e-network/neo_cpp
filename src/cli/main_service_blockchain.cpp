#include <neo/cli/main_service.h>
#include <neo/cli/console_helper.h>
#include <neo/io/uint256.h>
#include <neo/ledger/block.h>
#include <neo/ledger/block_header.h>
#include <neo/ledger/transaction.h>

namespace neo::cli
{
    void MainService::InitializeBlockchainCommands()
    {
        // Blockchain Commands
        RegisterCommand("showblock", [this](const std::vector<std::string>& args) {
            if (args.empty())
            {
                ConsoleHelper::Error("Missing argument: index or hash");
                return false;
            }

            OnShowBlock(args[0]);
            return true;
        }, "Blockchain");

        RegisterCommand("showheader", [this](const std::vector<std::string>& args) {
            if (args.empty())
            {
                ConsoleHelper::Error("Missing argument: index or hash");
                return false;
            }

            OnShowHeader(args[0]);
            return true;
        }, "Blockchain");

        RegisterCommand("showtx", [this](const std::vector<std::string>& args) {
            if (args.empty())
            {
                ConsoleHelper::Error("Missing argument: hash");
                return false;
            }

            try
            {
                io::UInt256 hash = io::UInt256::Parse(args[0]);
                OnShowTransaction(hash);
            }
            catch (const std::exception& ex)
            {
                ConsoleHelper::Error(ex.what());
            }

            return true;
        }, "Blockchain");
    }

    void MainService::OnShowBlock(const std::string& indexOrHash)
    {
        if (!neoSystem_)
        {
            ConsoleHelper::Error("Neo system not initialized");
            return;
        }

        try
        {
            std::shared_ptr<ledger::Block> block;
            if (indexOrHash.size() == 64)
            {
                // Hash
                io::UInt256 hash = io::UInt256::Parse(indexOrHash);
                block = neoSystem_->GetBlockchain().GetBlock(hash);
            }
            else
            {
                // Index
                uint32_t index = std::stoul(indexOrHash);
                block = neoSystem_->GetBlockchain().GetBlock(index);
            }

            if (!block)
            {
                ConsoleHelper::Error("Block not found");
                return;
            }

            ConsoleHelper::Info("Block " + std::to_string(block->GetIndex()) + ":");
            ConsoleHelper::Info("  Hash: " + block->GetHash().ToString());
            ConsoleHelper::Info("  Previous Hash: " + block->GetPrevHash().ToString());
            ConsoleHelper::Info("  Merkle Root: " + block->GetMerkleRoot().ToString());
            ConsoleHelper::Info("  Timestamp: " + std::to_string(block->GetTimestamp()));
            ConsoleHelper::Info("  Version: " + std::to_string(block->GetVersion()));
            ConsoleHelper::Info("  Next Consensus: " + block->GetNextConsensus().ToString());
            ConsoleHelper::Info("  Transactions: " + std::to_string(block->GetTransactions().size()));
        }
        catch (const std::exception& ex)
        {
            ConsoleHelper::Error(ex.what());
        }
    }

    void MainService::OnShowHeader(const std::string& indexOrHash)
    {
        if (!neoSystem_)
        {
            ConsoleHelper::Error("Neo system not initialized");
            return;
        }

        try
        {
            std::shared_ptr<ledger::BlockHeader> header;
            if (indexOrHash.size() == 64)
            {
                // Hash
                io::UInt256 hash = io::UInt256::Parse(indexOrHash);
                header = neoSystem_->GetBlockchain().GetHeader(hash);
            }
            else
            {
                // Index
                uint32_t index = std::stoul(indexOrHash);
                header = neoSystem_->GetBlockchain().GetHeader(index);
            }

            if (!header)
            {
                ConsoleHelper::Error("Header not found");
                return;
            }

            ConsoleHelper::Info("Header " + std::to_string(header->GetIndex()) + ":");
            ConsoleHelper::Info("  Hash: " + header->GetHash().ToString());
            ConsoleHelper::Info("  Previous Hash: " + header->GetPrevHash().ToString());
            ConsoleHelper::Info("  Merkle Root: " + header->GetMerkleRoot().ToString());
            ConsoleHelper::Info("  Timestamp: " + std::to_string(header->GetTimestamp()));
            ConsoleHelper::Info("  Version: " + std::to_string(header->GetVersion()));
            ConsoleHelper::Info("  Next Consensus: " + header->GetNextConsensus().ToString());
        }
        catch (const std::exception& ex)
        {
            ConsoleHelper::Error(ex.what());
        }
    }

    void MainService::OnShowTransaction(const io::UInt256& hash)
    {
        if (!neoSystem_)
        {
            ConsoleHelper::Error("Neo system not initialized");
            return;
        }

        try
        {
            auto tx = neoSystem_->GetBlockchain().GetTransaction(hash);
            if (!tx)
            {
                ConsoleHelper::Error("Transaction not found");
                return;
            }

            ConsoleHelper::Info("Transaction " + hash.ToString() + ":");
            ConsoleHelper::Info("  Version: " + std::to_string(tx->GetVersion()));
            ConsoleHelper::Info("  Nonce: " + std::to_string(tx->GetNonce()));
            ConsoleHelper::Info("  Sender: " + tx->GetSender().ToString());
            ConsoleHelper::Info("  System Fee: " + std::to_string(tx->GetSystemFee()));
            ConsoleHelper::Info("  Network Fee: " + std::to_string(tx->GetNetworkFee()));
            ConsoleHelper::Info("  Valid Until Block: " + std::to_string(tx->GetValidUntilBlock()));
            ConsoleHelper::Info("  Script: " + tx->GetScript().ToHexString());
        }
        catch (const std::exception& ex)
        {
            ConsoleHelper::Error(ex.what());
        }
    }
}
