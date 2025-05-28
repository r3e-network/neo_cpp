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
    std::shared_ptr<vm::StackItem> OracleContract::OnGetPrice(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        return vm::StackItem::Create(GetPrice(engine.GetSnapshot()));
    }

    std::shared_ptr<vm::StackItem> OracleContract::OnSetPrice(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.empty())
            throw std::runtime_error("Invalid arguments");

        // Check if caller is committee
        if (!CheckCommittee(engine))
            throw std::runtime_error("Not authorized");

        auto priceItem = args[0];
        auto price = priceItem->GetInteger();

        if (price <= 0)
            throw std::runtime_error("Invalid price");

        // Set price
        SetPrice(engine.GetSnapshot(), price);

        return vm::StackItem::Create(true);
    }

    std::shared_ptr<vm::StackItem> OracleContract::OnGetOracles(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        auto oracles = GetOracles(engine.GetSnapshot());
        std::vector<std::shared_ptr<vm::StackItem>> result;
        result.reserve(oracles.size());
        for (const auto& oracle : oracles)
        {
            result.push_back(vm::StackItem::Create(io::ByteVector(io::ByteSpan(oracle.Data(), oracle.Size()))));
        }
        return vm::StackItem::Create(result);
    }

    std::shared_ptr<vm::StackItem> OracleContract::OnSetOracles(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.empty())
            throw std::runtime_error("Invalid arguments");

        // Check if caller is committee
        if (!CheckCommittee(engine))
            throw std::runtime_error("Not authorized");

        auto oraclesItem = args[0];
        if (oraclesItem->GetType() != vm::StackItemType::Array)
            throw std::runtime_error("Invalid oracles");

        auto oraclesArray = oraclesItem->GetArray();
        std::vector<io::UInt160> oracles;
        oracles.reserve(oraclesArray.size());
        for (const auto& item : oraclesArray)
        {
            auto bytes = item->GetByteArray();
            if (bytes.Size() != 20)
                throw std::runtime_error("Invalid oracle");

            io::UInt160 oracle;
            std::memcpy(oracle.Data(), bytes.Data(), 20);
            oracles.push_back(oracle);
        }

        // Set oracles
        SetOracles(engine.GetSnapshot(), oracles);

        return vm::StackItem::Create(true);
    }

    std::shared_ptr<vm::StackItem> OracleContract::OnRequest(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.size() < 4)
            throw std::runtime_error("Invalid arguments");

        auto urlItem = args[0];
        auto filterItem = args[1];
        auto callbackItem = args[2];
        auto callbackMethodItem = args[3];
        auto gasForResponseItem = args.size() > 4 ? args[4] : vm::StackItem::Create(0);
        auto userDataItem = args.size() > 5 ? args[5] : vm::StackItem::Create(io::ByteVector{});

        auto url = urlItem->GetString();
        auto filterStr = filterItem->GetString();
        auto callback = callbackItem->GetByteArray();
        auto callbackMethod = callbackMethodItem->GetString();
        auto gasForResponse = gasForResponseItem->GetInteger();
        auto userData = userDataItem->GetByteArray();

        if (url.empty() || url.length() > MAX_URL_LENGTH)
            throw std::runtime_error("Invalid URL");

        if (filterStr.length() > MAX_FILTER_LENGTH)
            throw std::runtime_error("Filter too long");

        if (callback.Size() != 20)
            throw std::runtime_error("Invalid callback contract");

        if (callbackMethod.empty() || callbackMethod.length() > MAX_CALLBACK_LENGTH)
            throw std::runtime_error("Invalid callback method");

        if (gasForResponse < 0)
            throw std::runtime_error("Invalid gas for response");

        if (userData.Size() > MAX_USER_DATA_LENGTH)
            throw std::runtime_error("User data too large");

        // Check if the caller has enough GAS
        auto price = GetPrice(engine.GetSnapshot());
        auto gasToken = GasToken::GetInstance();
        auto caller = engine.GetCurrentScriptHash();
        auto gasBalance = gasToken->GetBalance(engine.GetSnapshot(), caller);
        if (gasBalance < price + gasForResponse)
            throw std::runtime_error("Insufficient GAS");

        // Transfer GAS to the oracle contract
        if (!gasToken->Transfer(engine.GetSnapshot(), caller, GetScriptHash(), price))
            throw std::runtime_error("Failed to transfer GAS");

        // Reserve gas for response
        if (gasForResponse > 0)
        {
            if (!gasToken->Transfer(engine.GetSnapshot(), caller, GetScriptHash(), gasForResponse))
                throw std::runtime_error("Failed to reserve gas for response");
        }

        // Create request
        io::UInt160 callbackContract;
        std::memcpy(callbackContract.Data(), callback.Data(), 20);

        // Get original transaction ID
        auto originalTxid = GetOriginalTxid(engine);

        // Create request
        uint64_t id = CreateRequest(engine.GetSnapshot(), url, filterStr, callbackContract, callbackMethod, gasForResponse, userData, originalTxid);

        // Send notification
        std::vector<std::shared_ptr<vm::StackItem>> notificationArgs;
        notificationArgs.push_back(vm::StackItem::Create(static_cast<int64_t>(id)));
        notificationArgs.push_back(vm::StackItem::Create(io::ByteVector(io::ByteSpan(callbackContract.Data(), callbackContract.Size()))));
        notificationArgs.push_back(vm::StackItem::Create(url));
        notificationArgs.push_back(vm::StackItem::Create(filterStr));
        engine.SendNotification(GetScriptHash(), "OracleRequest", vm::StackItem::Create(notificationArgs));

        return vm::StackItem::Create(id);
    }

    std::shared_ptr<vm::StackItem> OracleContract::OnFinish(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        // Check if this is a valid oracle response transaction
        auto tx = engine.GetScriptContainer<ledger::Transaction>();
        if (!tx)
            throw std::runtime_error("Not a transaction");

        auto response = tx->GetAttribute<ledger::OracleResponse>();
        if (!response)
            throw std::runtime_error("Not an oracle response");

        // Check if caller is an oracle node
        if (!CheckOracleNode(engine))
            throw std::runtime_error("Not authorized");

        // Get the response ID
        uint64_t id = response->GetId();

        // Get request
        auto request = GetRequest(engine.GetSnapshot(), id);

        // Get response code and result
        uint8_t code = response->GetCode();
        std::string result = response->GetResult();

        // Create response
        std::ostringstream stream;
        io::BinaryWriter writer(stream);
        writer.WriteByte(code);
        writer.WriteVarString(result);
        std::string data = stream.str();

        auto key = GetStorageKey(PREFIX_RESPONSE, io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(&id), sizeof(uint64_t))));
        io::ByteVector value(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
        PutStorageValue(engine.GetSnapshot(), key, value);

        // Remove request from ID list
        RemoveRequestFromIdList(engine.GetSnapshot(), id);

        // Delete request
        auto requestKey = GetStorageKey(PREFIX_REQUEST, io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(&id), sizeof(uint64_t))));
        DeleteStorageValue(engine.GetSnapshot(), requestKey);

        // Send notification
        std::vector<std::shared_ptr<vm::StackItem>> notificationArgs;
        notificationArgs.push_back(vm::StackItem::Create(static_cast<int64_t>(id)));
        notificationArgs.push_back(vm::StackItem::Create(io::ByteVector(io::ByteSpan(request.GetOriginalTxid().Data(), request.GetOriginalTxid().Size()))));
        engine.SendNotification(GetScriptHash(), "OracleResponse", vm::StackItem::Create(notificationArgs));

        // Call callback
        std::vector<std::shared_ptr<vm::StackItem>> callbackArgs;
        callbackArgs.push_back(vm::StackItem::Create(request.GetUrl()));
        callbackArgs.push_back(vm::StackItem::Create(request.GetUserData()));
        callbackArgs.push_back(vm::StackItem::Create(static_cast<int64_t>(code)));
        callbackArgs.push_back(vm::StackItem::Create(result));

        // Set gas limit for callback
        engine.AddGas(request.GetGasForResponse());

        // Call callback contract
        try
        {
            engine.CallContract(request.GetCallbackContract(), request.GetCallbackMethod(), callbackArgs, CallFlags::All);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Callback failed: " << ex.what() << std::endl;
            // Continue execution even if callback fails
        }

        return vm::StackItem::Create(true);
    }

    std::shared_ptr<vm::StackItem> OracleContract::OnVerify(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        // Check if this is a valid oracle response transaction
        auto tx = engine.GetScriptContainer<ledger::Transaction>();
        if (!tx)
            return vm::StackItem::Create(false);

        auto response = tx->GetAttribute<ledger::OracleResponse>();
        return vm::StackItem::Create(response != nullptr);
    }

    bool OracleContract::CheckCommittee(ApplicationEngine& engine) const
    {
        // Get the current script hash
        auto currentScriptHash = engine.GetCurrentScriptHash();

        // Get the committee members
        auto roleManagement = RoleManagement::GetInstance();
        auto committee = roleManagement->GetDesignatedByRole(engine.GetSnapshot(), Role::Committee, engine.GetPersistingBlock()->GetIndex());

        // Check if the current script hash is a committee member
        for (const auto& member : committee)
        {
            auto scriptHash = cryptography::Hash::Hash160(member.ToArray().AsSpan());
            if (scriptHash == currentScriptHash)
                return true;
        }

        return false;
    }

    bool OracleContract::CheckOracleNode(ApplicationEngine& engine) const
    {
        // Get the current script hash
        auto currentScriptHash = engine.GetCurrentScriptHash();

        // Get the oracle nodes
        auto roleManagement = RoleManagement::GetInstance();
        auto oracleNodes = roleManagement->GetDesignatedByRole(engine.GetSnapshot(), Role::Oracle, engine.GetPersistingBlock()->GetIndex());

        // Check if the current script hash is an oracle node
        for (const auto& node : oracleNodes)
        {
            auto scriptHash = cryptography::Hash::Hash160(node.ToArray().AsSpan());
            if (scriptHash == currentScriptHash)
                return true;
        }

        return false;
    }

    io::UInt256 OracleContract::GetOriginalTxid(ApplicationEngine& engine) const
    {
        // Get the transaction
        auto tx = engine.GetScriptContainer<ledger::Transaction>();
        if (!tx)
            return io::UInt256();

        // Get the oracle response attribute
        auto response = tx->GetAttribute<ledger::OracleResponse>();
        if (!response)
            return tx->GetHash();

        // Get the request
        try
        {
            auto request = GetRequest(engine.GetSnapshot(), response->GetId());
            return request.GetOriginalTxid();
        }
        catch (const std::exception&)
        {
            return tx->GetHash();
        }
    }
}
