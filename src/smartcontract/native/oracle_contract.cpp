#include <neo/smartcontract/native/oracle_contract.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/role_management.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/storage_item.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/transaction_attribute.h>
#include <neo/ledger/oracle_response.h>
#include <neo/cryptography/crypto.h>
#include <sstream>
#include <algorithm>
#include <iostream>

namespace neo::smartcontract::native
{
    OracleContract::OracleContract()
        : NativeContract(NAME, ID)
    {
    }

    std::shared_ptr<OracleContract> OracleContract::GetInstance()
    {
        static std::shared_ptr<OracleContract> instance = std::make_shared<OracleContract>();
        return instance;
    }

    void OracleContract::Initialize()
    {
        RegisterMethod("getPrice", CallFlags::ReadStates, std::bind(&OracleContract::OnGetPrice, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("setPrice", CallFlags::States, std::bind(&OracleContract::OnSetPrice, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("getOracles", CallFlags::ReadStates, std::bind(&OracleContract::OnGetOracles, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("setOracles", CallFlags::States, std::bind(&OracleContract::OnSetOracles, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("request", CallFlags::States | CallFlags::AllowCall | CallFlags::AllowNotify, std::bind(&OracleContract::OnRequest, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("finish", CallFlags::States | CallFlags::AllowCall | CallFlags::AllowNotify, std::bind(&OracleContract::OnFinish, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("verify", CallFlags::ReadStates, std::bind(&OracleContract::OnVerify, this, std::placeholders::_1, std::placeholders::_2));
    }

    int64_t OracleContract::GetPrice(std::shared_ptr<persistence::StoreView> snapshot) const
    {
        auto key = GetStorageKey(PREFIX_PRICE, io::ByteVector{});
        auto value = GetStorageValue(snapshot, key);
        if (value.Size() == 0)
            return DEFAULT_PRICE;

        return *reinterpret_cast<const int64_t*>(value.Data());
    }

    std::vector<io::UInt160> OracleContract::GetOracles(std::shared_ptr<persistence::StoreView> snapshot) const
    {
        auto key = GetStorageKey(PREFIX_ORACLE, io::ByteVector{});
        auto value = GetStorageValue(snapshot, key);
        if (value.Size() == 0)
            return {};

        std::istringstream stream(std::string(reinterpret_cast<const char*>(value.Data()), value.Size()));
        io::BinaryReader reader(stream);
        uint32_t count = static_cast<uint32_t>(reader.ReadVarInt());
        std::vector<io::UInt160> oracles;
        oracles.reserve(count);
        for (uint32_t i = 0; i < count; i++)
        {
            oracles.push_back(reader.ReadSerializable<io::UInt160>());
        }
        return oracles;
    }

    void OracleContract::SetPrice(std::shared_ptr<persistence::StoreView> snapshot, int64_t price)
    {
        if (price <= 0)
            throw std::runtime_error("Price must be positive");

        auto key = GetStorageKey(PREFIX_PRICE, io::ByteVector{});
        io::ByteVector value(io::ByteSpan(reinterpret_cast<const uint8_t*>(&price), sizeof(int64_t)));
        PutStorageValue(snapshot, key, value);
    }

    void OracleContract::SetOracles(std::shared_ptr<persistence::StoreView> snapshot, const std::vector<io::UInt160>& oracles)
    {
        std::ostringstream stream;
        io::BinaryWriter writer(stream);
        writer.WriteVarInt(oracles.size());
        for (const auto& oracle : oracles)
        {
            writer.Write(oracle);
        }
        std::string data = stream.str();

        auto key = GetStorageKey(PREFIX_ORACLE, io::ByteVector{});
        io::ByteVector value(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
        PutStorageValue(snapshot, key, value);
        }

    bool OracleContract::InitializeContract(ApplicationEngine& engine, uint32_t hardfork)
    {
        if (hardfork == 0)
        {
            // Initialize price
            auto priceKey = GetStorageKey(PREFIX_PRICE, io::ByteVector{});
            int64_t price = DEFAULT_PRICE;
            io::ByteVector priceValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(&price), sizeof(int64_t)));
            PutStorageValue(engine.GetSnapshot(), priceKey, priceValue);

            // Initialize request ID
            auto idKey = GetStorageKey(PREFIX_REQUEST_ID, io::ByteVector{});
            uint64_t requestId = 0;
            io::ByteVector idValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(&requestId), sizeof(uint64_t)));
            PutStorageValue(engine.GetSnapshot(), idKey, idValue);
        }
        return true;
    }

    bool OracleContract::OnPersist(ApplicationEngine& engine)
    {
        auto block = engine.GetPersistingBlock();
        if (!block)
            return true;

        for (const auto& tx : block->GetTransactions())
        {
            // Find OracleResponse attribute
            auto attr_it = std::find_if(tx->GetAttributes().begin(), tx->GetAttributes().end(),
                [](const ledger::TransactionAttribute& a) { 
                    return a.GetUsage() == ledger::TransactionAttribute::Usage::OracleResponse; 
                });
            if (attr_it == tx->GetAttributes().end())
                continue;

            // Process oracle response
            auto responseAttr = tx->GetAttribute<ledger::OracleResponse>();
            if (responseAttr)
            {
                uint64_t id = responseAttr->GetId();
                uint8_t code = responseAttr->GetCode();
                auto result = responseAttr->GetResult();

                // Store response
                auto responseKey = GetStorageKey(PREFIX_RESPONSE, io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(&id), sizeof(uint64_t))));
                
                std::ostringstream stream;
                io::BinaryWriter writer(stream);
                writer.Write(code);
                writer.WriteVarBytes(result);
                std::string data = stream.str();
                
                io::ByteVector responseValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
                PutStorageValue(engine.GetSnapshot(), responseKey, responseValue);

                // Remove request
                auto requestKey = GetStorageKey(PREFIX_REQUEST, io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(&id), sizeof(uint64_t))));
                engine.GetSnapshot()->Delete(requestKey);

                RemoveRequestFromIdList(engine.GetSnapshot(), id);
            }
        }

        return true;
    }

    bool OracleContract::PostPersist(ApplicationEngine& engine)
    {
        return true;
    }

    OracleRequest OracleContract::GetRequest(std::shared_ptr<persistence::StoreView> snapshot, uint64_t id) const
    {
        auto key = GetStorageKey(PREFIX_REQUEST, io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(&id), sizeof(uint64_t))));
        auto value = GetStorageValue(snapshot, key);
        if (value.Size() == 0)
            throw std::runtime_error("Request not found");

        return OracleRequest::FromByteArray(value);
    }

    IdList OracleContract::GetIdList(std::shared_ptr<persistence::StoreView> snapshot, const io::ByteVector& urlHash) const
    {
        auto key = GetStorageKey(PREFIX_ID_LIST, urlHash);
        auto value = GetStorageValue(snapshot, key);
        if (value.Size() == 0)
            return IdList();

        return IdList::FromByteArray(value);
    }

    io::UInt256 OracleContract::GetUrlHash(const std::string& url)
    {
        return cryptography::Crypto::Hash256(io::ByteVector(reinterpret_cast<const uint8_t*>(url.data()), url.size()));
    }

    std::vector<std::pair<uint64_t, OracleRequest>> OracleContract::GetRequests(std::shared_ptr<persistence::StoreView> snapshot) const
    {
        std::vector<std::pair<uint64_t, OracleRequest>> requests;
        
        // Iterate through all request keys
        auto prefix = GetStorageKey(PREFIX_REQUEST, io::ByteVector{});
        snapshot->Find(prefix.AsSpan(), [&](const auto& key, const auto& value) {
            if (key.Size() >= prefix.Size() + sizeof(uint64_t))
            {
                uint64_t id = *reinterpret_cast<const uint64_t*>(key.Data() + prefix.Size());
                try {
                    auto request = OracleRequest::FromByteArray(value);
                    requests.emplace_back(id, request);
                } catch (...) {
                    // Skip invalid requests
                }
            }
            return true;
        });

        return requests;
    }

    std::vector<std::pair<uint64_t, OracleRequest>> OracleContract::GetRequestsByUrl(std::shared_ptr<persistence::StoreView> snapshot, const std::string& url) const
    {
        std::vector<std::pair<uint64_t, OracleRequest>> requests;
        auto urlHash = GetUrlHash(url);
        auto idList = GetIdList(snapshot, io::ByteVector(urlHash.AsSpan()));
        
        for (auto id : idList.GetIds())
        {
            try {
                auto request = GetRequest(snapshot, id);
                requests.emplace_back(id, request);
            } catch (...) {
                // Skip invalid requests
            }
        }

        return requests;
    }

    std::tuple<uint8_t, std::string> OracleContract::GetResponse(std::shared_ptr<persistence::StoreView> snapshot, uint64_t id) const
    {
        auto key = GetStorageKey(PREFIX_RESPONSE, io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(&id), sizeof(uint64_t))));
        auto value = GetStorageValue(snapshot, key);
        if (value.Size() == 0)
            throw std::runtime_error("Response not found");

        std::istringstream stream(std::string(reinterpret_cast<const char*>(value.Data()), value.Size()));
        io::BinaryReader reader(stream);
        uint8_t code = reader.ReadByte();
        auto result = reader.ReadVarBytes();
        
        return std::make_tuple(code, std::string(reinterpret_cast<const char*>(result.data()), result.size()));
    }

    uint64_t OracleContract::CreateRequest(std::shared_ptr<persistence::StoreView> snapshot, const std::string& url, const std::string& filter,
                                           const io::UInt160& callback, const std::string& callbackMethod, int64_t gasForResponse,
                                           const io::ByteVector& userData, const io::UInt256& originalTxid)
    {
        if (url.length() > MAX_URL_LENGTH)
            throw std::runtime_error("URL too long");
        if (filter.length() > MAX_FILTER_LENGTH)
            throw std::runtime_error("Filter too long");
        if (callbackMethod.length() > MAX_CALLBACK_LENGTH)
            throw std::runtime_error("Callback method too long");
        if (userData.Size() > MAX_USER_DATA_LENGTH)
            throw std::runtime_error("User data too long");

        uint64_t id = GetNextRequestId(snapshot);
        
        OracleRequest request;
        request.SetUrl(url);
        request.SetFilter(filter);
        request.SetCallbackContract(callback);
        request.SetCallbackMethod(callbackMethod);
        request.SetGasForResponse(gasForResponse);
        request.SetUserData(userData);
        request.SetOriginalTxid(originalTxid);

        auto requestKey = GetStorageKey(PREFIX_REQUEST, io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(&id), sizeof(uint64_t))));
        auto requestData = request.ToByteArray();
        PutStorageValue(snapshot, requestKey, requestData);

        AddRequestToIdList(snapshot, id);

        return id;
    }

    uint64_t OracleContract::GetNextRequestId(std::shared_ptr<persistence::StoreView> snapshot) const
    {
        auto key = GetStorageKey(PREFIX_REQUEST_ID, io::ByteVector{});
        auto value = GetStorageValue(snapshot, key);
        
        uint64_t currentId = 0;
        if (value.Size() >= sizeof(uint64_t))
        {
            currentId = *reinterpret_cast<const uint64_t*>(value.Data());
        }

        uint64_t nextId = currentId + 1;
        io::ByteVector nextIdValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(&nextId), sizeof(uint64_t)));
        const_cast<OracleContract*>(this)->PutStorageValue(snapshot, key, nextIdValue);

        return nextId;
    }

    void OracleContract::AddRequestToIdList(std::shared_ptr<persistence::StoreView> snapshot, uint64_t id)
    {
        auto request = GetRequest(snapshot, id);
        auto urlHash = GetUrlHash(request.GetUrl());
        auto idList = GetIdList(snapshot, io::ByteVector(urlHash.AsSpan()));
        
        idList.Add(id);
        
        auto key = GetStorageKey(PREFIX_ID_LIST, io::ByteVector(urlHash.AsSpan()));
        auto data = idList.ToByteArray();
        PutStorageValue(snapshot, key, data);
    }

    void OracleContract::RemoveRequestFromIdList(std::shared_ptr<persistence::StoreView> snapshot, uint64_t id)
    {
        try {
            auto request = GetRequest(snapshot, id);
            auto urlHash = GetUrlHash(request.GetUrl());
            auto idList = GetIdList(snapshot, io::ByteVector(urlHash.AsSpan()));
            
            idList.Remove(id);
            
            auto key = GetStorageKey(PREFIX_ID_LIST, io::ByteVector(urlHash.AsSpan()));
            if (idList.IsEmpty())
            {
                snapshot->Delete(key);
            }
            else
            {
                auto data = idList.ToByteArray();
                PutStorageValue(snapshot, key, data);
            }
        } catch (...) {
            // Request not found, ignore
        }
    }

    IdList OracleContract::GetIdList(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt256& urlHash) const
    {
        auto key = GetStorageKey(PREFIX_ID_LIST, io::ByteVector(urlHash.AsSpan()));
        auto value = GetStorageValue(snapshot, key);
        if (value.Size() == 0)
            return IdList();

        return IdList::FromByteArray(value);
    }

    bool OracleContract::CheckCommittee(ApplicationEngine& engine) const
    {
        auto neoToken = NeoToken::GetInstance();
        auto committeeAddress = neoToken->GetCommitteeAddress(engine.GetSnapshot());
        return engine.CheckWitness(committeeAddress);
    }

    bool OracleContract::CheckOracleNode(ApplicationEngine& engine) const
    {
        auto oracles = GetOracles(engine.GetSnapshot());
        auto currentScriptHash = engine.GetCurrentScriptHash();
        
        return std::find(oracles.begin(), oracles.end(), currentScriptHash) != oracles.end();
    }

    io::UInt256 OracleContract::GetOriginalTxid(ApplicationEngine& engine) const
    {
        auto tx = dynamic_cast<ledger::Transaction*>(engine.GetScriptContainer());
        if (tx)
            return tx->GetHash();
        return io::UInt256::Zero();
    }

    // Method implementations
    std::shared_ptr<vm::StackItem> OracleContract::OnGetPrice(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        auto price = GetPrice(engine.GetSnapshot());
        return vm::StackItem::Create(price);
    }

    std::shared_ptr<vm::StackItem> OracleContract::OnSetPrice(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (!CheckCommittee(engine))
            throw std::runtime_error("Only committee can set price");

        if (args.empty())
            throw std::runtime_error("Invalid arguments");

        auto price = args[0]->GetInteger();
        if (price <= 0)
            throw std::runtime_error("Price must be positive");

        SetPrice(engine.GetSnapshot(), price);
        return vm::StackItem::Create(true);
    }

    std::shared_ptr<vm::StackItem> OracleContract::OnGetOracles(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        auto oracles = GetOracles(engine.GetSnapshot());
        auto array = vm::StackItem::CreateArray();
        
        for (const auto& oracle : oracles)
        {
            array->Add(vm::StackItem::Create(io::ByteVector(oracle.AsSpan())));
        }
        
        return array;
    }

    std::shared_ptr<vm::StackItem> OracleContract::OnSetOracles(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (!CheckCommittee(engine))
            throw std::runtime_error("Only committee can set oracles");

        if (args.empty() || !args[0]->IsArray())
            throw std::runtime_error("Invalid arguments");

        auto oracleArray = args[0]->GetArray();
        std::vector<io::UInt160> oracles;
        
        for (const auto& item : oracleArray)
        {
            auto bytes = item->GetByteArray();
            if (bytes.Size() != 20)
                throw std::runtime_error("Invalid oracle address");
            
            io::UInt160 oracle;
            std::memcpy(oracle.Data(), bytes.Data(), 20);
            oracles.push_back(oracle);
        }

        SetOracles(engine.GetSnapshot(), oracles);
        return vm::StackItem::Create(true);
    }

    std::shared_ptr<vm::StackItem> OracleContract::OnRequest(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.size() < 6)
            throw std::runtime_error("Invalid arguments");

        auto url = args[0]->GetString();
        auto filter = args[1]->GetString();
        auto callbackBytes = args[2]->GetByteArray();
        auto callbackMethod = args[3]->GetString();
        auto gasForResponse = args[4]->GetInteger();
        auto userData = args[5]->GetByteArray();

        if (callbackBytes.Size() != 20)
            throw std::runtime_error("Invalid callback contract");

        io::UInt160 callback;
        std::memcpy(callback.Data(), callbackBytes.Data(), 20);

        auto price = GetPrice(engine.GetSnapshot());
        if (gasForResponse < price)
            throw std::runtime_error("Insufficient gas for response");

        auto originalTxid = GetOriginalTxid(engine);
        auto id = CreateRequest(engine.GetSnapshot(), url, filter, callback, callbackMethod, gasForResponse, userData, originalTxid);

        // Transfer gas for the request
        auto gasToken = GasToken::GetInstance();
        auto sender = engine.GetCallingScriptHash();
        if (!gasToken->Transfer(engine, sender, GetScriptHash(), gasForResponse, vm::StackItem::Null(), false))
            throw std::runtime_error("Failed to transfer gas");

        return vm::StackItem::Create(static_cast<int64_t>(id));
    }

    std::shared_ptr<vm::StackItem> OracleContract::OnFinish(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (!CheckOracleNode(engine))
            throw std::runtime_error("Only oracle nodes can finish requests");

        // This method is called by oracle nodes to process responses
        // The actual response processing is handled in OnPersist
        return vm::StackItem::Create(true);
    }

    std::shared_ptr<vm::StackItem> OracleContract::OnVerify(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        // Verify oracle transactions
        auto tx = dynamic_cast<ledger::Transaction*>(engine.GetScriptContainer());
        if (!tx)
            return vm::StackItem::Create(false);

        // Check if transaction has OracleResponse attribute
        auto attr_it = std::find_if(tx->GetAttributes().begin(), tx->GetAttributes().end(),
            [](const ledger::TransactionAttribute& a) { 
                return a.GetUsage() == ledger::TransactionAttribute::Usage::OracleResponse; 
            });
        
        if (attr_it == tx->GetAttributes().end())
            return vm::StackItem::Create(false);

        // Verify oracle node signature
        return vm::StackItem::Create(CheckOracleNode(engine));
    }
}
