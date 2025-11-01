/**
 * @file rpc_server.cpp
 * @brief JSON-RPC server implementation
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/cryptography/base64.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json.h>
#include <neo/io/json_writer.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/core/neo_system.h>
#include <neo/ledger/block.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/memory_pool.h>
#include <neo/ledger/transaction.h>
#include <neo/network/p2p/local_node.h>
#include <neo/network/p2p/peer.h>
#include <neo/network/p2p/peer_list.h>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>
#include <neo/rpc/rpc_server.h>
#include <neo/smartcontract/contract_state.h>
#include <neo/smartcontract/native/native_contract_manager.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/role_management.h>

#include <nlohmann/json.hpp>
#include <sstream>
#include <thread>

#if defined(NEO_HAS_HTTPLIB) || defined(HAS_HTTPLIB)
#include <httplib.h>
#define NEO_RPC_HAS_HTTPLIB
#endif

namespace neo::rpc
{
using json = nlohmann::json;

RpcServer::RpcServer(const RpcConfig& config)
    : config_(config),
      running_(false),
      total_requests_(0),
      failed_requests_(0),
      start_time_(std::chrono::steady_clock::now())
{
    logger_ = &logging::Logger::Instance();
}

RpcServer::RpcServer(const RpcConfig& config, std::shared_ptr<NeoSystem> system)
    : RpcServer(config)
{
    system_ = std::move(system);
}

RpcServer::~RpcServer() { Stop(); }

void RpcServer::Start()
{
    if (running_.exchange(true)) return;

    const std::string message = "Starting RPC server on " + config_.bind_address + ":" + std::to_string(config_.port);
    logger_->Info(message);

#ifdef NEO_RPC_HAS_HTTPLIB
    server_thread_ = std::thread(
        [this]()
        {
            httplib::Server server;

            server.Post("/",
                        [this](const httplib::Request& req, httplib::Response& res)
                        {
                            total_requests_++;

                            try
                            {
                                auto request_json = json::parse(req.body);
                                io::JsonValue request(request_json);
                                auto response = ProcessRequest(request);
                                res.set_content(response.ToString(), "application/json");
                            }
                            catch (const std::exception& e)
                            {
                                failed_requests_++;
                                json error_response = {
                                    {"jsonrpc", "2.0"},
                                    {"error", {{"code", -32700}, {"message", "Parse error"}, {"data", e.what()}}},
                                    {"id", nullptr}};
                                res.set_content(error_response.dump(), "application/json");
                            }
                        });

            server.listen(config_.bind_address.c_str(), config_.port);
        });
#else
    logger_->Error("HTTP server not available - RPC server cannot start");
    running_ = false;
#endif
}

void RpcServer::Stop()
{
    if (!running_.exchange(false)) return;

    logger_->Info("Stopping RPC server");

#ifdef NEO_RPC_HAS_HTTPLIB
    if (server_thread_.joinable())
    {
        server_thread_.join();
    }
#endif
}

bool RpcServer::IsRunning() const { return running_.load(); }

#ifdef NEO_RPC_HAS_HTTPLIB
std::string RpcServer::GetClientIP(const httplib::Request& req) const
{
    // Check for X-Forwarded-For header first (proxy support)
    if (req.has_header("X-Forwarded-For"))
    {
        auto forwarded = req.get_header_value("X-Forwarded-For");
        // Take the first IP in the chain
        auto pos = forwarded.find(',');
        return pos != std::string::npos ? forwarded.substr(0, pos) : forwarded;
    }
    
    // Check for X-Real-IP header
    if (req.has_header("X-Real-IP"))
    {
        return req.get_header_value("X-Real-IP");
    }
    
    // Use remote address as fallback
    return req.remote_addr;
}

bool RpcServer::IsAuthenticated(const httplib::Request& req) const
{
    if (!config_.enable_authentication)
    {
        return true;
    }
    
    if (!req.has_header("Authorization"))
    {
        return false;
    }
    
    auto auth_header = req.get_header_value("Authorization");
    if (auth_header.substr(0, 6) != "Basic ")
    {
        return false;
    }
    
    // TODO: Implement proper base64 decoding and credential validation
    // For now, this is a placeholder - in production, implement secure authentication
    return true; // PLACEHOLDER: Implement proper authentication
}
#endif

bool RpcServer::IsMethodAllowed(const io::JsonValue& request) const
{
    if (config_.disabled_methods.empty())
    {
        return true;
    }

    auto methodValue = request.GetValue("method");
    if (!methodValue.IsString())
    {
        return false;
    }

    const std::string method = methodValue.GetString();
    for (const auto& disabled_method : config_.disabled_methods)
    {
        if (method == disabled_method)
        {
            return false;
        }
    }

    return true;
}

io::JsonValue RpcServer::GetStatistics() const
{
    json stats = {
        {"totalRequests", total_requests_.load()},
        {"failedRequests", failed_requests_.load()},
        {"uptime",
         std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start_time_).count()}};
    return io::JsonValue(stats);
}

io::JsonValue RpcServer::ProcessRequest(const io::JsonValue& request)
{
    const auto validation_error = ValidateRequest(request);
    if (!validation_error.empty())
    {
        return CreateErrorResponse(request.GetValue("id"), -32600, validation_error);
    }

    auto id = request.GetValue("id");

    if (!IsMethodAllowed(request))
    {
        return CreateErrorResponse(id, -32601, "Method not allowed");
    }

    const auto methodValue = request.GetValue("method");
    const std::string method = methodValue.IsString() ? methodValue.GetString() : std::string();

    try
    {
        if (!system_)
        {
            return CreateErrorResponse(id, -32603, "Node system not available");
        }

        auto params = request.HasMember("params") ? request.GetValue("params") : io::JsonValue::CreateArray();

        if (method == "getblockcount")
        {
            const uint32_t height = system_->GetCurrentBlockHeight();
            io::JsonValue result;
            result.SetInt64(static_cast<int64_t>(height) + 1);
            return CreateResponse(id, result);
        }
        if (method == "getversion")
        {
            const auto& settings = system_->settings();
            json versionJson = {
                {"useragent", "NeoCppNode/0.1"},
                {"protocol", settings.GetNetwork()},
                {"port", config_.port}};
            return CreateResponse(id, io::JsonValue(versionJson));
        }
        if (method == "getblockhash")
        {
            if (!params.IsArray() || params.Size() == 0 || !params[0].IsNumber())
            {
                return CreateErrorResponse(id, -32602, "Invalid params");
            }
            const uint32_t index = static_cast<uint32_t>(params[0].GetInt64());
            auto blockchain = system_->GetBlockchain();
            if (!blockchain)
            {
                return CreateErrorResponse(id, -32603, "Blockchain unavailable");
            }
            const auto hash = blockchain->GetBlockHash(index);
            if (hash.IsZero())
            {
                return CreateErrorResponse(id, -100, "Unknown block");
            }
            io::JsonValue result;
            result.SetString(hash.ToString());
            return CreateResponse(id, result);
        }
        if (method == "getbestblockhash")
        {
            auto blockchain = system_->GetBlockchain();
            if (!blockchain)
            {
                return CreateErrorResponse(id, -32603, "Blockchain unavailable");
            }
            const uint32_t height = system_->GetCurrentBlockHeight();
            const auto hash = blockchain->GetBlockHash(height);
            if (hash.IsZero())
            {
                return CreateErrorResponse(id, -104, "Unknown block");
            }
            io::JsonValue result;
            result.SetString(hash.ToString());
            return CreateResponse(id, result);
        }
        if (method == "getblock")
        {
            if (!params.IsArray() || params.Size() == 0)
            {
                return CreateErrorResponse(id, -32602, "Invalid params");
            }

            auto blockchain = system_->GetBlockchain();
            if (!blockchain)
            {
                return CreateErrorResponse(id, -32603, "Blockchain unavailable");
            }

            std::shared_ptr<ledger::Block> block;
            const auto target = params[0];
            if (target.IsString())
            {
                io::UInt256 hash;
                try
                {
                    hash = io::UInt256::FromString(target.GetString());
                }
                catch (const std::exception&)
                {
                    return CreateErrorResponse(id, -32602, "Invalid block hash");
                }
                block = blockchain->GetBlock(hash);
            }
            else if (target.IsNumber())
            {
                const uint32_t index = static_cast<uint32_t>(target.GetInt64());
                auto hash = blockchain->GetBlockHash(index);
                if (!hash.IsZero())
                {
                    block = blockchain->GetBlock(hash);
                }
            }
            else
            {
                return CreateErrorResponse(id, -32602, "Invalid params");
            }

            if (!block)
            {
                return CreateErrorResponse(id, -100, "Unknown block");
            }

            bool verbose = false;
            if (params.Size() > 1 && params[1].IsBoolean())
            {
                verbose = params[1].GetBoolean();
            }

            if (!verbose)
            {
                std::ostringstream stream;
                io::BinaryWriter writer(stream);
                block->Serialize(writer);
                const auto data = stream.str();
                std::string encoded = cryptography::Base64::Encode(
                    io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
                io::JsonValue result;
                result.SetString(encoded);
                return CreateResponse(id, result);
            }

            nlohmann::json blockJson;
            {
                io::JsonWriter headerWriter(blockJson);
                block->GetHeader().SerializeJson(headerWriter);
            }
            nlohmann::json txArray = nlohmann::json::array();
            for (const auto& tx : block->GetTransactions())
            {
                nlohmann::json txJson;
                io::JsonWriter txWriter(txJson);
                tx.SerializeJson(txWriter);
                txArray.push_back(std::move(txJson));
            }
            blockJson["tx"] = std::move(txArray);
            return CreateResponse(id, io::JsonValue(blockJson));
        }
        if (method == "getblockheader")
        {
            if (!params.IsArray() || params.Size() == 0)
            {
                return CreateErrorResponse(id, -32602, "Invalid params");
            }

            auto blockchain = system_->GetBlockchain();
            if (!blockchain)
            {
                return CreateErrorResponse(id, -32603, "Blockchain unavailable");
            }

            std::shared_ptr<ledger::Block> block;
            const auto target = params[0];
            if (target.IsString())
            {
                io::UInt256 hash;
                try
                {
                    hash = io::UInt256::FromString(target.GetString());
                }
                catch (const std::exception&)
                {
                    return CreateErrorResponse(id, -32602, "Invalid block hash");
                }
                block = blockchain->GetBlock(hash);
            }
            else if (target.IsNumber())
            {
                const uint32_t index = static_cast<uint32_t>(target.GetInt64());
                auto hash = blockchain->GetBlockHash(index);
                if (!hash.IsZero())
                {
                    block = blockchain->GetBlock(hash);
                }
            }
            else
            {
                return CreateErrorResponse(id, -32602, "Invalid params");
            }

            if (!block)
            {
                return CreateErrorResponse(id, -100, "Unknown block");
            }

            nlohmann::json headerJson;
            io::JsonWriter writer(headerJson);
            block->GetHeader().SerializeJson(writer);
            headerJson.erase("tx");
            return CreateResponse(id, io::JsonValue(headerJson));
        }
        if (method == "getcontractstate")
        {
            if (!params.IsArray() || params.Size() == 0 || !params[0].IsString())
            {
                return CreateErrorResponse(id, -32602, "Invalid params");
            }

            io::UInt160 script_hash;
            try
            {
                script_hash = io::UInt160::FromString(params[0].GetString());
            }
            catch (const std::exception&)
            {
                return CreateErrorResponse(id, -32602, "Invalid contract hash");
            }

            auto blockchain = system_->GetBlockchain();
            if (!blockchain)
            {
                return CreateErrorResponse(id, -32603, "Blockchain unavailable");
            }

            auto contract = blockchain->GetContract(script_hash);
            if (!contract)
            {
                return CreateErrorResponse(id, static_cast<int>(RpcError::UnknownContract), "Contract not found");
            }

            nlohmann::json contract_json;
            contract_json["id"] = contract->GetId();
            contract_json["updatecounter"] = contract->GetUpdateCounter();
            contract_json["hash"] = contract->GetScriptHash().ToString();

            const auto& script_bytes = contract->GetScript();
            std::string script_base64;
            if (!script_bytes.empty())
            {
                script_base64 = cryptography::Base64::Encode(
                    io::ByteSpan(reinterpret_cast<const uint8_t*>(script_bytes.Data()), script_bytes.Size()));
            }
            contract_json["script"] = script_base64;

            const auto& manifest = contract->GetManifest();
            if (!manifest.empty())
            {
                try
                {
                    contract_json["manifest"] = nlohmann::json::parse(manifest);
                }
                catch (const std::exception&)
                {
                    contract_json["manifest"] = manifest;
                }
            }
            else
            {
                contract_json["manifest"] = nlohmann::json::object();
            }

            return CreateResponse(id, io::JsonValue(contract_json));
        }
        if (method == "getstorage")
        {
            if (!params.IsArray() || params.Size() < 2 || !params[0].IsString() || !params[1].IsString())
            {
                return CreateErrorResponse(id, -32602, "Invalid params");
            }

            io::UInt160 script_hash;
            try
            {
                script_hash = io::UInt160::FromString(params[0].GetString());
            }
            catch (const std::exception&)
            {
                return CreateErrorResponse(id, -32602, "Invalid contract hash");
            }

            const auto key_bytes = cryptography::Base64::Decode(params[1].GetString());

            auto blockchain = system_->GetBlockchain();
            if (!blockchain)
            {
                return CreateErrorResponse(id, -32603, "Blockchain unavailable");
            }

            auto contract = blockchain->GetContract(script_hash);
            if (!contract)
            {
                return CreateErrorResponse(id, static_cast<int>(RpcError::UnknownContract), "Contract not found");
            }

            auto snapshot = system_->GetSnapshot();
            if (!snapshot)
            {
                return CreateErrorResponse(id, -32603, "Snapshot unavailable");
            }

            persistence::StorageKey storage_key(contract->GetId(), key_bytes);
            auto item = snapshot->TryGet(storage_key);
            if (!item)
            {
                io::JsonValue result;
                result.SetNull();
                return CreateResponse(id, result);
            }

            const auto value_base64 = cryptography::Base64::Encode(item->GetValue().AsSpan());
            io::JsonValue result;
            result.SetString(value_base64);
            return CreateResponse(id, result);
        }
        if (method == "findstorage")
        {
            if (!params.IsArray() || params.Size() < 2 || !params[0].IsString() || !params[1].IsString())
            {
                return CreateErrorResponse(id, -32602, "Invalid params");
            }

            io::UInt160 script_hash;
            try
            {
                script_hash = io::UInt160::FromString(params[0].GetString());
            }
            catch (const std::exception&)
            {
                return CreateErrorResponse(id, -32602, "Invalid contract hash");
            }

            const auto prefix_bytes = cryptography::Base64::Decode(params[1].GetString());

            auto blockchain = system_->GetBlockchain();
            if (!blockchain)
            {
                return CreateErrorResponse(id, -32603, "Blockchain unavailable");
            }

            auto contract = blockchain->GetContract(script_hash);
            if (!contract)
            {
                return CreateErrorResponse(id, static_cast<int>(RpcError::UnknownContract), "Contract not found");
            }

            auto snapshot = system_->GetSnapshot();
            if (!snapshot)
            {
                return CreateErrorResponse(id, -32603, "Snapshot unavailable");
            }

            persistence::StorageKey prefix_key(contract->GetId(), prefix_bytes);
            const auto entries = snapshot->Find(&prefix_key);

            nlohmann::json result;
            result["results"] = nlohmann::json::array();
            result["firstproofpair"] = nullptr;
            result["truncated"] = false;

            for (const auto& entry : entries)
            {
                const auto& storage_key = entry.first;
                const auto& storage_item = entry.second;

                nlohmann::json item;
                item["key"] = cryptography::Base64::Encode(storage_key.GetKey().AsSpan());
                item["value"] = cryptography::Base64::Encode(storage_item.GetValue().AsSpan());
                result["results"].push_back(std::move(item));
            }

            return CreateResponse(id, io::JsonValue(result));
        }
        if (method == "getblockheadercount")
        {
            const uint32_t height = system_->GetCurrentBlockHeight();
            io::JsonValue result;
            result.SetInt64(static_cast<int64_t>(height) + 1);
            return CreateResponse(id, result);
        }
        if (method == "getconnectioncount")
        {
            size_t connection_count = 0;
            if (auto* local_node = system_->GetLocalNode())
            {
                connection_count = local_node->GetConnectedCount();
            }

            io::JsonValue result;
            result.SetInt64(static_cast<int64_t>(connection_count));
            return CreateResponse(id, result);
        }
        if (method == "getpeers")
        {
            nlohmann::json result;
            result["connected"] = nlohmann::json::array();
            result["unconnected"] = nlohmann::json::array();
            result["bad"] = nlohmann::json::array();

            if (auto* local_node = system_->GetLocalNode())
            {
                const auto peers = local_node->GetConnectedNodes();
                for (const auto* peer : peers)
                {
                    if (!peer) continue;

                    nlohmann::json peer_json;
                    const auto endpoint = peer->GetRemoteEndPoint();
                    peer_json["address"] = endpoint.ToString();
                    peer_json["port"] = endpoint.GetPort();
                    peer_json["isConnected"] = peer->IsConnected();

                    if (auto connection = peer->GetConnection())
                    {
                        peer_json["lastMessageReceived"] =
                            static_cast<int64_t>(connection->GetLastMessageReceived());
                        peer_json["lastMessageSent"] =
                            static_cast<int64_t>(connection->GetLastMessageSent());
                        peer_json["ping"] = static_cast<int64_t>(connection->GetPingTime());
                        peer_json["bytesSent"] = static_cast<int64_t>(connection->GetBytesSent());
                        peer_json["bytesReceived"] = static_cast<int64_t>(connection->GetBytesReceived());
                    }

                    result["connected"].push_back(std::move(peer_json));
                }

                auto& peer_list = local_node->GetPeerList();

                const auto append_peer = [](nlohmann::json& array, const network::p2p::Peer& peer)
                {
                    nlohmann::json entry;
                    entry["address"] = peer.GetEndPoint().ToString();
                    entry["port"] = peer.GetEndPoint().GetPort();
                    entry["lastSeen"] = static_cast<int64_t>(peer.GetLastSeenTime());
                    entry["lastConnection"] = static_cast<int64_t>(peer.GetLastConnectionTime());
                    entry["attempts"] = static_cast<int64_t>(peer.GetConnectionAttempts());
                    entry["version"] = peer.GetVersion();
                    array.push_back(std::move(entry));
                };

                for (const auto& peer : peer_list.GetUnconnectedPeers())
                {
                    append_peer(result["unconnected"], peer);
                }

                for (const auto& peer : peer_list.GetBadPeers())
                {
                    append_peer(result["bad"], peer);
                }
            }

            return CreateResponse(id, io::JsonValue(result));
        }
        if (method == "getcommittee")
        {
            auto neo_token = system_->GetNeoToken();
            if (!neo_token)
            {
                return CreateErrorResponse(id, -32603, "Native contracts unavailable");
            }

            auto snapshot = system_->GetSnapshot();
            if (!snapshot)
            {
                return CreateErrorResponse(id, -32603, "Snapshot unavailable");
            }

            const auto committee = neo_token->GetCommittee(snapshot);
            nlohmann::json committee_json = nlohmann::json::array();
            for (const auto& member : committee)
            {
                committee_json.push_back(member.ToString());
            }

            return CreateResponse(id, io::JsonValue(committee_json));
        }
        if (method == "getvalidators")
        {
            auto role_management = system_->GetRoleManagement();
            if (!role_management)
            {
                return CreateErrorResponse(id, -32603, "Role management unavailable");
            }

            auto snapshot = system_->GetSnapshot();
            if (!snapshot)
            {
                return CreateErrorResponse(id, -32603, "Snapshot unavailable");
            }

            const auto current_height = system_->GetCurrentBlockHeight();
            const auto validators = role_management->GetDesignatedByRole(
                snapshot, smartcontract::native::RoleManagement::ROLE_STATE_VALIDATOR, current_height + 1);

            nlohmann::json result = nlohmann::json::array();
            for (const auto& validator : validators)
            {
                nlohmann::json entry;
                entry["publickey"] = validator.ToString();
                entry["votes"] = 0;
                entry["active"] = true;
                result.push_back(std::move(entry));
            }

            return CreateResponse(id, io::JsonValue(result));
        }
        if (method == "getcandidates")
        {
            auto neo_token = system_->GetNeoToken();
            if (!neo_token)
            {
                return CreateErrorResponse(id, -32603, "Native contracts unavailable");
            }

            auto snapshot = system_->GetSnapshot();
            if (!snapshot)
            {
                return CreateErrorResponse(id, -32603, "Snapshot unavailable");
            }

            const auto candidates = neo_token->GetCandidates(snapshot);
            nlohmann::json result = nlohmann::json::array();
            for (const auto& candidate : candidates)
            {
                nlohmann::json entry;
                entry["publickey"] = candidate.first.ToString();
                entry["votes"] = candidate.second.votes;
                entry["active"] = candidate.second.registered;
                result.push_back(std::move(entry));
            }

            return CreateResponse(id, io::JsonValue(result));
        }
        if (method == "getnextblockvalidators")
        {
            auto neo_token = system_->GetNeoToken();
            if (!neo_token)
            {
                return CreateErrorResponse(id, -32603, "Native contracts unavailable");
            }

            auto snapshot = system_->GetSnapshot();
            if (!snapshot)
            {
                return CreateErrorResponse(id, -32603, "Snapshot unavailable");
            }

            const int validators_count = system_->settings().GetValidatorsCount();
            const auto validators = neo_token->GetNextBlockValidators(snapshot, validators_count);

            nlohmann::json result = nlohmann::json::array();
            for (const auto& validator : validators)
            {
                result.push_back(validator.ToString());
            }

            return CreateResponse(id, io::JsonValue(result));
        }
        if (method == "getaccountstate")
        {
            if (!params.IsArray() || params.Size() == 0 || !params[0].IsString())
            {
                return CreateErrorResponse(id, -32602, "Invalid params");
            }

            io::UInt160 account_hash;
            try
            {
                account_hash = io::UInt160::FromString(params[0].GetString());
            }
            catch (const std::exception&)
            {
                return CreateErrorResponse(id, -32602, "Invalid account address");
            }

            auto neo_token = system_->GetNeoToken();
            if (!neo_token)
            {
                return CreateErrorResponse(id, -32603, "Native contracts unavailable");
            }

            auto snapshot = system_->GetSnapshot();
            if (!snapshot)
            {
                return CreateErrorResponse(id, -32603, "Snapshot unavailable");
            }

            try
            {
                const auto state = neo_token->GetAccountState(snapshot, account_hash);
                nlohmann::json result;
                result["address"] = account_hash.ToString();
                result["balance"] = state.balance;
                result["balanceheight"] = state.balanceHeight;
                result["lastgaspervote"] = state.lastGasPerVote;
                if (!state.voteTo.IsInfinity())
                {
                    result["voteto"] = state.voteTo.ToString();
                }
                else
                {
                    result["voteto"] = nullptr;
                }

                return CreateResponse(id, io::JsonValue(result));
            }
            catch (const std::exception& e)
            {
                return CreateErrorResponse(id, static_cast<int>(RpcError::Unknown), std::string("Failed to get account state: ") + e.what());
            }
        }
        if (method == "getnativecontracts")
        {
            nlohmann::json result = nlohmann::json::array();
            auto& manager = smartcontract::native::NativeContractManager::GetInstance();
            const auto& contracts = manager.GetContracts();

            for (const auto& contract : contracts)
            {
                if (!contract) continue;

                nlohmann::json contract_json;
                contract_json["id"] = contract->GetId();
                contract_json["hash"] = contract->GetScriptHash().ToString();

                nlohmann::json nef = nlohmann::json::object();
                nef["magic"] = 0x3346454E;
                nef["compiler"] = contract->GetName();
                nef["tokens"] = nlohmann::json::array();
                nef["script"] = "";
                nef["checksum"] = 0;
                contract_json["nef"] = std::move(nef);

                nlohmann::json manifest = nlohmann::json::object();
                manifest["name"] = contract->GetName();
                manifest["groups"] = nlohmann::json::array();
                manifest["supportedstandards"] = nlohmann::json::array();
                nlohmann::json abi = nlohmann::json::object();
                abi["methods"] = nlohmann::json::array();
                abi["events"] = nlohmann::json::array();
                manifest["abi"] = std::move(abi);
                manifest["permissions"] = nlohmann::json::array();
                manifest["trusts"] = nlohmann::json::array();
                manifest["extra"] = nullptr;
                contract_json["manifest"] = std::move(manifest);

                result.push_back(std::move(contract_json));
            }

            return CreateResponse(id, io::JsonValue(result));
        }
        if (method == "getgasperblock")
        {
            auto neo_token = system_->GetNeoToken();
            if (!neo_token)
            {
                return CreateErrorResponse(id, -32603, "Native contracts unavailable");
            }

            auto snapshot = system_->GetSnapshot();
            if (!snapshot)
            {
                return CreateErrorResponse(id, -32603, "Snapshot unavailable");
            }

            const auto gas_per_block = neo_token->GetGasPerBlock(snapshot);
            io::JsonValue result;
            result.SetInt64(static_cast<int64_t>(gas_per_block));
            return CreateResponse(id, result);
        }
        if (method == "getunclaimedgas")
        {
            if (!params.IsArray() || params.Size() == 0 || !params[0].IsString())
            {
                return CreateErrorResponse(id, -32602, "Invalid params");
            }

            io::UInt160 account;
            try
            {
                account = io::UInt160::FromString(params[0].GetString());
            }
            catch (const std::exception&)
            {
                return CreateErrorResponse(id, -32602, "Invalid account address");
            }

            auto neo_token = system_->GetNeoToken();
            if (!neo_token)
            {
                return CreateErrorResponse(id, -32603, "Native contracts unavailable");
            }

            auto snapshot = system_->GetSnapshot();
            if (!snapshot)
            {
                return CreateErrorResponse(id, -32603, "Snapshot unavailable");
            }

            uint32_t end_height = system_->GetCurrentBlockHeight();
            if (params.Size() > 1 && params[1].IsNumber())
            {
                const int64_t provided = params[1].GetInt64();
                if (provided < 0)
                {
                    return CreateErrorResponse(id, -32602, "Invalid end height");
                }
                end_height = static_cast<uint32_t>(provided);
            }

            const auto unclaimed = neo_token->GetUnclaimedGas(snapshot, account, end_height);

            nlohmann::json result;
            result["address"] = account.ToString();
            result["unclaimed"] = unclaimed;
            result["endheight"] = end_height;

            return CreateResponse(id, io::JsonValue(result));
        }
        if (method == "getrawmempool")
        {
            const bool include_unverified = params.IsArray() && params.Size() > 0 && params[0].IsBoolean() &&
                                            params[0].GetBoolean();
            auto mempool = system_->GetMemPool();

            if (!include_unverified)
            {
                io::JsonValue result = io::JsonValue::CreateArray();
                if (mempool)
                {
                    const auto transactions = mempool->GetSortedTransactions();
                    for (const auto& tx : transactions)
                    {
                        result.PushBack(tx.GetHash().ToString());
                    }
                }
                return CreateResponse(id, result);
            }

            nlohmann::json result_json;
            result_json["height"] = system_->GetCurrentBlockHeight();
            nlohmann::json verified = nlohmann::json::array();
            nlohmann::json unverified = nlohmann::json::array();

            if (mempool)
            {
                for (const auto& tx : mempool->GetSortedTransactions())
                {
                    verified.push_back(tx.GetHash().ToString());
                }
                for (const auto& tx : mempool->GetUnverifiedTransactions())
                {
                    unverified.push_back(tx.GetHash().ToString());
                }
            }

            result_json["verified"] = std::move(verified);
            result_json["unverified"] = std::move(unverified);
            return CreateResponse(id, io::JsonValue(result_json));
        }
        if (method == "getrawtransaction")
        {
            if (!params.IsArray() || params.Size() == 0 || !params[0].IsString())
            {
                return CreateErrorResponse(id, -32602, "Invalid params");
            }

            io::UInt256 hash;
            try
            {
                hash = io::UInt256::FromString(params[0].GetString());
            }
            catch (const std::exception&)
            {
                return CreateErrorResponse(id, -32602, "Invalid transaction hash");
            }

            bool verbose = false;
            if (params.Size() > 1 && params[1].IsBoolean())
            {
                verbose = params[1].GetBoolean();
            }

            std::shared_ptr<ledger::Transaction> transaction;

            if (auto mempool = system_->GetMemPool())
            {
                if (const auto* pool_tx = mempool->GetTransaction(hash))
                {
                    transaction = std::make_shared<ledger::Transaction>(*pool_tx);
                }
            }

            if (!transaction)
            {
                auto blockchain = system_->GetBlockchain();
                if (!blockchain)
                {
                    return CreateErrorResponse(id, -32603, "Blockchain unavailable");
                }
                transaction = blockchain->GetTransaction(hash);
            }

            if (!transaction)
            {
                return CreateErrorResponse(id, static_cast<int>(RpcError::UnknownTransaction), "Transaction not found");
            }

            if (!verbose)
            {
                std::ostringstream stream;
                io::BinaryWriter writer(stream);
                transaction->Serialize(writer);
                const auto data = stream.str();
                const std::string encoded = cryptography::Base64::Encode(
                    io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
                io::JsonValue result;
                result.SetString(encoded);
                return CreateResponse(id, result);
            }

            nlohmann::json tx_json;
            io::JsonWriter writer(tx_json);
            transaction->SerializeJson(writer);
            return CreateResponse(id, io::JsonValue(tx_json));
        }
        if (method == "gettransactionheight")
        {
            if (!params.IsArray() || params.Size() == 0 || !params[0].IsString())
            {
                return CreateErrorResponse(id, -32602, "Invalid params");
            }

            io::UInt256 hash;
            try
            {
                hash = io::UInt256::FromString(params[0].GetString());
            }
            catch (const std::exception&)
            {
                return CreateErrorResponse(id, -32602, "Invalid transaction hash");
            }

            auto blockchain = system_->GetBlockchain();
            if (!blockchain)
            {
                return CreateErrorResponse(id, -32603, "Blockchain unavailable");
            }

            const int32_t height = blockchain->GetTransactionHeight(hash);
            if (height < 0)
            {
                return CreateErrorResponse(id, static_cast<int>(RpcError::UnknownTransaction), "Transaction not found");
            }

            io::JsonValue result;
            result.SetInt64(static_cast<int64_t>(height));
            return CreateResponse(id, result);
        }
        if (method == "sendrawtransaction")
        {
            if (!params.IsArray() || params.Size() == 0 || !params[0].IsString())
            {
                return CreateErrorResponse(id, -32602, "Invalid params");
            }

            auto mempool = system_->GetMemPool();
            auto blockchain = system_->GetBlockchain();
            if (!mempool || !blockchain)
            {
                return CreateErrorResponse(id, -32603, "Node services unavailable");
            }

            try
            {
                const auto raw = cryptography::Base64::Decode(params[0].GetString());
                std::istringstream stream(std::string(reinterpret_cast<const char*>(raw.Data()), raw.Size()));
                io::BinaryReader reader(stream);

                ledger::Transaction tx;
                tx.Deserialize(reader);
                const auto& hash = tx.GetHash();

                if (blockchain->ContainsTransaction(hash))
                {
                    return CreateErrorResponse(id, static_cast<int>(RpcError::Unknown), "Transaction already exists");
                }

                if (!mempool->TryAdd(tx))
                {
                    return CreateErrorResponse(id, static_cast<int>(RpcError::PolicyFailed),
                                               "Transaction rejected by policy");
                }

                nlohmann::json result = {{"hash", hash.ToString()}};
                return CreateResponse(id, io::JsonValue(result));
            }
            catch (const std::exception& e)
            {
                return CreateErrorResponse(id, -32602, std::string("Invalid transaction data: ") + e.what());
            }
        }
        if (method == "submitblock")
        {
            if (!params.IsArray() || params.Size() == 0 || !params[0].IsString())
            {
                return CreateErrorResponse(id, -32602, "Invalid params");
            }

            auto blockchain = system_->GetBlockchain();
            if (!blockchain)
            {
                return CreateErrorResponse(id, -32603, "Blockchain unavailable");
            }

            try
            {
                const auto raw = cryptography::Base64::Decode(params[0].GetString());
                std::istringstream stream(std::string(reinterpret_cast<const char*>(raw.Data()), raw.Size()));
                io::BinaryReader reader(stream);

                auto block = std::make_shared<ledger::Block>();
                block->Deserialize(reader);

                ledger::ImportData import_data;
                import_data.blocks.push_back(block);
                import_data.verify = true;

                if (!blockchain->ImportBlocks(import_data))
                {
                    return CreateErrorResponse(id, static_cast<int>(RpcError::PolicyFailed),
                                               "Block rejected by blockchain");
                }

                nlohmann::json result = {{"hash", block->GetHash().ToString()}};
                return CreateResponse(id, io::JsonValue(result));
            }
            catch (const std::exception& e)
            {
                return CreateErrorResponse(id, -32602, std::string("Invalid block data: ") + e.what());
            }
        }
        if (method == "validateaddress")
        {
            if (!params.IsArray() || params.Size() == 0 || !params[0].IsString())
            {
                return CreateErrorResponse(id, -32602, "Invalid params");
            }

            const std::string address = params[0].GetString();
            bool is_valid = false;

            try
            {
                io::UInt160::FromAddress(address);
                is_valid = true;
            }
            catch (...)
            {
                try
                {
                    io::UInt160::FromString(address);
                    is_valid = true;
                }
                catch (...)
                {
                    is_valid = false;
                }
            }

            nlohmann::json result = {{"address", address}, {"isvalid", is_valid}};
            return CreateResponse(id, io::JsonValue(result));
        }

        return CreateErrorResponse(id, -32601, "RPC method not implemented");
    }
    catch (const std::exception& e)
    {
        return CreateErrorResponse(id, -32603, std::string("Internal error: ") + e.what());
    }
}

std::string RpcServer::ValidateRequest(const io::JsonValue& request) const
{
    if (!request.IsObject())
    {
        return "Request must be a JSON object";
    }

    auto jsonrpc = request.GetValue("jsonrpc");
    if (!jsonrpc.IsString() || jsonrpc.GetString() != "2.0")
    {
        return "Missing or invalid jsonrpc field";
    }

    auto method = request.GetValue("method");
    if (!method.IsString())
    {
        return "Missing or invalid method field";
    }

    return "";
}

io::JsonValue RpcServer::CreateResponse(const io::JsonValue& id, const io::JsonValue& result) const
{
    json response = {{"jsonrpc", "2.0"}, {"result", result.GetJson()}, {"id", id.GetJson()}};
    return io::JsonValue(response);
}

io::JsonValue RpcServer::CreateErrorResponse(const io::JsonValue& id, int code, const std::string& message) const
{
    json response = {{"jsonrpc", "2.0"},
                     {"error", {{"code", code}, {"message", message}}},
                     {"id", id.IsNull() ? nullptr : id.GetJson()}};
    return io::JsonValue(response);
}

void RpcServer::SetSystem(std::shared_ptr<NeoSystem> system)
{
    system_ = std::move(system);
}

}  // namespace neo::rpc
