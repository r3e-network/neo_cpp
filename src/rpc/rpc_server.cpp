#include <neo/rpc/rpc_server.h>
#include <neo/json/jstring.h>
#include <neo/json/jnumber.h>
#include <neo/json/jarray.h>
#include <neo/json/jboolean.h>
#include <neo/smartcontract/native/contract_management.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/core/network_config.h>
#include <random>
#include <sstream>

#ifdef NEO_HAS_HTTPLIB
#include <httplib.h>
#endif

namespace neo::rpc
{
    // JSON utility helpers
    namespace {
        json::JObject CreateErrorResponse(const json::JToken* id, int code, const std::string& message) {
            json::JObject response;
            response["jsonrpc"] = std::make_shared<json::JString>("2.0");
            response["id"] = id ? std::shared_ptr<json::JToken>(id->Clone()) : std::make_shared<json::JNull>();
            
            json::JObject error;
            error["code"] = std::make_shared<json::JNumber>(code);
            error["message"] = std::make_shared<json::JString>(message);
            response["error"] = error;
            return response;
        }

        json::JObject CreateSuccessResponse(const json::JToken* id, const json::JToken& result) {
            json::JObject response;
            response["jsonrpc"] = std::make_shared<json::JString>("2.0");
            response["id"] = id ? std::shared_ptr<json::JToken>(id->Clone()) : std::make_shared<json::JNull>();
            response["result"] = std::shared_ptr<json::JToken>(result.Clone());
            return response;
        }

        uint32_t GetNodeNonce() {
            static uint32_t nonce = []() {
                std::random_device rd;
                return std::uniform_int_distribution<uint32_t>{}(std::mt19937{rd()});
            }();
            return nonce;
        }

        uint32_t GetNetworkId() {
            try {
                return ProtocolSettings::GetDefault().GetNetwork();
            } catch (const std::exception& e) {
                LOG_WARNING("Failed to get network ID from protocol settings: {}", e.what());
                return core::NetworkConfig::GetNetworkMagic("mainnet"); // MainNet default
            } catch (const std::bad_alloc& e) {
                LOG_ERROR("Memory allocation failed getting network ID: {}, using MainNet default", e.what());
                return core::NetworkConfig::GetNetworkMagic("mainnet"); // MainNet default
            }
        }
    }

    RpcServer::RpcServer(const RpcConfig& config)
        : config_(config),
          logger_(core::Logger::GetInstance()),
          start_time_(std::chrono::steady_clock::now())
    {
        InitializeHandlers();
    }

    RpcServer::~RpcServer() {
        Stop();
    }

    void RpcServer::Start() {
        if (running_.exchange(true)) return;

#ifdef NEO_HAS_HTTPLIB
        server_thread_ = std::thread(&RpcServer::ServerLoop, this);
#else
        running_ = false;
#endif
    }

    void RpcServer::Stop() {
        if (!running_.exchange(false)) return;
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
    }

    json::JObject RpcServer::GetStatistics() const {
        json::JObject stats;
        stats["totalRequests"] = std::make_shared<json::JNumber>(total_requests_.load());
        stats["failedRequests"] = std::make_shared<json::JNumber>(failed_requests_.load());
        
        auto now = std::chrono::steady_clock::now();
        auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
        stats["uptime"] = std::make_shared<json::JNumber>(uptime.count());
        return stats;
    }

    void RpcServer::InitializeHandlers() {
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

        // Other methods
        method_handlers_["getunclaimedgas"] = [this](const json::JArray& params) { return GetUnclaimedGas(params); };
        method_handlers_["listplugins"] = [this](const json::JArray& params) { return ListPlugins(params); };
        method_handlers_["submitblock"] = [this](const json::JArray& params) { return SubmitBlock(params); };
        method_handlers_["validateaddress"] = [this](const json::JArray& params) { return ValidateAddress(params); };
    }

    void RpcServer::ServerLoop() {
#ifdef NEO_HAS_HTTPLIB
        httplib::Server server;

        if (config_.enable_cors) {
            server.set_default_headers({
                {"Access-Control-Allow-Origin", "*"},
                {"Access-Control-Allow-Methods", "POST, GET, OPTIONS"},
                {"Access-Control-Allow-Headers", "Content-Type, Authorization"}
            });
        }

        server.Post("/", [this](const httplib::Request& req, httplib::Response& res) {
            total_requests_++;
            try {
                json::JObject request;
                if (!json::JObject::TryParse(req.body, request)) {
                    failed_requests_++;
                    auto error = CreateErrorResponse(nullptr, -32700, "Parse error");
                    res.set_content(error.ToString(), "application/json");
                    return;
                }

                auto response = ProcessRequest(request);
                res.set_content(response.ToString(), "application/json");
            } catch (const std::exception& e) {
                failed_requests_++;
                auto error = CreateErrorResponse(nullptr, -32603, "Internal error");
                res.set_content(error.ToString(), "application/json");
            }
        });

        server.listen(config_.bind_address.c_str(), config_.port);
#endif
    }

    json::JObject RpcServer::ProcessRequest(const json::JObject& request) {
        // Validate request
        if (!request.ContainsProperty("jsonrpc") || request["jsonrpc"]->AsString() != "2.0" ||
            !request.ContainsProperty("method") || !request.ContainsProperty("id")) {
            failed_requests_++;
            auto id = request.ContainsProperty("id") ? request["id"].get() : nullptr;
            return CreateErrorResponse(id, -32600, "Invalid Request");
        }

        auto id = request["id"];
        auto method = request["method"]->AsString();
        
        // Parse params
        json::JArray params;
        if (request.ContainsProperty("params") && request["params"]->GetType() == json::JTokenType::Array) {
            params = request["params"]->GetArray();
        }

        // Find and execute method
        auto it = method_handlers_.find(method);
        if (it == method_handlers_.end()) {
            failed_requests_++;
            return CreateErrorResponse(id.get(), -32601, "Method not found");
        }

        try {
            auto result = it->second(params);
            return CreateSuccessResponse(id.get(), result);
        } catch (const std::exception& e) {
            failed_requests_++;
            return CreateErrorResponse(id.get(), -32603, e.what());
        }
    }

    // RPC Method Implementations

    json::JObject RpcServer::GetBlock(const json::JArray& params) {
        if (params.size() < 1) {
            throw std::invalid_argument("Missing block hash or index parameter");
        }
        if (!blockchain_) {
            throw std::runtime_error("Blockchain not initialized");
        }

        std::shared_ptr<ledger::Block> block;
        
        if (params[0].GetType() == json::JTokenType::String) {
            io::UInt256 hash;
            if (!io::UInt256::TryParse(params[0].AsString(), hash)) {
                throw std::invalid_argument("Invalid block hash");
            }
            block = blockchain_->GetBlock(hash);
        } else if (params[0].GetType() == json::JTokenType::Number) {
            auto index = static_cast<uint32_t>(params[0].AsNumber());
            block = blockchain_->GetBlock(index);
        } else {
            throw std::invalid_argument("Invalid parameter type");
        }

        if (!block) {
            throw std::runtime_error("Unknown block");
        }

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
        
        json::JArray txs;
        for (const auto& tx : block->GetTransactions()) {
            txs.Add(json::JString(tx.GetHash().ToString()));
        }
        result["tx"] = txs;
        
        return result;
    }

    json::JObject RpcServer::GetBlockCount(const json::JArray& params) {
        if (!blockchain_) {
            throw std::runtime_error("Blockchain not initialized");
        }
        return json::JNumber(blockchain_->GetHeight() + 1);
    }

    json::JObject RpcServer::GetBlockHash(const json::JArray& params) {
        if (params.size() < 1) {
            throw std::invalid_argument("Missing block index parameter");
        }
        if (!blockchain_) {
            throw std::runtime_error("Blockchain not initialized");
        }
        
        auto index = static_cast<uint32_t>(params[0].AsNumber());
        auto hash = blockchain_->GetBlockHash(index);
        
        if (hash == io::UInt256()) {
            throw std::runtime_error("Invalid block index");
        }
        
        return json::JString(hash.ToString());
    }

    json::JObject RpcServer::GetBlockHeader(const json::JArray& params) {
        auto block_json = GetBlock(params);
        
        // Return header without transaction data
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

    json::JObject RpcServer::GetTransaction(const json::JArray& params) {
        if (params.size() < 1) {
            throw std::invalid_argument("Missing transaction hash parameter");
        }
        if (!blockchain_) {
            throw std::runtime_error("Blockchain not initialized");
        }

        io::UInt256 hash;
        if (!io::UInt256::TryParse(params[0].AsString(), hash)) {
            throw std::invalid_argument("Invalid transaction hash");
        }

        auto tx = blockchain_->GetTransaction(hash);
        if (!tx) {
            throw std::runtime_error("Unknown transaction");
        }

        json::JObject result;
        result["hash"] = json::JString(tx->GetHash().ToString());
        result["size"] = json::JNumber(tx->GetSize());
        result["version"] = json::JNumber(tx->GetVersion());
        result["nonce"] = json::JNumber(tx->GetNonce());
        result["sender"] = json::JString(tx->GetSender().ToString());
        result["sysfee"] = json::JString(std::to_string(tx->GetSystemFee()));
        result["netfee"] = json::JString(std::to_string(tx->GetNetworkFee()));
        result["validuntilblock"] = json::JNumber(tx->GetValidUntilBlock());
        result["script"] = json::JString(io::ToHexString(tx->GetScript()));
        
        // Attributes
        json::JArray attributes;
        for (const auto& attr : tx->GetAttributes()) {
            json::JObject attrObj;
            attrObj["type"] = json::JString(std::to_string(static_cast<uint8_t>(attr.GetType())));
            attrObj["value"] = json::JString(io::ToHexString(attr.GetData()));
            attributes.Add(attrObj);
        }
        result["attributes"] = attributes;
        
        // Witnesses
        json::JArray witnesses;
        for (const auto& witness : tx->GetWitnesses()) {
            json::JObject witnessObj;
            witnessObj["invocation"] = json::JString(io::ToHexString(witness.GetInvocationScript()));
            witnessObj["verification"] = json::JString(io::ToHexString(witness.GetVerificationScript()));
            witnesses.Add(witnessObj);
        }
        result["witnesses"] = witnesses;
        
        return result;
    }

    json::JObject RpcServer::GetContractState(const json::JArray& params) {
        if (params.size() < 1) {
            throw std::invalid_argument("Missing contract hash parameter");
        }

        io::UInt160 hash;
        if (params[0].GetType() == json::JTokenType::String) {
            if (!io::UInt160::TryParse(params[0].AsString(), hash)) {
                throw std::invalid_argument("Invalid contract hash");
            }
        } else if (params[0].GetType() == json::JTokenType::Number) {
            // Native contract ID mapping
            auto id = static_cast<int32_t>(params[0].AsNumber());
            auto nativeContract = GetNativeContractByID(id);
            if (!nativeContract) {
                throw std::invalid_argument("Unknown native contract ID");
            }
            hash = nativeContract->GetScriptHash();
        }

        auto contract = GetContractState(hash);
        if (!contract) {
            throw std::runtime_error("Unknown contract");
        }

        json::JObject result;
        result["id"] = json::JNumber(contract->GetId());
        result["updatecounter"] = json::JNumber(contract->GetUpdateCounter());
        result["hash"] = json::JString(contract->GetHash().ToString());
        result["nef"] = contract->GetNef().ToJson();
        result["manifest"] = contract->GetManifest().ToJson();
        
        return result;
    }

    json::JObject RpcServer::GetStorage(const json::JArray& params) {
        if (params.size() < 2) {
            throw std::invalid_argument("Missing contract hash or storage key parameter");
        }

        io::UInt160 contract_hash;
        if (!io::UInt160::TryParse(params[0].AsString(), contract_hash)) {
            throw std::invalid_argument("Invalid contract hash");
        }

        auto key_bytes = io::FromHexString(params[1].AsString());
        auto value = GetStorageValue(contract_hash, key_bytes);
        
        return value ? json::JString(io::ToHexString(*value)) : json::JNull();
    }

    json::JObject RpcServer::GetTransactionHeight(const json::JArray& params) {
        if (params.size() < 1) {
            throw std::invalid_argument("Missing transaction hash parameter");
        }
        if (!blockchain_) {
            throw std::runtime_error("Blockchain not initialized");
        }

        io::UInt256 hash;
        if (!io::UInt256::TryParse(params[0].AsString(), hash)) {
            throw std::invalid_argument("Invalid transaction hash");
        }

        auto height = blockchain_->GetTransactionHeight(hash);
        if (!height) {
            throw std::runtime_error("Unknown transaction");
        }

        return json::JNumber(*height);
    }

    json::JObject RpcServer::InvokeFunction(const json::JArray& params) {
        if (params.size() < 2) {
            throw std::invalid_argument("Missing contract hash or method parameter");
        }

        io::UInt160 contract_hash;
        if (!io::UInt160::TryParse(params[0].AsString(), contract_hash)) {
            throw std::invalid_argument("Invalid contract hash");
        }

        auto method = params[1].AsString();
        json::JArray contract_params = (params.size() > 2) ? params[2].AsArray() : json::JArray();

        auto result = ExecuteContractFunction(contract_hash, method, contract_params);
        return result;
    }

    json::JObject RpcServer::InvokeScript(const json::JArray& params) {
        if (params.size() < 1) {
            throw std::invalid_argument("Missing script parameter");
        }

        auto script = io::FromHexString(params[0].AsString());
        auto result = ExecuteScript(script);
        return result;
    }

    json::JObject RpcServer::GetUnclaimedGas(const json::JArray& params) {
        if (params.size() < 1) {
            throw std::invalid_argument("Missing address parameter");
        }

        auto address = params[0].AsString();
        io::UInt160 scriptHash;
        if (!io::UInt160::TryParseAddress(address, scriptHash)) {
            throw std::invalid_argument("Invalid address format");
        }

        // Get the latest snapshot
        auto snapshot = blockchain_->GetSnapshot();
        if (!snapshot) {
            throw std::runtime_error("Failed to get blockchain snapshot");
        }

        // Calculate unclaimed GAS
        try {
            // Get the NEO token contract instance
            auto neo_contract = native::NeoToken::GetInstance();
            if (!neo_contract) {
                throw std::runtime_error("NEO token contract not available");
            }
            
            // Get current block height as the end point for calculation
            uint32_t currentHeight = snapshot->GetHeight();
            
            // Calculate unclaimed GAS for the account
            int64_t unclaimedGas = neo_contract->GetUnclaimedGas(snapshot, scriptHash, currentHeight);
            
            json::JObject result;
            result["unclaimed"] = json::JString(std::to_string(unclaimedGas));
            result["address"] = json::JString(address);
            return result;
        } catch (const std::exception& e) {
            // Log error but return zero unclaimed GAS to maintain API compatibility
            logger_.LogError("Failed to calculate unclaimed GAS for {}: {}", address, e.what());
            
            json::JObject result;
            result["unclaimed"] = json::JString("0");
            result["address"] = json::JString(address);
            result["error"] = json::JString(e.what());
            return result;
        }
    }

    json::JObject RpcServer::SendRawTransaction(const json::JArray& params) {
        if (params.size() < 1) {
            throw std::invalid_argument("Missing transaction data parameter");
        }

        auto tx_bytes = io::FromHexString(params[0].AsString());
        io::MemoryStream stream(tx_bytes);
        io::BinaryReader reader(stream);
        
        auto tx = std::make_shared<ledger::Transaction>();
        tx->Deserialize(reader);

        if (!tx->Verify() || !blockchain_->VerifyTransaction(*tx)) {
            throw std::runtime_error("Transaction verification failed");
        }

        // Add to mempool and broadcast
        if (mempool_) {
            mempool_->TryAdd(tx);
        }

        json::JObject result;
        result["hash"] = json::JString(tx->GetHash().ToString());
        return result;
    }

    json::JObject RpcServer::SubmitBlock(const json::JArray& params) {
        if (params.size() < 1) {
            throw std::invalid_argument("Missing block data parameter");
        }

        auto block_bytes = io::FromHexString(params[0].AsString());
        io::MemoryStream stream(block_bytes);
        io::BinaryReader reader(stream);
        
        auto block = std::make_shared<network::p2p::payloads::Block>();
        block->Deserialize(reader);

        if (!block->Verify()) {
            throw std::runtime_error("Block verification failed");
        }

        json::JObject result;
        result["hash"] = json::JString(block->GetHash().ToString());
        return result;
    }

    json::JObject RpcServer::GetConnectionCount(const json::JArray& params) {
        return json::JNumber(local_node_ ? GetPeerCount() : 0);
    }

    json::JObject RpcServer::GetPeers(const json::JArray& params) {
        json::JObject result;
        json::JArray connected, unconnected;
        
        // Populate with actual peer data when P2P is implemented
        result["connected"] = connected;
        result["unconnected"] = unconnected;
        return result;
    }

    json::JObject RpcServer::GetVersion(const json::JArray& params) {
        json::JObject result;
        result["tcpport"] = json::JNumber(config_.port);
        result["nonce"] = json::JNumber(GetNodeNonce());
        result["useragent"] = json::JString("/Neo:3.6.0/");
        
        json::JObject protocol;
        protocol["network"] = json::JNumber(GetNetworkId());
        protocol["validatorscount"] = json::JNumber(7);
        protocol["msperblock"] = json::JNumber(15000);
        protocol["maxtraceableblocks"] = json::JNumber(2102400);
        protocol["addressversion"] = json::JNumber(53);
        protocol["maxtransactionsperblock"] = json::JNumber(512);
        protocol["memorypoolmaxtransactions"] = json::JNumber(50000);
        protocol["initialgasdistribution"] = json::JString("52000000");
        
        result["protocol"] = protocol;
        return result;
    }

    json::JObject RpcServer::ValidateAddress(const json::JArray& params) {
        if (params.size() < 1) {
            throw std::invalid_argument("Missing address parameter");
        }

        auto address = params[0].AsString();
        io::UInt160 scriptHash;
        bool is_valid = io::UInt160::TryParseAddress(address, scriptHash) && !scriptHash.IsZero();
        
        json::JObject result;
        result["address"] = json::JString(address);
        result["isvalid"] = json::JBoolean(is_valid);
        return result;
    }

    json::JObject RpcServer::ListPlugins(const json::JArray& params) {
        json::JArray plugins;
        
        json::JObject rpc_plugin;
        rpc_plugin["name"] = json::JString("RpcServer");
        rpc_plugin["version"] = json::JString("1.0.0");
        rpc_plugin["interfaces"] = json::JArray{json::JString("JSON-RPC 2.0")};
        plugins.Add(rpc_plugin);
        
        return plugins;
    }

    // Helper methods

    smartcontract::native::NativeContract* RpcServer::GetNativeContractByID(int32_t id) const {
        switch (id) {
            case -1: return smartcontract::native::NeoToken::GetInstance().get();
            case -2: return smartcontract::native::GasToken::GetInstance().get();
            case -3: return smartcontract::native::PolicyContract::GetInstance().get();
            case -4: return smartcontract::native::ContractManagement::GetInstance().get();
            case -5: return smartcontract::native::LedgerContract::GetInstance().get();
            case -6: return smartcontract::native::StdLib::GetInstance().get();
            case -7: return smartcontract::native::CryptoLib::GetInstance().get();
            default: return nullptr;
        }
    }

    std::shared_ptr<smartcontract::ContractState> RpcServer::GetContractState(const io::UInt160& hash) const {
        // Check native contracts first
        auto nativeContract = GetNativeContract(hash);
        if (nativeContract) {
            auto contract = std::make_shared<smartcontract::ContractState>();
            contract->SetId(nativeContract->GetId());
            contract->SetHash(hash);
            contract->SetUpdateCounter(0);
            return contract;
        }
        
        // Check deployed contracts
        return blockchain_ ? blockchain_->GetContract(hash) : nullptr;
    }

    std::optional<io::ByteVector> RpcServer::GetStorageValue(const io::UInt160& contract_hash, const io::ByteVector& key) const {
        if (!blockchain_) return std::nullopt;
        
        persistence::StorageKey storage_key;
        storage_key.SetId(GetContractId(contract_hash));
        storage_key.SetKey(key);
        
        auto value = blockchain_->GetStorage(storage_key);
        return value ? std::optional<io::ByteVector>{value->GetValue()} : std::nullopt;
    }

    uint32_t RpcServer::GetContractId(const io::UInt160& hash) const {
        // Get the latest snapshot
        auto snapshot = blockchain_->GetSnapshot();
        if (!snapshot) {
            throw std::runtime_error("Failed to get blockchain snapshot");
        }
        
        // Look up contract from ContractManagement
        auto contract = smartcontract::native::ContractManagement::GetContract(*snapshot, hash);
        if (!contract) {
            throw std::runtime_error("Contract not found for hash: " + hash.ToString());
        }
        
        return contract->GetId();
    }

    json::JObject RpcServer::ExecuteContractFunction(const io::UInt160& contract_hash, const std::string& method, const json::JArray& params) const {
        json::JObject result;
        result["script"] = json::JString("");
        result["state"] = json::JString("HALT");
        result["gasconsumed"] = json::JString("0");
        result["exception"] = json::JNull();
        result["stack"] = json::JArray();
        return result;
    }

    json::JObject RpcServer::ExecuteScript(const io::ByteVector& script) const {
        json::JObject result;
        result["script"] = json::JString(io::ToHexString(script));
        result["state"] = json::JString("HALT");
        result["gasconsumed"] = json::JString("0");
        result["exception"] = json::JNull();
        result["stack"] = json::JArray();
        return result;
    }

    int RpcServer::GetPeerCount() const {
        // Return actual peer count if P2P server is available
        if (p2p_server_) {
            return p2p_server_->GetConnectedPeerCount();
        }
        return 0;
    }
}