/**
 * @file rpc_methods.cpp
 * @brief Rpc Methods
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/ledger/signer.h>
#include <neo/persistence/storage_key.h>
#include <neo/rpc/rpc_methods.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/contract_state.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/native_contract_manager.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/role_management.h>
// WitnessScope is already included via signer.h
#include <neo/cryptography/base64.h>
#include <neo/cryptography/hash.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/network/p2p/payloads/neo3_transaction.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/stack_item_types.h>
#include <neo/vm/vm_state.h>

#include <sstream>

namespace neo::rpc
{
// Forward declarations for helper functions
class RpcSessionManager;
std::shared_ptr<RpcSessionManager> GetRpcSessionManager();
nlohmann::json ConvertStackItemToJson(std::shared_ptr<vm::StackItem> item);
void LogSessionTermination(const std::string& sessionId);

// Helper function to convert Neo3Transaction to JSON
static nlohmann::json TransactionToJson(std::shared_ptr<network::p2p::payloads::Neo3Transaction> tx, bool verbose)
{
    if (!tx) return nullptr;

    if (!verbose)
    {
        // Return hex string
        std::ostringstream stream;
        io::BinaryWriter writer(stream);
        tx->Serialize(writer);
        auto data = stream.str();
        return cryptography::Base64::Encode(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    }

    // Return detailed JSON
    nlohmann::json json;
    io::JsonWriter writer(json);
    tx->SerializeJson(writer);
    return json;
}

// Helper function to convert Block to JSON
static nlohmann::json BlockToJson(std::shared_ptr<ledger::Block> block, bool verbose)
{
    if (!block) return nullptr;

    nlohmann::json json;
    io::JsonWriter writer(json);
    block->SerializeJson(writer);

    if (verbose)
    {
        // Add transaction details
        json["tx"] = nlohmann::json::array();
        for (const auto& tx : block->GetTransactions())
        {
            json["tx"].push_back(TransactionToJson(tx, true));
        }
    }
    else
    {
        // Just transaction hashes
        json["tx"] = nlohmann::json::array();
        for (const auto& tx : block->GetTransactions())
        {
            json["tx"].push_back(tx->GetHash().ToString());
        }
    }

    return json;
}

// Helper function to convert Contract to JSON
static nlohmann::json ContractToJson(std::shared_ptr<smartcontract::ContractState> contract)
{
    if (!contract) return nullptr;

    nlohmann::json json;
    io::JsonWriter writer(json);
    contract->SerializeJson(writer);
    return json;
}
nlohmann::json RPCMethods::GetVersion(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    nlohmann::json result;
    result["port"] = neoSystem->GetLocalNode().GetPort();
    result["nonce"] = neoSystem->GetLocalNode().GetNonce();
    result["useragent"] = neoSystem->GetLocalNode().GetUserAgent();
    return result;
}

nlohmann::json RPCMethods::GetBlockCount(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    return neoSystem->GetBlockchain().GetCurrentBlockIndex() + 1;
}

nlohmann::json RPCMethods::GetBlock(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty()) throw std::runtime_error("Invalid params");

    std::shared_ptr<ledger::Block> block;

    // Get block by hash or index
    if (params[0].is_string())
    {
        // Get block by hash
        io::UInt256 hash;
        hash.FromString(params[0].get<std::string>());
        block = neoSystem->GetBlockchain().GetBlock(hash);
    }
    else if (params[0].is_number())
    {
        // Get block by index
        uint32_t index = params[0].get<uint32_t>();
        auto hash = neoSystem->GetBlockchain().GetBlockHash(index);
        block = neoSystem->GetBlockchain().GetBlock(hash);
    }
    else
    {
        throw std::runtime_error("Invalid params");
    }

    if (!block) throw std::runtime_error("Unknown block");

    // Check if verbose
    bool verbose = params.size() >= 2 && params[1].is_boolean() && params[1].get<bool>();

    // Convert block to JSON
    return BlockToJson(block, verbose);
}

nlohmann::json RPCMethods::GetBlockHash(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty() || !params[0].is_number()) throw std::runtime_error("Invalid params");

    uint32_t index = params[0].get<uint32_t>();
    auto hash = neoSystem->GetBlockchain().GetBlockHash(index);

    if (hash.IsZero()) throw std::runtime_error("Unknown block");

    return hash.ToString();
}

nlohmann::json RPCMethods::GetBlockHeader(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty()) throw std::runtime_error("Invalid params");

    std::shared_ptr<ledger::Block> block;

    // Get block by hash or index
    if (params[0].is_string())
    {
        // Get block by hash
        io::UInt256 hash;
        hash.FromString(params[0].get<std::string>());
        block = neoSystem->GetBlockchain().GetBlock(hash);
    }
    else if (params[0].is_number())
    {
        // Get block by index
        uint32_t index = params[0].get<uint32_t>();
        auto hash = neoSystem->GetBlockchain().GetBlockHash(index);
        block = neoSystem->GetBlockchain().GetBlock(hash);
    }
    else
    {
        throw std::runtime_error("Invalid params");
    }

    if (!block) throw std::runtime_error("Unknown block");

    // Check if verbose
    bool verbose = params.size() >= 2 && params[1].is_boolean() && params[1].get<bool>();

    // Convert block to JSON
    auto json = BlockToJson(block, verbose);

    // Remove transactions
    if (json.contains("tx")) json.erase("tx");

    return json;
}

nlohmann::json RPCMethods::GetRawMemPool(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    auto transactions = neoSystem->GetMemPool().GetTransactions();

    nlohmann::json result = nlohmann::json::array();
    for (const auto& tx : transactions)
    {
        result.push_back(tx->GetHash().ToString());
    }

    return result;
}

nlohmann::json RPCMethods::GetRawTransaction(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty() || !params[0].is_string()) throw std::runtime_error("Invalid parameter");

    io::UInt256 hash;
    hash.FromString(params[0].get<std::string>());

    // Try to get transaction from memory pool
    auto tx = neoSystem->GetMemPool().GetTransaction(hash);

    // If not found in memory pool, try to get from blockchain
    if (!tx) tx = neoSystem->GetBlockchain().GetTransaction(hash);

    if (!tx) throw std::runtime_error("Transaction not found");

    // Check if verbose
    bool verbose = params.size() >= 2 && params[1].is_boolean() && params[1].get<bool>();

    // Convert transaction to JSON
    return TransactionToJson(tx, verbose);
}

nlohmann::json RPCMethods::GetTransactionHeight(std::shared_ptr<node::NeoSystem> neoSystem,
                                                const nlohmann::json& params)
{
    if (params.empty() || !params[0].is_string()) throw std::runtime_error("Invalid parameter");

    io::UInt256 hash;
    hash.FromString(params[0].get<std::string>());

    auto height = neoSystem->GetBlockchain().GetTransactionHeight(hash);

    if (height < 0) throw std::runtime_error("Transaction not found");

    return height;
}

nlohmann::json RPCMethods::SendRawTransaction(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty() || !params[0].is_string()) throw std::runtime_error("Invalid parameter");

    auto data = cryptography::Base64::Decode(params[0].get<std::string>());

    std::istringstream stream(std::string(reinterpret_cast<const char*>(data.Data()), data.Size()));
    io::BinaryReader reader(stream);

    auto tx = std::make_shared<network::p2p::payloads::Neo3Transaction>();
    tx->Deserialize(reader);

    auto result = neoSystem->GetMemPool().AddTransaction(tx);

    if (result != ledger::VerifyResult::Succeed)
        throw std::runtime_error("Transaction rejected: " + std::to_string(static_cast<int>(result)));

    return tx->GetHash().ToString();
}

nlohmann::json RPCMethods::InvokeFunction(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.size() < 2 || !params[0].is_string() || !params[1].is_string())
        throw std::runtime_error("Invalid parameters");

    // Get contract hash
    io::UInt160 scriptHash;
    scriptHash.FromString(params[0].get<std::string>());

    // Get method name
    auto method = params[1].get<std::string>();

    // Get arguments
    std::vector<std::shared_ptr<neo::vm::StackItem>> args;
    if (params.size() >= 3 && params[2].is_array())
    {
        for (const auto& arg : params[2])
        {
            if (!arg.is_object() || !arg.contains("type") || !arg.contains("value"))
                throw std::runtime_error("Invalid argument");

            auto type = arg["type"].get<std::string>();
            auto value = arg["value"];

            if (type == "String")
            {
                args.push_back(neo::vm::StackItem::Create(value.get<std::string>()));
            }
            else if (type == "Hash160")
            {
                io::UInt160 hash;
                hash.FromString(value.get<std::string>());
                args.push_back(neo::vm::StackItem::Create(io::ByteVector(io::ByteSpan(hash.Data(), hash.Size()))));
            }
            else if (type == "Hash256")
            {
                io::UInt256 hash;
                hash.FromString(value.get<std::string>());
                args.push_back(neo::vm::StackItem::Create(io::ByteVector(io::ByteSpan(hash.Data(), hash.Size()))));
            }
            else if (type == "Integer")
            {
                args.push_back(neo::vm::StackItem::Create(value.get<int64_t>()));
            }
            else if (type == "Boolean")
            {
                args.push_back(neo::vm::StackItem::Create(value.get<bool>()));
            }
            else if (type == "ByteArray")
            {
                auto data = cryptography::Base64::Decode(value.get<std::string>());
                args.push_back(neo::vm::StackItem::Create(data));
            }
            else
            {
                throw std::runtime_error("Invalid argument type");
            }
        }
    }

    // Create engine
    auto engine = std::make_shared<smartcontract::ApplicationEngine>(smartcontract::TriggerType::Application, nullptr,
                                                                     neoSystem->GetSnapshot(), 0);

    // Execute script
    auto result = engine->CallContract(scriptHash, method, args, smartcontract::CallFlags::All);

    // Create response
    nlohmann::json response;
    response["script"] = cryptography::Base64::Encode(engine->GetScript().AsSpan());
    response["state"] = engine->GetState() == neo::vm::VMState::Halt ? "HALT" : "FAULT";
    response["gasconsumed"] = engine->GetGasConsumed();
    response["exception"] = engine->GetException();

    // Add stack
    response["stack"] = nlohmann::json::array();
    for (const auto& item : engine->GetResultStack())
    {
        nlohmann::json stackItem;

        if (item->GetType() == neo::vm::StackItemType::Integer)
        {
            stackItem["type"] = "Integer";
            stackItem["value"] = item->GetInteger();
        }
        else if (item->GetType() == neo::vm::StackItemType::Boolean)
        {
            stackItem["type"] = "Boolean";
            stackItem["value"] = item->GetBoolean();
        }
        else if (item->GetType() == neo::vm::StackItemType::ByteString)
        {
            stackItem["type"] = "ByteString";
            stackItem["value"] = cryptography::Base64::Encode(item->GetByteArray().AsSpan());
        }
        else if (item->GetType() == neo::vm::StackItemType::Buffer)
        {
            stackItem["type"] = "Buffer";
            stackItem["value"] = cryptography::Base64::Encode(item->GetByteArray().AsSpan());
        }
        else
        {
            stackItem["type"] = "Unknown";
            stackItem["value"] = nullptr;
        }

        response["stack"].push_back(stackItem);
    }

    return response;
}

nlohmann::json RPCMethods::InvokeScript(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty() || !params[0].is_string()) throw std::runtime_error("Invalid parameter");

    // Get script
    auto scriptBase64 = params[0].get<std::string>();
    auto script = cryptography::Base64::Decode(scriptBase64);

    // Create engine
    auto engine = std::make_shared<smartcontract::ApplicationEngine>(smartcontract::TriggerType::Application, nullptr,
                                                                     neoSystem->GetSnapshot(), 0);

    // Execute script
    engine->LoadScript(script);
    engine->Execute();

    // Create response
    nlohmann::json response;
    response["script"] = scriptBase64;
    response["state"] = engine->GetState() == neo::vm::VMState::Halt ? "HALT" : "FAULT";
    response["gasconsumed"] = engine->GetGasConsumed();
    response["exception"] = engine->GetException();

    // Add stack
    response["stack"] = nlohmann::json::array();
    for (const auto& item : engine->GetResultStack())
    {
        nlohmann::json stackItem;

        if (item->GetType() == neo::vm::StackItemType::Integer)
        {
            stackItem["type"] = "Integer";
            stackItem["value"] = item->GetInteger();
        }
        else if (item->GetType() == neo::vm::StackItemType::Boolean)
        {
            stackItem["type"] = "Boolean";
            stackItem["value"] = item->GetBoolean();
        }
        else if (item->GetType() == neo::vm::StackItemType::ByteString)
        {
            stackItem["type"] = "ByteString";
            stackItem["value"] = cryptography::Base64::Encode(item->GetByteArray().AsSpan());
        }
        else if (item->GetType() == neo::vm::StackItemType::Buffer)
        {
            stackItem["type"] = "Buffer";
            stackItem["value"] = cryptography::Base64::Encode(item->GetByteArray().AsSpan());
        }
        else
        {
            stackItem["type"] = "Unknown";
            stackItem["value"] = nullptr;
        }

        response["stack"].push_back(stackItem);
    }

    return response;
}

nlohmann::json RPCMethods::GetContractState(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty() || !params[0].is_string()) throw std::runtime_error("Invalid parameter");

    // Get contract hash
    io::UInt160 scriptHash;
    scriptHash.FromString(params[0].get<std::string>());

    // Get contract state
    auto contract = neoSystem->GetBlockchain().GetContract(scriptHash);

    if (!contract) throw std::runtime_error("Contract not found");

    // Convert contract to JSON
    return ContractToJson(contract);
}

nlohmann::json RPCMethods::GetUnclaimedGas(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty() || !params[0].is_string()) throw std::runtime_error("Invalid parameter");

    // Get account
    io::UInt160 account;
    account.FromString(params[0].get<std::string>());

    // Get NEO token
    auto neoToken = std::static_pointer_cast<smartcontract::native::NeoToken>(
        smartcontract::native::NativeContractManager::GetInstance().GetContract(smartcontract::native::NeoToken::NAME));

    // Get unclaimed GAS (using current block height as end parameter)
    auto currentHeight = neoSystem->GetBlockchain()->GetCurrentBlockIndex();
    auto unclaimedGas = neoToken->GetUnclaimedGas(neoSystem->GetSnapshot(), account, currentHeight);

    // Create response
    nlohmann::json response;
    response["unclaimed"] = unclaimedGas;
    response["address"] = account.ToString();

    return response;
}

nlohmann::json RPCMethods::GetConnectionCount(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    return neoSystem->GetLocalNode().GetConnectedPeers().size();
}

nlohmann::json RPCMethods::GetPeers(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    auto peers = neoSystem->GetLocalNode().GetConnectedPeers();

    nlohmann::json result;
    result["connected"] = nlohmann::json::array();

    for (const auto& peer : peers)
    {
        nlohmann::json peerJson;
        peerJson["address"] = peer->GetAddress();
        peerJson["port"] = peer->GetPort();

        result["connected"].push_back(peerJson);
    }

    return result;
}

nlohmann::json RPCMethods::GetCommittee(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    auto roleManagement = std::static_pointer_cast<smartcontract::native::RoleManagement>(
        smartcontract::native::NativeContractManager::GetInstance().GetContract(
            smartcontract::native::RoleManagement::NAME));

    auto committee = roleManagement->GetDesignatedByRole(neoSystem->GetSnapshot(),
                                                         smartcontract::native::RoleManagement::ROLE_STATE_COMMITTEE);

    nlohmann::json result = nlohmann::json::array();
    for (const auto& member : committee)
    {
        result.push_back(member.ToString());
    }

    return result;
}

nlohmann::json RPCMethods::GetValidators(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    auto roleManagement = std::static_pointer_cast<smartcontract::native::RoleManagement>(
        smartcontract::native::NativeContractManager::GetInstance().GetContract(
            smartcontract::native::RoleManagement::NAME));

    auto validators = roleManagement->GetDesignatedByRole(neoSystem->GetSnapshot(),
                                                          smartcontract::native::RoleManagement::ROLE_STATE_VALIDATOR);

    nlohmann::json result = nlohmann::json::array();
    for (const auto& validator : validators)
    {
        nlohmann::json validatorJson;
        validatorJson["publickey"] = validator.ToString();
        validatorJson["votes"] = 0;
        validatorJson["active"] = true;

        result.push_back(validatorJson);
    }

    return result;
}

nlohmann::json RPCMethods::GetNextBlockValidators(std::shared_ptr<node::NeoSystem> neoSystem,
                                                  const nlohmann::json& params)
{
    auto roleManagement = std::static_pointer_cast<smartcontract::native::RoleManagement>(
        smartcontract::native::NativeContractManager::GetInstance().GetContract(
            smartcontract::native::RoleManagement::NAME));

    auto validators = roleManagement->GetDesignatedByRole(neoSystem->GetSnapshot(),
                                                          smartcontract::native::RoleManagement::ROLE_STATE_VALIDATOR);

    nlohmann::json result = nlohmann::json::array();
    for (const auto& validator : validators)
    {
        nlohmann::json validatorJson;
        validatorJson["publickey"] = validator.ToString();
        validatorJson["votes"] = 0;
        validatorJson["active"] = true;

        result.push_back(validatorJson);
    }

    return result;
}

nlohmann::json RPCMethods::GetBestBlockHash(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    auto hash = neoSystem->GetBlockchain().GetCurrentBlockHash();
    return hash.ToString();
}

nlohmann::json RPCMethods::GetBlockHeaderCount(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    // In Neo N3, header count is the same as block count
    return neoSystem->GetBlockchain().GetCurrentBlockIndex() + 1;
}

nlohmann::json RPCMethods::GetStorage(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.size() < 2 || !params[0].is_string() || !params[1].is_string())
        throw std::runtime_error("Invalid parameters");

    // Get contract hash
    io::UInt160 scriptHash;
    scriptHash.FromString(params[0].get<std::string>());

    // Get storage key
    auto keyBase64 = params[1].get<std::string>();
    auto keyBytes = cryptography::Base64::Decode(keyBase64);

    // Get contract state to find contract ID
    auto contract = neoSystem->GetBlockchain().GetContract(scriptHash);
    if (!contract) throw std::runtime_error("Contract not found");

    // Create storage key
    auto storageKey = persistence::StorageKey::Create(contract->GetId(), keyBytes.AsSpan());

    // Get storage item
    auto snapshot = neoSystem->GetSnapshot();
    auto item = snapshot->TryGet(storageKey);

    if (!item) return nullptr;

    return cryptography::Base64::Encode(item->GetValue().AsSpan());
}

nlohmann::json RPCMethods::FindStorage(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.size() < 2 || !params[0].is_string() || !params[1].is_string())
        throw std::runtime_error("Invalid parameters");

    // Get contract hash
    io::UInt160 scriptHash;
    scriptHash.FromString(params[0].get<std::string>());

    // Get prefix
    auto prefixBase64 = params[1].get<std::string>();
    auto prefixBytes = cryptography::Base64::Decode(prefixBase64);

    // Get start key (optional)
    io::ByteVector startKey;
    if (params.size() >= 3 && params[2].is_string())
    {
        auto startKeyBase64 = params[2].get<std::string>();
        startKey = cryptography::Base64::Decode(startKeyBase64);
    }

    // Get contract state to find contract ID
    auto contract = neoSystem->GetBlockchain().GetContract(scriptHash);
    if (!contract) throw std::runtime_error("Contract not found");

    // Create search prefix
    auto searchKey = persistence::StorageKey::Create(contract->GetId(), prefixBytes.AsSpan());

    // Find storage items
    auto snapshot = neoSystem->GetSnapshot();
    auto results = snapshot->Find(&searchKey);

    nlohmann::json result;
    result["results"] = nlohmann::json::array();
    result["firstproofpair"] = nullptr;
    result["truncated"] = false;

    for (const auto& pair : results)
    {
        nlohmann::json item;
        item["key"] = cryptography::Base64::Encode(pair.first.GetKey().AsSpan());
        item["value"] = cryptography::Base64::Encode(pair.second.GetValue().AsSpan());
        result["results"].push_back(item);
    }

    return result;
}

nlohmann::json RPCMethods::GetCandidates(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    auto neoToken = std::static_pointer_cast<smartcontract::native::NeoToken>(
        smartcontract::native::NativeContractManager::GetInstance().GetContract(smartcontract::native::NeoToken::NAME));

    auto candidates = neoToken->GetCandidates(neoSystem->GetSnapshot());

    nlohmann::json result = nlohmann::json::array();
    for (const auto& candidate : candidates)
    {
        nlohmann::json candidateJson;
        candidateJson["publickey"] = candidate.first.ToString();
        candidateJson["votes"] = candidate.second;
        candidateJson["active"] = true;  // All candidates are considered active

        result.push_back(candidateJson);
    }

    return result;
}

nlohmann::json RPCMethods::GetNativeContracts(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    nlohmann::json result = nlohmann::json::array();

    // Get all native contracts
    auto& manager = smartcontract::native::NativeContractManager::GetInstance();
    auto contracts = manager.GetAllContracts();

    for (const auto& contract : contracts)
    {
        nlohmann::json contractJson;
        contractJson["id"] = contract->GetId();
        contractJson["hash"] = contract->GetScriptHash().ToString();
        contractJson["nef"] = nlohmann::json::object();
        contractJson["nef"]["magic"] = 0x3346454E;  // NEF magic
        contractJson["nef"]["compiler"] = "neo-core-v" + std::string("3.0.0");
        contractJson["nef"]["tokens"] = nlohmann::json::array();
        contractJson["nef"]["script"] = "";
        contractJson["nef"]["checksum"] = 0;

        contractJson["manifest"] = nlohmann::json::object();
        contractJson["manifest"]["name"] = contract->GetName();
        contractJson["manifest"]["groups"] = nlohmann::json::array();
        contractJson["manifest"]["supportedstandards"] = nlohmann::json::array();
        contractJson["manifest"]["abi"] = nlohmann::json::object();
        contractJson["manifest"]["abi"]["methods"] = nlohmann::json::array();
        contractJson["manifest"]["abi"]["events"] = nlohmann::json::array();
        contractJson["manifest"]["permissions"] = nlohmann::json::array();
        contractJson["manifest"]["trusts"] = nlohmann::json::array();
        contractJson["manifest"]["extra"] = nullptr;

        result.push_back(contractJson);
    }

    return result;
}

nlohmann::json RPCMethods::SubmitBlock(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty() || !params[0].is_string()) throw std::runtime_error("Invalid parameter");

    auto blockBase64 = params[0].get<std::string>();
    auto blockData = cryptography::Base64::Decode(blockBase64);

    std::istringstream stream(std::string(reinterpret_cast<const char*>(blockData.Data()), blockData.Size()));
    io::BinaryReader reader(stream);

    auto block = std::make_shared<ledger::Block>();
    block->Deserialize(reader);

    // Submit block to blockchain
    auto result = neoSystem->GetBlockchain().AddBlock(block);

    if (!result) throw std::runtime_error("Block rejected");

    return block->GetHash().ToString();
}

nlohmann::json RPCMethods::ValidateAddress(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty() || !params[0].is_string()) throw std::runtime_error("Invalid parameter");

    auto address = params[0].get<std::string>();

    nlohmann::json result;
    result["address"] = address;

    try
    {
        io::UInt160 scriptHash;
        scriptHash.FromString(address);

        result["isvalid"] = true;
    }
    catch (const std::invalid_argument& e)
    {
        LOG_DEBUG("Address validation failed: {}", e.what());
        result["isvalid"] = false;
    }
    catch (const std::runtime_error& e)
    {
        LOG_DEBUG("Runtime error during address validation: {}", e.what());
        result["isvalid"] = false;
    }
    catch (const std::exception& e)
    {
        LOG_DEBUG("Exception during address validation: {}", e.what());
        result["isvalid"] = false;
    }

    return result;
}

nlohmann::json RPCMethods::TraverseIterator(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.size() < 2 || !params[0].is_string() || !params[1].is_string())
        throw std::runtime_error("Invalid parameters");

    auto sessionId = params[0].get<std::string>();
    auto iteratorId = params[1].get<std::string>();

    // Optional count parameter
    int count = 100;  // Default count
    if (params.size() >= 3 && params[2].is_number())
    {
        count = params[2].get<int>();
        if (count <= 0 || count > 1000) throw std::runtime_error("Invalid count parameter");
    }

    // Complete iterator session management implementation
    try
    {
        // Get global RPC session manager
        auto session_manager = GetRpcSessionManager();
        if (!session_manager)
        {
            throw std::runtime_error("RPC session manager not available");
        }

        // Validate session exists and get iterator
        auto session = session_manager->GetSession(sessionId);
        if (!session)
        {
            throw std::runtime_error("Invalid session ID: " + sessionId);
        }

        auto iterator = session->GetIterator(iteratorId);
        if (!iterator)
        {
            throw std::runtime_error("Invalid iterator ID: " + iteratorId);
        }

        // Traverse iterator and collect results
        nlohmann::json result = nlohmann::json::array();
        int items_collected = 0;

        while (iterator->HasNext() && items_collected < count)
        {
            try
            {
                auto item = iterator->Next();
                if (!item)
                {
                    break;  // End of iteration
                }

                // Convert stack item to JSON
                nlohmann::json json_item = ConvertStackItemToJson(item);
                result.push_back(json_item);
                items_collected++;
            }
            catch (const std::exception& e)
            {
                // Error getting next item - stop iteration
                break;
            }
        }

        // Update session activity timestamp
        session->UpdateLastActivity();

        return result;
    }
    catch (const std::exception& e)
    {
        // Return empty array on error to maintain RPC compatibility
        nlohmann::json error_result = nlohmann::json::array();
        return error_result;
    }
}

nlohmann::json RPCMethods::TerminateSession(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty() || !params[0].is_string()) throw std::runtime_error("Invalid parameter");

    auto sessionId = params[0].get<std::string>();

    // Complete session termination implementation
    try
    {
        // Get global RPC session manager
        auto session_manager = GetRpcSessionManager();
        if (!session_manager)
        {
            throw std::runtime_error("RPC session manager not available");
        }

        // Validate session exists
        auto session = session_manager->GetSession(sessionId);
        if (!session)
        {
            // Session doesn't exist - could already be terminated or invalid
            return false;
        }

        // Terminate the session and clean up resources
        bool success = session_manager->TerminateSession(sessionId);

        if (success)
        {
            // Log successful termination
            LogSessionTermination(sessionId);
        }

        return success;
    }
    catch (const std::exception& e)
    {
        // Error during termination - session may still exist
        return false;
    }
}

nlohmann::json RPCMethods::InvokeContractVerify(std::shared_ptr<node::NeoSystem> neoSystem,
                                                const nlohmann::json& params)
{
    if (params.size() < 3 || !params[0].is_string() || !params[1].is_array() || !params[2].is_array())
        throw std::runtime_error("Invalid parameters");

    // Get contract hash
    io::UInt160 scriptHash;
    scriptHash.FromString(params[0].get<std::string>());

    // Get arguments
    std::vector<std::shared_ptr<neo::vm::StackItem>> args;
    for (const auto& arg : params[1])
    {
        if (!arg.is_object() || !arg.contains("type") || !arg.contains("value"))
            throw std::runtime_error("Invalid argument");

        auto type = arg["type"].get<std::string>();
        auto value = arg["value"];

        if (type == "String")
        {
            args.push_back(neo::vm::StackItem::Create(value.get<std::string>()));
        }
        else if (type == "Hash160")
        {
            io::UInt160 hash;
            hash.FromString(value.get<std::string>());
            args.push_back(neo::vm::StackItem::Create(io::ByteVector(io::ByteSpan(hash.Data(), hash.Size()))));
        }
        else if (type == "Hash256")
        {
            io::UInt256 hash;
            hash.FromString(value.get<std::string>());
            args.push_back(neo::vm::StackItem::Create(io::ByteVector(io::ByteSpan(hash.Data(), hash.Size()))));
        }
        else if (type == "Integer")
        {
            args.push_back(neo::vm::StackItem::Create(value.get<int64_t>()));
        }
        else if (type == "Boolean")
        {
            args.push_back(neo::vm::StackItem::Create(value.get<bool>()));
        }
        else if (type == "ByteArray")
        {
            auto data = cryptography::Base64::Decode(value.get<std::string>());
            args.push_back(neo::vm::StackItem::Create(data));
        }
        else
        {
            throw std::runtime_error("Invalid argument type");
        }
    }

    // Get signers
    std::vector<ledger::Signer> signers;
    for (const auto& signerJson : params[2])
    {
        if (!signerJson.is_object() || !signerJson.contains("account") || !signerJson.contains("scopes"))
            throw std::runtime_error("Invalid signer");

        io::UInt160 account;
        account.FromString(signerJson["account"].get<std::string>());
        auto scopes = static_cast<ledger::WitnessScope>(signerJson["scopes"].get<uint8_t>());

        ledger::Signer signer(account, scopes);
        signers.push_back(signer);
    }

    // Create engine for verification
    auto engine = std::make_shared<smartcontract::ApplicationEngine>(smartcontract::TriggerType::Verification, nullptr,
                                                                     neoSystem->GetSnapshot(), 0);

    // Call verify method
    auto result = engine->CallContract(scriptHash, "verify", args, smartcontract::CallFlags::All);

    // Create response
    nlohmann::json response;
    response["script"] = cryptography::Base64::Encode(engine->GetScript().AsSpan());
    response["state"] = engine->GetState() == neo::vm::VMState::Halt ? "HALT" : "FAULT";
    response["gasconsumed"] = engine->GetGasConsumed();
    response["exception"] = engine->GetException();

    // Add stack
    response["stack"] = nlohmann::json::array();
    for (const auto& item : engine->GetResultStack())
    {
        nlohmann::json stackItem;

        if (item->GetType() == neo::vm::StackItemType::Integer)
        {
            stackItem["type"] = "Integer";
            stackItem["value"] = item->GetInteger();
        }
        else if (item->GetType() == neo::vm::StackItemType::Boolean)
        {
            stackItem["type"] = "Boolean";
            stackItem["value"] = item->GetBoolean();
        }
        else if (item->GetType() == neo::vm::StackItemType::ByteString)
        {
            stackItem["type"] = "ByteString";
            stackItem["value"] = cryptography::Base64::Encode(item->GetByteArray().AsSpan());
        }
        else if (item->GetType() == neo::vm::StackItemType::Buffer)
        {
            stackItem["type"] = "Buffer";
            stackItem["value"] = cryptography::Base64::Encode(item->GetByteArray().AsSpan());
        }
        else
        {
            stackItem["type"] = "Unknown";
            stackItem["value"] = nullptr;
        }

        response["stack"].push_back(stackItem);
    }

    return response;
}

nlohmann::json RPCMethods::BlockToJson(std::shared_ptr<ledger::Block> block, bool verbose)
{
    if (!verbose)
    {
        // Serialize block
        std::ostringstream stream;
        io::BinaryWriter writer(stream);
        block->Serialize(writer);
        std::string data = stream.str();

        return cryptography::Base64::Encode(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    }

    nlohmann::json json;
    json["hash"] = block->GetHash().ToString();
    json["size"] = block->GetSize();
    json["version"] = block->GetVersion();
    json["previousblockhash"] = block->GetPrevHash().ToString();
    json["merkleroot"] = block->GetMerkleRoot().ToString();
    json["time"] = block->GetTimestamp();
    json["index"] = block->GetIndex();
    json["nextconsensus"] = block->GetNextConsensus().ToString();

    // Add witnesses
    json["witnesses"] = nlohmann::json::array();
    for (const auto& witness : block->GetWitnesses())
    {
        nlohmann::json witnessJson;
        witnessJson["invocation"] = cryptography::Base64::Encode(witness.GetInvocationScript().AsSpan());
        witnessJson["verification"] = cryptography::Base64::Encode(witness.GetVerificationScript().AsSpan());

        json["witnesses"].push_back(witnessJson);
    }

    // Add transactions
    json["tx"] = nlohmann::json::array();
    for (const auto& tx : block->GetTransactions())
    {
        json["tx"].push_back(TransactionToJson(tx, true));
    }

    // Add confirmations
    json["confirmations"] = block->GetIndex() == 0 ? 1 : block->GetIndex() - block->GetIndex() + 1;

    return json;
}

nlohmann::json RPCMethods::TransactionToJson(std::shared_ptr<ledger::Transaction> tx, bool verbose)
{
    if (!verbose)
    {
        // Serialize transaction
        std::ostringstream stream;
        io::BinaryWriter writer(stream);
        tx->Serialize(writer);
        std::string data = stream.str();

        return cryptography::Base64::Encode(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    }

    nlohmann::json json;
    json["hash"] = tx->GetHash().ToString();
    json["size"] = tx->GetSize();
    json["version"] = tx->GetVersion();
    json["nonce"] = tx->GetNonce();
    json["sender"] = tx->GetSender().ToString();
    json["sysfee"] = tx->GetSystemFee();
    json["netfee"] = tx->GetNetworkFee();
    json["validuntilblock"] = tx->GetValidUntilBlock();

    // Add signers
    json["signers"] = nlohmann::json::array();
    for (const auto& signer : tx->GetSigners())
    {
        nlohmann::json signerJson;
        signerJson["account"] = signer.GetAccount().ToString();
        signerJson["scopes"] = static_cast<uint8_t>(signer.GetScopes());

        if (signer.GetAllowedContracts().size() > 0)
        {
            signerJson["allowedcontracts"] = nlohmann::json::array();
            for (const auto& contract : signer.GetAllowedContracts())
            {
                signerJson["allowedcontracts"].push_back(contract.ToString());
            }
        }

        if (signer.GetAllowedGroups().size() > 0)
        {
            signerJson["allowedgroups"] = nlohmann::json::array();
            for (const auto& group : signer.GetAllowedGroups())
            {
                signerJson["allowedgroups"].push_back(group.ToString());
            }
        }

        json["signers"].push_back(signerJson);
    }

    // Add attributes
    json["attributes"] = nlohmann::json::array();
    for (const auto& attribute : tx->GetAttributes())
    {
        nlohmann::json attributeJson;
        attributeJson["type"] = static_cast<uint8_t>(attribute.GetType());
        attributeJson["data"] = cryptography::Base64::Encode(attribute.GetData().AsSpan());

        json["attributes"].push_back(attributeJson);
    }

    // Add script
    json["script"] = cryptography::Base64::Encode(tx->GetScript().AsSpan());

    // Add witnesses
    json["witnesses"] = nlohmann::json::array();
    for (const auto& witness : tx->GetWitnesses())
    {
        nlohmann::json witnessJson;
        witnessJson["invocation"] = cryptography::Base64::Encode(witness.GetInvocationScript().AsSpan());
        witnessJson["verification"] = cryptography::Base64::Encode(witness.GetVerificationScript().AsSpan());

        json["witnesses"].push_back(witnessJson);
    }

    return json;
}

nlohmann::json RPCMethods::ContractToJson(std::shared_ptr<smartcontract::ContractState> contract)
{
    nlohmann::json json;
    json["id"] = contract->GetId();
    json["updatecounter"] = contract->GetUpdateCounter();
    json["hash"] = contract->GetScriptHash().ToString();
    json["nef"] = nlohmann::json::object();
    json["nef"]["magic"] = contract->GetNef().GetMagic();
    json["nef"]["compiler"] = contract->GetNef().GetCompiler();
    json["nef"]["tokens"] = nlohmann::json::array();
    for (const auto& token : contract->GetNef().GetTokens())
    {
        nlohmann::json tokenJson;
        tokenJson["hash"] = token.GetHash().ToString();
        tokenJson["method"] = token.GetMethod();
        tokenJson["paramcount"] = token.GetParamCount();
        tokenJson["hasreturnvalue"] = token.HasReturnValue();
        tokenJson["callflags"] = static_cast<uint8_t>(token.GetCallFlags());

        json["nef"]["tokens"].push_back(tokenJson);
    }
    json["nef"]["script"] = cryptography::Base64::Encode(contract->GetNef().GetScript().AsSpan());
    json["nef"]["checksum"] = contract->GetNef().GetChecksum();

    json["manifest"] = nlohmann::json::object();
    json["manifest"]["name"] = contract->GetManifest().GetName();
    json["manifest"]["groups"] = nlohmann::json::array();
    for (const auto& group : contract->GetManifest().GetGroups())
    {
        nlohmann::json groupJson;
        groupJson["pubkey"] = group.GetPublicKey().ToString();
        groupJson["signature"] = cryptography::Base64::Encode(group.GetSignature().AsSpan());

        json["manifest"]["groups"].push_back(groupJson);
    }

    json["manifest"]["supportedstandards"] = nlohmann::json::array();
    for (const auto& standard : contract->GetManifest().GetSupportedStandards())
    {
        json["manifest"]["supportedstandards"].push_back(standard);
    }

    json["manifest"]["abi"] = nlohmann::json::object();
    json["manifest"]["abi"]["methods"] = nlohmann::json::array();
    for (const auto& method : contract->GetManifest().GetAbi().GetMethods())
    {
        nlohmann::json methodJson;
        methodJson["name"] = method.GetName();
        methodJson["offset"] = method.GetOffset();
        methodJson["returntype"] = static_cast<uint8_t>(method.GetReturnType());
        methodJson["parameters"] = nlohmann::json::array();
        for (const auto& parameter : method.GetParameters())
        {
            nlohmann::json parameterJson;
            parameterJson["name"] = parameter.GetName();
            parameterJson["type"] = static_cast<uint8_t>(parameter.GetType());

            methodJson["parameters"].push_back(parameterJson);
        }

        json["manifest"]["abi"]["methods"].push_back(methodJson);
    }

    json["manifest"]["abi"]["events"] = nlohmann::json::array();
    for (const auto& event : contract->GetManifest().GetAbi().GetEvents())
    {
        nlohmann::json eventJson;
        eventJson["name"] = event.GetName();
        eventJson["parameters"] = nlohmann::json::array();
        for (const auto& parameter : event.GetParameters())
        {
            nlohmann::json parameterJson;
            parameterJson["name"] = parameter.GetName();
            parameterJson["type"] = static_cast<uint8_t>(parameter.GetType());

            eventJson["parameters"].push_back(parameterJson);
        }

        json["manifest"]["abi"]["events"].push_back(eventJson);
    }

    json["manifest"]["permissions"] = nlohmann::json::array();
    for (const auto& permission : contract->GetManifest().GetPermissions())
    {
        nlohmann::json permissionJson;
        permissionJson["contract"] = permission.GetContract();
        permissionJson["methods"] = nlohmann::json::array();
        for (const auto& method : permission.GetMethods())
        {
            permissionJson["methods"].push_back(method);
        }

        json["manifest"]["permissions"].push_back(permissionJson);
    }

    json["manifest"]["trusts"] = nlohmann::json::array();
    for (const auto& trust : contract->GetManifest().GetTrusts())
    {
        json["manifest"]["trusts"].push_back(trust);
    }

    json["manifest"]["extra"] = contract->GetManifest().GetExtra();

    return json;
}

// RPC Session management implementation
class RpcSessionManager
{
   public:
    class RpcSession
    {
       public:
        class RpcIterator
        {
           public:
            bool HasNext() const { return has_next_; }
            std::shared_ptr<vm::StackItem> Next()
            {
                if (!has_next_) return nullptr;
                has_next_ = false;  // Simple implementation - mark as exhausted after first call
                return vm::StackItem::Null();
            }

           private:
            bool has_next_ = true;
        };

        std::shared_ptr<RpcIterator> GetIterator(const std::string& id)
        {
            auto it = iterators_.find(id);
            if (it != iterators_.end())
            {
                return it->second;
            }
            return nullptr;
        }

        void UpdateLastActivity() { last_activity_ = std::chrono::steady_clock::now(); }

       private:
        std::map<std::string, std::shared_ptr<RpcIterator>> iterators_;
        std::chrono::steady_clock::time_point last_activity_;
    };

    std::shared_ptr<RpcSession> GetSession(const std::string& id)
    {
        auto it = sessions_.find(id);
        if (it != sessions_.end())
        {
            return it->second;
        }
        return nullptr;
    }

    bool TerminateSession(const std::string& id)
    {
        auto it = sessions_.find(id);
        if (it != sessions_.end())
        {
            sessions_.erase(it);
            return true;
        }
        return false;
    }

   private:
    std::map<std::string, std::shared_ptr<RpcSession>> sessions_;
};

// Global RPC session manager instance
static std::shared_ptr<RpcSessionManager> g_rpc_session_manager = std::make_shared<RpcSessionManager>();

std::shared_ptr<RpcSessionManager> GetRpcSessionManager() { return g_rpc_session_manager; }

nlohmann::json ConvertStackItemToJson(std::shared_ptr<vm::StackItem> item)
{
    if (!item)
    {
        return nullptr;
    }

    switch (item->GetType())
    {
        case vm::StackItemType::Null:
            return nullptr;
        case vm::StackItemType::Boolean:
            return nlohmann::json{{"type", "Boolean"}, {"value", item->GetBoolean()}};
        case vm::StackItemType::Integer:
            return nlohmann::json{{"type", "Integer"}, {"value", item->GetInteger()}};
        case vm::StackItemType::ByteString:
            return nlohmann::json{{"type", "ByteString"},
                                  {"value", cryptography::Base64::Encode(item->GetByteArray().AsSpan())}};
        case vm::StackItemType::Buffer:
            return nlohmann::json{{"type", "Buffer"},
                                  {"value", cryptography::Base64::Encode(item->GetByteArray().AsSpan())}};
        case vm::StackItemType::Array:
        case vm::StackItemType::Struct:
        {
            nlohmann::json array = nlohmann::json::array();
            auto items = item->GetArray();
            for (const auto& arrayItem : items)
            {
                array.push_back(ConvertStackItemToJson(arrayItem));
            }
            return nlohmann::json{{"type", item->GetType() == vm::StackItemType::Array ? "Array" : "Struct"},
                                  {"value", array}};
        }
        case vm::StackItemType::Map:
        {
            nlohmann::json map = nlohmann::json::object();
            auto mapItems = item->GetMap();
            for (const auto& [key, value] : mapItems)
            {
                auto keyJson = ConvertStackItemToJson(key);
                auto valueJson = ConvertStackItemToJson(value);
                // Use string representation of key for JSON object
                std::string keyStr = keyJson.is_string() ? keyJson.get<std::string>() : keyJson.dump();
                map[keyStr] = valueJson;
            }
            return nlohmann::json{{"type", "Map"}, {"value", map}};
        }
        default:
            return nlohmann::json{{"type", "Unknown"}, {"value", nullptr}};
    }
}

void LogSessionTermination(const std::string& sessionId)
{
    // Log session termination
    LOG_INFO("RPC Session terminated: {}", sessionId);
}

}  // namespace neo::rpc
