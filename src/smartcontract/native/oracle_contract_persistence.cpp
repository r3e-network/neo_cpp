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
        if (value.IsEmpty())
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

        // Process oracle response transactions
        for (const auto& tx : block->GetTransactions())
        {
            // Check if this is an oracle response transaction
            auto response = tx->GetAttribute<ledger::OracleResponse>();
            if (!response)
                continue;

            // Get the request ID
            uint64_t id = response->GetId();

            try
            {
                // Get request
                auto request = GetRequest(engine.GetSnapshot(), id);

                // Remove request from storage
                auto requestKey = GetStorageKey(PREFIX_REQUEST, io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(&id), sizeof(uint64_t))));
                DeleteStorageValue(engine.GetSnapshot(), requestKey);

                // Remove request from ID list
                auto urlHash = GetUrlHash(request.GetUrl());
                auto idList = GetIdList(engine.GetSnapshot(), urlHash);
                idList.Remove(id);

                // If the list is empty, delete it
                if (idList.GetCount() == 0)
                {
                    auto idListKey = GetStorageKey(PREFIX_ID_LIST, io::ByteVector(io::ByteSpan(urlHash.Data(), urlHash.Size())));
                    DeleteStorageValue(engine.GetSnapshot(), idListKey);
                }
                else
                {
                    // Serialize the ID list
                    std::ostringstream idListStream;
                    io::BinaryWriter idListWriter(idListStream);
                    idList.Serialize(idListWriter);
                    std::string idListData = idListStream.str();

                    // Store the ID list
                    auto idListKey = GetStorageKey(PREFIX_ID_LIST, io::ByteVector(io::ByteSpan(urlHash.Data(), urlHash.Size())));
                    io::ByteVector idListValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(idListData.data()), idListData.size()));
                    PutStorageValue(engine.GetSnapshot(), idListKey, idListValue);
                }

                // Distribute GAS to oracle nodes
                auto roleManagement = RoleManagement::GetInstance();
                auto oracleNodes = roleManagement->GetDesignatedByRole(engine.GetSnapshot(), Role::Oracle, block->GetIndex());

                if (!oracleNodes.empty())
                {
                    // Calculate GAS per node
                    auto gasToken = GasToken::GetInstance();
                    int64_t gasPerNode = GetPrice(engine.GetSnapshot()) / oracleNodes.size();

                    // Distribute GAS to oracle nodes
                    for (const auto& node : oracleNodes)
                    {
                        auto account = cryptography::Hash::Hash160(node.ToArray().AsSpan());
                        gasToken->Transfer(engine.GetSnapshot(), GetScriptHash(), account, gasPerNode);
                    }
                }
            }
            catch (const std::exception& ex)
            {
                std::cerr << "Failed to process oracle response: " << ex.what() << std::endl;
                // Continue processing other transactions
            }
        }

        return true;
    }
}
