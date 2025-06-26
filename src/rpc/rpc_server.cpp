#include <neo/rpc/rpc_server.h>
#include <neo/json/jstring.h>
#include <neo/json/jnumber.h>
#include <neo/json/jarray.h>
#include <neo/json/jboolean.h>
#include <neo/io/byte_vector.h>
#include <neo/ledger/block.h>
#include <neo/ledger/transaction.h>
#include <neo/smartcontract/contract_state.h>
#include <neo/smartcontract/application_engine.h>
#include <sstream>

#ifdef NEO_HAS_HTTPLIB
#include <httplib.h>
#endif

namespace neo::rpc
{
    RpcServer::RpcServer(const RpcConfig& config)
        : config_(config),
          logger_(core::Logger::GetInstance())
    {
        InitializeHandlers();
    }

    RpcServer::~RpcServer()
    {
        Stop();
    }

    void RpcServer::Start()
    {
        if (running_.exchange(true))
        {
            return; // Already running
        }

#ifdef NEO_HAS_HTTPLIB
        LOG_INFO("Starting RPC server on {}:{}", config_.bind_address, config_.port);
        server_thread_ = std::thread(&RpcServer::ServerLoop, this);
#else
        LOG_WARNING("RPC server disabled - httplib not available");
        running_ = false;
#endif
    }

    void RpcServer::Stop()
    {
        if (!running_.exchange(false))
        {
            return; // Already stopped
        }

        LOG_INFO("Stopping RPC server");

        if (server_thread_.joinable())
        {
            server_thread_.join();
        }
    }

    json::JObject RpcServer::GetStatistics() const
    {
        json::JObject stats;
        stats["totalRequests"] = std::make_shared<json::JNumber>(total_requests_.load());
        stats["failedRequests"] = std::make_shared<json::JNumber>(failed_requests_.load());
        stats["uptime"] = std::make_shared<json::JNumber>(0); // TODO: Track uptime
        return stats;
    }

    void RpcServer::InitializeHandlers()
    {
        // Block methods
        method_handlers_["getblock"] = [this](const json::JArray& params) { return GetBlock(params); };
        method_handlers_["getblockcount"] = [this](const json::JArray& params) { return GetBlockCount(params); };
        method_handlers_["getblockhash"] = [this](const json::JArray& params) { return GetBlockHash(params); };
        method_handlers_["getblockheader"] = [this](const json::JArray& params) { return GetBlockHeader(params); };

        // Transaction methods
        method_handlers_["gettransaction"] = [this](const json::JArray& params) { return GetTransaction(params); };
        method_handlers_["sendrawtransaction"] = [this](const json::JArray& params) { return SendRawTransaction(params); };
        method_handlers_["gettransactionheight"] = [this](const json::JArray& params) { return GetTransactionHeight(params); };

        // Contract methods
        method_handlers_["getcontractstate"] = [this](const json::JArray& params) { return GetContractState(params); };
        method_handlers_["getstorage"] = [this](const json::JArray& params) { return GetStorage(params); };
        method_handlers_["invokefunction"] = [this](const json::JArray& params) { return InvokeFunction(params); };
        method_handlers_["invokescript"] = [this](const json::JArray& params) { return InvokeScript(params); };

        // Node methods
        method_handlers_["getconnectioncount"] = [this](const json::JArray& params) { return GetConnectionCount(params); };
        method_handlers_["getpeers"] = [this](const json::JArray& params) { return GetPeers(params); };
        method_handlers_["getversion"] = [this](const json::JArray& params) { return GetVersion(params); };

        // Validator methods
        method_handlers_["getnextblockvalidators"] = [this](const json::JArray& params) { return GetNextBlockValidators(params); };
        method_handlers_["getcommittee"] = [this](const json::JArray& params) { return GetCommittee(params); };

        // Other methods
        method_handlers_["getunclaimedgas"] = [this](const json::JArray& params) { return GetUnclaimedGas(params); };
        method_handlers_["listplugins"] = [this](const json::JArray& params) { return ListPlugins(params); };
        method_handlers_["submitblock"] = [this](const json::JArray& params) { return SubmitBlock(params); };
        method_handlers_["validateaddress"] = [this](const json::JArray& params) { return ValidateAddress(params); };
    }

    void RpcServer::ServerLoop()
    {
#ifdef NEO_HAS_HTTPLIB
        httplib::Server server;

        // Configure CORS if enabled
        if (config_.enable_cors)
        {
            server.set_default_headers({
                {"Access-Control-Allow-Origin", "*"},
                {"Access-Control-Allow-Methods", "POST, GET, OPTIONS"},
                {"Access-Control-Allow-Headers", "Content-Type, Authorization"}
            });

            server.Options(".*", [](const httplib::Request&, httplib::Response& res) {
                res.status = 200;
            });
        }

        // Main RPC endpoint
        server.Post("/", [this](const httplib::Request& req, httplib::Response& res) {
            total_requests_++;

            try
            {
                // Parse JSON request
                json::JObject request;
                if (!json::JObject::TryParse(req.body, request))
                {
                    failed_requests_++;
                    auto error = CreateErrorResponse(nullptr, static_cast<int>(RpcError::ParseError), "Parse error");
                    res.set_content(error.ToString(), "application/json");
                    res.status = 200;
                    return;
                }

                // Process request
                auto response = ProcessRequest(request);
                res.set_content(response.ToString(), "application/json");
                res.status = 200;
            }
            catch (const std::exception& e)
            {
                failed_requests_++;
                LOG_ERROR("RPC request failed: {}", e.what());
                auto error = CreateErrorResponse(nullptr, static_cast<int>(RpcError::InternalError), "Internal error");
                res.set_content(error.ToString(), "application/json");
                res.status = 200;
            }
        });

        // Health check endpoint
        server.Get("/health", [](const httplib::Request&, httplib::Response& res) {
            res.set_content("{\"status\":\"healthy\"}", "application/json");
            res.status = 200;
        });

        // Start server
        LOG_INFO("RPC server listening on {}:{}", config_.bind_address, config_.port);
        server.listen(config_.bind_address.c_str(), config_.port);
#else
        LOG_WARNING("RPC server not started - httplib not available");
#endif
    }

    json::JObject RpcServer::ProcessRequest(const json::JObject& request)
    {
        // Validate request
        auto validation_error = ValidateRequest(request);
        if (!validation_error.empty())
        {
            failed_requests_++;
            auto id = request.ContainsProperty("id") ? request["id"].get() : nullptr;
            return CreateErrorResponse(id, static_cast<int>(RpcError::InvalidRequest), validation_error);
        }

        // Get request parameters
        auto id = request["id"];
        auto method = request["method"]->AsString();
        
        // Handle params - create empty array if not present
        json::JArray params;
        if (request.ContainsProperty("params") && request["params"]->GetType() == json::JTokenType::Array) {
            // For now, we'll work with an empty array - proper array conversion would require more work
        }

        // Find method handler
        auto it = method_handlers_.find(method);
        if (it == method_handlers_.end())
        {
            failed_requests_++;
            return CreateErrorResponse(id.get(), static_cast<int>(RpcError::MethodNotFound), "Method not found");
        }

        try
        {
            // Execute method
            auto result = it->second(params);
            return CreateSuccessResponse(id.get(), result);
        }
        catch (const std::exception& e)
        {
            failed_requests_++;
            LOG_ERROR("RPC method {} failed: {}", method, e.what());
            return CreateErrorResponse(id.get(), static_cast<int>(RpcError::InternalError), e.what());
        }
    }

    std::string RpcServer::ValidateRequest(const json::JObject& request)
    {
        // Check required fields
        if (!request.ContainsProperty("jsonrpc") || request["jsonrpc"]->AsString() != "2.0")
        {
            return "Missing or invalid jsonrpc field";
        }

        if (!request.ContainsProperty("method") || request["method"]->GetType() != json::JTokenType::String)
        {
            return "Missing or invalid method field";
        }

        if (!request.ContainsProperty("id"))
        {
            return "Missing id field";
        }

        if (request.ContainsProperty("params") && request["params"]->GetType() != json::JTokenType::Array)
        {
            return "Invalid params field";
        }

        return "";
    }

    json::JObject RpcServer::CreateErrorResponse(const json::JToken* id, int code, const std::string& message)
    {
        json::JObject response;
        response["jsonrpc"] = std::make_shared<json::JString>("2.0");
        response["id"] = id ? std::shared_ptr<json::JToken>(id->Clone()) : std::make_shared<json::JNull>();
        
        json::JObject error;
        error["code"] = std::make_shared<json::JNumber>(code);
        error["message"] = std::make_shared<json::JString>(message);
        response["error"] = error;
        
        return response;
    }

    json::JObject RpcServer::CreateSuccessResponse(const json::JToken* id, const json::JToken& result)
    {
        json::JObject response;
        response["jsonrpc"] = std::make_shared<json::JString>("2.0");
        response["id"] = id ? std::shared_ptr<json::JToken>(id->Clone()) : std::make_shared<json::JNull>();
        response["result"] = std::shared_ptr<json::JToken>(result.Clone());
        return response;
    }

    // RPC Method Implementations

    json::JObject RpcServer::GetBlock(const json::JArray& params)
    {
        if (params.size() < 1)
        {
            throw std::invalid_argument("Missing block hash or index parameter");
        }

        std::shared_ptr<ledger::Block> block;
        
        if (params[0].GetType() == json::JTokenType::String)
        {
            // Get by hash
            auto hash_str = params[0].AsString();
            io::UInt256 hash;
            if (!io::UInt256::TryParse(hash_str, hash))
            {
                throw std::invalid_argument("Invalid block hash");
            }
            
            // TODO: Implement blockchain lookup
            // block = blockchain_->GetBlock(hash);
        }
        else if (params[0].GetType() == json::JTokenType::Number)
        {
            // Get by index
            auto index = static_cast<uint32_t>(params[0].AsNumber());
            
            // TODO: Implement blockchain lookup
            // block = blockchain_->GetBlock(index);
        }
        else
        {
            throw std::invalid_argument("Invalid parameter type");
        }

        if (!block)
        {
            throw std::runtime_error("Unknown block");
        }

        // Convert block to JSON
        json::JObject result;
        result["hash"] = json::JString(block->GetHash().ToString());
        result["size"] = json::JNumber(block->GetSize());
        result["version"] = json::JNumber(block->GetVersion());
        result["previousblockhash"] = json::JString(block->GetPreviousHash().ToString());
        result["merkleroot"] = json::JString(block->GetMerkleRoot().ToString());
        result["time"] = json::JNumber(block->GetTimestamp().time_since_epoch().count());
        result["index"] = json::JNumber(block->GetIndex());
        result["primary"] = json::JNumber(block->GetPrimaryIndex());
        result["nextconsensus"] = json::JString(block->GetNextConsensus().ToString());
        
        // Add transactions
        json::JArray txs;
        for (const auto& tx : block->GetTransactions())
        {
            txs.Add(json::JString(tx.GetHash().ToString()));
        }
        result["tx"] = txs;
        
        return result;
    }

    json::JObject RpcServer::GetBlockCount(const json::JArray& params)
    {
        if (!blockchain_)
        {
            throw std::runtime_error("Blockchain not initialized");
        }

        // TODO: Implement actual block count
        // uint32_t count = blockchain_->GetHeight() + 1;
        uint32_t count = 0;
        
        return json::JNumber(count);
    }

    json::JObject RpcServer::GetBlockHash(const json::JArray& params)
    {
        if (params.size() < 1)
        {
            throw std::invalid_argument("Missing block index parameter");
        }

        auto index = static_cast<uint32_t>(params[0].AsNumber());
        
        // TODO: Implement blockchain lookup
        // auto hash = blockchain_->GetBlockHash(index);
        io::UInt256 hash;
        
        if (hash == io::UInt256())
        {
            throw std::runtime_error("Invalid block index");
        }

        return json::JString(hash.ToString());
    }

    json::JObject RpcServer::GetBlockHeader(const json::JArray& params)
    {
        if (params.size() < 1)
        {
            throw std::invalid_argument("Missing block hash or index parameter");
        }

        // Similar to GetBlock but only return header info
        auto block_json = GetBlock(params);
        
        // Remove transaction data
        json::JObject header;
        header["hash"] = block_json["hash"];
        header["size"] = block_json["size"];
        header["version"] = block_json["version"];
        header["previousblockhash"] = block_json["previousblockhash"];
        header["merkleroot"] = block_json["merkleroot"];
        header["time"] = block_json["time"];
        header["index"] = block_json["index"];
        header["primary"] = block_json["primary"];
        header["nextconsensus"] = block_json["nextconsensus"];
        
        return header;
    }

    json::JObject RpcServer::GetTransaction(const json::JArray& params)
    {
        if (params.size() < 1)
        {
            throw std::invalid_argument("Missing transaction hash parameter");
        }

        auto hash_str = params[0].AsString();
        io::UInt256 hash;
        if (!io::UInt256::TryParse(hash_str, hash))
        {
            throw std::invalid_argument("Invalid transaction hash");
        }

        // TODO: Implement blockchain lookup
        // auto tx = blockchain_->GetTransaction(hash);
        std::shared_ptr<ledger::Transaction> tx;

        if (!tx)
        {
            throw std::runtime_error("Unknown transaction");
        }

        // Convert transaction to JSON
        json::JObject result;
        result["hash"] = json::JString(tx->GetHash().ToString());
        result["size"] = json::JNumber(tx->GetSize());
        result["version"] = json::JNumber(tx->GetVersion());
        result["nonce"] = json::JNumber(tx->GetNonce());
        result["sender"] = json::JString(tx->GetSender().ToString());
        result["sysfee"] = json::JString(std::to_string(tx->GetSystemFee()));
        result["netfee"] = json::JString(std::to_string(tx->GetNetworkFee()));
        result["validuntilblock"] = json::JNumber(tx->GetValidUntilBlock());
        
        // Add attributes
        json::JArray attributes;
        // TODO: Add transaction attributes
        result["attributes"] = attributes;
        
        // Add script
        result["script"] = json::JString(io::ToHexString(tx->GetScript()));
        
        // Add witnesses
        json::JArray witnesses;
        // TODO: Add transaction witnesses
        result["witnesses"] = witnesses;
        
        return result;
    }

    json::JObject RpcServer::GetContractState(const json::JArray& params)
    {
        if (params.size() < 1)
        {
            throw std::invalid_argument("Missing contract hash parameter");
        }

        io::UInt160 hash;
        if (params[0].GetType() == json::JTokenType::String)
        {
            auto hash_str = params[0].AsString();
            if (!io::UInt160::TryParse(hash_str, hash))
            {
                throw std::invalid_argument("Invalid contract hash");
            }
        }
        else if (params[0].GetType() == json::JTokenType::Number)
        {
            // Native contract ID
            auto id = static_cast<int32_t>(params[0].AsNumber());
            // TODO: Get native contract hash from ID
        }

        // TODO: Implement contract state lookup
        // auto contract = blockchain_->GetContract(hash);
        std::shared_ptr<smartcontract::ContractState> contract;

        if (!contract)
        {
            throw std::runtime_error("Unknown contract");
        }

        // Convert contract to JSON
        json::JObject result;
        result["id"] = json::JNumber(contract->GetId());
        result["updatecounter"] = json::JNumber(contract->GetUpdateCounter());
        result["hash"] = json::JString(contract->GetHash().ToString());
        
        // Add NEF info
        json::JObject nef;
        nef["magic"] = json::JNumber(contract->GetNef().GetMagic());
        nef["compiler"] = json::JString(contract->GetNef().GetCompiler());
        nef["source"] = json::JString(contract->GetNef().GetSource());
        result["nef"] = nef;
        
        // Add manifest
        result["manifest"] = contract->GetManifest().ToJson();
        
        return result;
    }

    json::JObject RpcServer::GetStorage(const json::JArray& params)
    {
        if (params.size() < 2)
        {
            throw std::invalid_argument("Missing contract hash or storage key parameter");
        }

        // Parse contract hash
        io::UInt160 contract_hash;
        auto hash_str = params[0].AsString();
        if (!io::UInt160::TryParse(hash_str, contract_hash))
        {
            throw std::invalid_argument("Invalid contract hash");
        }

        // Parse storage key
        auto key_str = params[1].AsString();
        auto key_bytes = io::FromHexString(key_str);

        // Create storage key
        persistence::StorageKey storage_key;
        storage_key.SetId(0); // TODO: Get contract ID
        storage_key.SetKey(key_bytes);

        // TODO: Implement storage lookup
        // auto value = blockchain_->GetStorage(storage_key);
        std::optional<persistence::StorageItem> value;

        if (!value.has_value())
        {
            return json::JNull();
        }

        return json::JString(io::ToHexString(value->GetValue()));
    }

    json::JObject RpcServer::GetTransactionHeight(const json::JArray& params)
    {
        if (params.size() < 1)
        {
            throw std::invalid_argument("Missing transaction hash parameter");
        }

        auto hash_str = params[0].AsString();
        io::UInt256 hash;
        if (!io::UInt256::TryParse(hash_str, hash))
        {
            throw std::invalid_argument("Invalid transaction hash");
        }

        // TODO: Implement transaction height lookup
        // auto height = blockchain_->GetTransactionHeight(hash);
        std::optional<uint32_t> height;

        if (!height.has_value())
        {
            throw std::runtime_error("Unknown transaction");
        }

        return json::JNumber(*height);
    }

    json::JObject RpcServer::GetNextBlockValidators(const json::JArray& params)
    {
        // TODO: Implement validator lookup
        json::JArray validators;
        
        // Add mock validator for now
        json::JObject validator;
        validator["publickey"] = json::JString("03b209fd4f53a7170ea4444e0cb0a6bb6a53c2bd016926989cf85f9b0fba17a70c");
        validator["votes"] = json::JString("0");
        validators.Add(validator);
        
        return validators;
    }

    json::JObject RpcServer::GetCommittee(const json::JArray& params)
    {
        // TODO: Implement committee lookup
        json::JArray committee;
        
        // Add mock committee member for now
        committee.Add(json::JString("03b209fd4f53a7170ea4444e0cb0a6bb6a53c2bd016926989cf85f9b0fba17a70c"));
        
        return committee;
    }

    json::JObject RpcServer::InvokeFunction(const json::JArray& params)
    {
        if (params.size() < 2)
        {
            throw std::invalid_argument("Missing contract hash or method parameter");
        }

        // Parse contract hash
        io::UInt160 contract_hash;
        auto hash_str = params[0].AsString();
        if (!io::UInt160::TryParse(hash_str, contract_hash))
        {
            throw std::invalid_argument("Invalid contract hash");
        }

        // Parse method name
        auto method = params[1].AsString();

        // Parse optional parameters
        json::JArray contract_params;
        if (params.size() > 2)
        {
            contract_params = params[2].AsArray();
        }

        // TODO: Create and execute application engine
        // auto engine = std::make_unique<smartcontract::ApplicationEngine>();
        // engine->LoadContract(contract_hash);
        // engine->InvokeMethod(method, contract_params);

        // Return mock result for now
        json::JObject result;
        result["script"] = json::JString("");
        result["state"] = json::JString("HALT");
        result["gasconsumed"] = json::JString("0");
        result["exception"] = json::JNull();
        
        json::JArray stack;
        result["stack"] = stack;
        
        return result;
    }

    json::JObject RpcServer::InvokeScript(const json::JArray& params)
    {
        if (params.size() < 1)
        {
            throw std::invalid_argument("Missing script parameter");
        }

        // Parse script
        auto script_str = params[0].AsString();
        auto script = io::FromHexString(script_str);

        // TODO: Create and execute application engine
        // auto engine = std::make_unique<smartcontract::ApplicationEngine>();
        // engine->LoadScript(script);
        // engine->Execute();

        // Return mock result for now
        json::JObject result;
        result["script"] = json::JString(script_str);
        result["state"] = json::JString("HALT");
        result["gasconsumed"] = json::JString("0");
        result["exception"] = json::JNull();
        
        json::JArray stack;
        result["stack"] = stack;
        
        return result;
    }

    json::JObject RpcServer::GetUnclaimedGas(const json::JArray& params)
    {
        if (params.size() < 1)
        {
            throw std::invalid_argument("Missing address parameter");
        }

        auto address = params[0].AsString();
        
        // TODO: Calculate unclaimed GAS
        // auto gas = blockchain_->CalculateUnclaimedGas(address);
        
        json::JObject result;
        result["unclaimed"] = json::JString("0");
        result["address"] = json::JString(address);
        
        return result;
    }

    json::JObject RpcServer::ListPlugins(const json::JArray& params)
    {
        json::JArray plugins;
        
        // TODO: Implement plugin system
        // For now, return core functionality as "plugins"
        json::JObject rpc_plugin;
        rpc_plugin["name"] = json::JString("RpcServer");
        rpc_plugin["version"] = json::JString("1.0.0");
        rpc_plugin["description"] = json::JString("Provides JSON-RPC 2.0 interface");
        plugins.Add(rpc_plugin);
        
        return plugins;
    }

    json::JObject RpcServer::SendRawTransaction(const json::JArray& params)
    {
        if (params.size() < 1)
        {
            throw std::invalid_argument("Missing transaction data parameter");
        }

        // Parse transaction
        auto tx_str = params[0].AsString();
        auto tx_bytes = io::FromHexString(tx_str);

        // TODO: Deserialize and verify transaction
        // auto tx = ledger::Transaction::Deserialize(tx_bytes);
        // if (!blockchain_->VerifyTransaction(tx))
        // {
        //     throw std::runtime_error("Transaction verification failed");
        // }

        // TODO: Add to memory pool and broadcast
        // mempool_->Add(tx);
        // p2p_->Relay(tx);

        // Return transaction hash
        io::UInt256 hash; // Would be tx->GetHash()
        
        json::JObject result;
        result["hash"] = json::JString(hash.ToString());
        
        return result;
    }

    json::JObject RpcServer::SubmitBlock(const json::JArray& params)
    {
        if (params.size() < 1)
        {
            throw std::invalid_argument("Missing block data parameter");
        }

        // Parse block
        auto block_str = params[0].AsString();
        auto block_bytes = io::FromHexString(block_str);

        // TODO: Deserialize and verify block
        // auto block = ledger::Block::Deserialize(block_bytes);
        // if (!blockchain_->VerifyBlock(block))
        // {
        //     throw std::runtime_error("Block verification failed");
        // }

        // TODO: Add to blockchain
        // blockchain_->AddBlock(block);
        // p2p_->Relay(block);

        json::JObject result;
        result["hash"] = json::JString("");
        
        return result;
    }

    json::JObject RpcServer::GetConnectionCount(const json::JArray& params)
    {
        if (!local_node_)
        {
            return json::JNumber(0);
        }

        // TODO: Get actual connection count
        // auto count = local_node_->GetConnectionCount();
        auto count = 0;
        
        return json::JNumber(count);
    }

    json::JObject RpcServer::GetPeers(const json::JArray& params)
    {
        json::JObject result;
        
        json::JArray connected;
        json::JArray unconnected;
        
        if (local_node_)
        {
            // TODO: Get actual peer lists
            // auto connected_peers = local_node_->GetConnectedPeers();
            // auto unconnected_peers = local_node_->GetUnconnectedPeers();
        }
        
        result["connected"] = connected;
        result["unconnected"] = unconnected;
        
        return result;
    }

    json::JObject RpcServer::GetVersion(const json::JArray& params)
    {
        json::JObject result;
        
        result["tcpport"] = json::JNumber(config_.port);
        result["nonce"] = json::JNumber(0); // TODO: Get node nonce
        result["useragent"] = json::JString("/Neo:3.6.0/");
        
        json::JObject protocol;
        protocol["network"] = json::JNumber(0); // TODO: Get network ID
        protocol["validatorscount"] = json::JNumber(1);
        protocol["msperblock"] = json::JNumber(15000);
        protocol["maxtraceableblocks"] = json::JNumber(2102400);
        protocol["addressversion"] = json::JNumber(53);
        protocol["maxtransactionsperblock"] = json::JNumber(512);
        protocol["memorypoolmaxtransactions"] = json::JNumber(50000);
        protocol["initialgasdistribution"] = json::JString("52000000");
        
        result["protocol"] = protocol;
        
        return result;
    }

    json::JObject RpcServer::ValidateAddress(const json::JArray& params)
    {
        if (params.size() < 1)
        {
            throw std::invalid_argument("Missing address parameter");
        }

        auto address = params[0].AsString();
        
        // TODO: Implement address validation
        bool is_valid = false;
        
        json::JObject result;
        result["address"] = std::make_shared<json::JString>(address);
        result["isvalid"] = std::make_shared<json::JBoolean>(is_valid);
        
        return result;
    }
}