#include <neo/smartcontract/native/oracle_contract.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/role_management.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/storage_item.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/transaction_attribute.h>
#include <neo/ledger/oracle_response.h>
#include <neo/cryptography/hash.h>
#include <sstream>
#include <algorithm>
#include <iostream>

namespace neo::smartcontract::native
{
    bool OracleContract::InitializeContract(ApplicationEngine& engine, uint32_t hardfork)
    {
        if (hardfork == 0)
        {
            // Initialize request ID
            auto requestIdKey = GetStorageKey(PREFIX_REQUEST_ID, io::ByteVector{});
            uint64_t id = 0;
            io::ByteVector requestIdValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(&id), sizeof(uint64_t)));
            PutStorageValue(engine.GetSnapshot(), requestIdKey, requestIdValue);

            // Initialize price (0.5 GAS)
            auto priceKey = GetStorageKey(PREFIX_PRICE, io::ByteVector{});
            int64_t price = 50000000; // 0.5 GAS
            io::ByteVector priceValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(&price), sizeof(int64_t)));
            PutStorageValue(engine.GetSnapshot(), priceKey, priceValue);
        }

        return true;
    }

    bool OracleContract::OnPersist(ApplicationEngine& engine)
    {
        // Initialize contract if needed
        auto key = GetStorageKey(PREFIX_PRICE, io::ByteVector{});
        auto value = GetStorageValue(engine.GetSnapshot(), key);
        if (value.Size() == 0)
        {
            InitializeContract(engine, 0);
        }

        return true;
    }

    bool OracleContract::PostPersist(ApplicationEngine& engine)
    {
        // Get the persisting block
        auto block = engine.GetPersistingBlock();
        if (!block)
            return false;

        // Track oracle nodes and their GAS rewards
        std::vector<std::pair<io::UInt160, int64_t>> nodeRewards;

        // Process oracle response transactions
        for (const auto& tx : block->GetTransactions())
        {
            // TODO: Check if this is an oracle response transaction
            // Skip oracle response processing for now
            continue;
        }

        // Distribute accumulated GAS rewards to oracle nodes
        if (!nodeRewards.empty())
        {
            auto gasToken = GasToken::GetInstance();
            for (const auto& reward : nodeRewards)
            {
                if (reward.second > 0)
                {
                    gasToken->Transfer(engine.GetSnapshot(), GetScriptHash(), reward.first, reward.second);
                }
            }
        }

        return true;
    }
}
