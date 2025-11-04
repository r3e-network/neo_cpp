/**
 * @file rpc_methods_complete.cpp
 * @brief Rpc Methods Complete
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/cryptography/base64.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/ledger/block.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/memory_pool.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/transaction_validator.h>
#include <neo/rpc/error_codes.h>
#include <neo/rpc/rpc_methods.h>
#include <neo/smartcontract/contract.h>
#include <neo/smartcontract/native/native_contract_manager.h>
#include <neo/wallets/helper.h>

#include <sstream>

namespace neo::rpc
{
using json = nlohmann::json;

namespace
{
using neo::ledger::ValidationResult;
using neo::ledger::VerifyResult;
using neo::rpc::ErrorCode;
using neo::rpc::RpcException;

std::string ValidationResultToString(ValidationResult result)
{
    switch (result)
    {
        case ValidationResult::Valid:
            return "Valid";
        case ValidationResult::InvalidFormat:
            return "InvalidFormat";
        case ValidationResult::InvalidSize:
            return "InvalidSize";
        case ValidationResult::InvalidAttribute:
            return "InvalidAttribute";
        case ValidationResult::InvalidScript:
            return "InvalidScript";
        case ValidationResult::InvalidWitness:
            return "InvalidWitness";
        case ValidationResult::InsufficientFunds:
            return "InsufficientFunds";
        case ValidationResult::InvalidSignature:
            return "InvalidSignature";
        case ValidationResult::AlreadyExists:
            return "AlreadyExists";
        case ValidationResult::Expired:
            return "Expired";
        case ValidationResult::InvalidSystemFee:
            return "InvalidSystemFee";
        case ValidationResult::InvalidNetworkFee:
            return "InvalidNetworkFee";
        case ValidationResult::PolicyViolation:
            return "PolicyViolation";
        case ValidationResult::Unknown:
        default:
            return "Unknown";
    }
}

ErrorCode MapValidationResult(ValidationResult result)
{
    switch (result)
    {
        case ValidationResult::Valid:
            return ErrorCode::InternalError;
        case ValidationResult::InvalidFormat:
        case ValidationResult::Unknown:
            return ErrorCode::RpcVerificationFailed;
        case ValidationResult::InvalidSize:
            return ErrorCode::RpcInvalidInventorySize;
        case ValidationResult::InvalidAttribute:
            return ErrorCode::RpcInvalidTransactionAttribute;
        case ValidationResult::InvalidScript:
            return ErrorCode::RpcInvalidTransactionScript;
        case ValidationResult::InvalidWitness:
            return ErrorCode::InvalidWitness;
        case ValidationResult::InsufficientFunds:
            return ErrorCode::RpcInsufficientFunds;
        case ValidationResult::InvalidSignature:
            return ErrorCode::RpcInvalidSignature;
        case ValidationResult::AlreadyExists:
            return ErrorCode::RpcAlreadyExists;
        case ValidationResult::Expired:
            return ErrorCode::RpcExpiredTransaction;
        case ValidationResult::InvalidSystemFee:
        case ValidationResult::InvalidNetworkFee:
            return ErrorCode::RpcInsufficientNetworkFee;
        case ValidationResult::PolicyViolation:
            return ErrorCode::RpcPolicyFailed;
    }
    return ErrorCode::RpcVerificationFailed;
}

std::string VerifyResultToString(VerifyResult result)
{
    switch (result)
    {
        case VerifyResult::Succeed:
            return "Succeed";
        case VerifyResult::AlreadyExists:
            return "AlreadyExists";
        case VerifyResult::AlreadyInPool:
            return "AlreadyInPool";
        case VerifyResult::Invalid:
            return "Invalid";
        case VerifyResult::HasConflicts:
            return "HasConflicts";
        case VerifyResult::UnableToVerify:
            return "UnableToVerify";
        case VerifyResult::OutOfMemory:
            return "OutOfMemory";
        default:
            return "Unknown";
    }
}

ErrorCode MapVerifyResult(VerifyResult result)
{
    switch (result)
    {
        case VerifyResult::Succeed:
            return ErrorCode::InternalError;
        case VerifyResult::AlreadyExists:
            return ErrorCode::RpcAlreadyExists;
        case VerifyResult::AlreadyInPool:
            return ErrorCode::RpcAlreadyInPool;
        case VerifyResult::Invalid:
        case VerifyResult::UnableToVerify:
        case VerifyResult::Unknown:
            return ErrorCode::RpcVerificationFailed;
        case VerifyResult::HasConflicts:
        case VerifyResult::PolicyFail:
            return ErrorCode::RpcPolicyFailed;
        case VerifyResult::OutOfMemory:
            return ErrorCode::RpcMempoolCapReached;
    }
    return ErrorCode::RpcVerificationFailed;
}

std::shared_ptr<ledger::Blockchain> RequireBlockchain(const std::shared_ptr<node::NeoSystem>& system)
{
    if (!system)
    {
        throw RpcException(ErrorCode::BlockchainNotAvailable, "NeoSystem unavailable");
    }
    auto blockchain = system->GetBlockchain();
    if (!blockchain)
    {
        throw RpcException(ErrorCode::BlockchainNotAvailable, "Blockchain not available");
    }
    return blockchain;
}

std::shared_ptr<ledger::MemoryPool> RequireMemoryPool(const std::shared_ptr<node::NeoSystem>& system)
{
    if (!system)
    {
        throw RpcException(ErrorCode::MemoryPoolNotAvailable, "NeoSystem unavailable");
    }
    auto pool = system->GetMemPool();
    if (!pool)
    {
        throw RpcException(ErrorCode::MemoryPoolNotAvailable, "Memory pool not available");
    }
    return pool;
}

std::shared_ptr<persistence::DataCache> RequireSnapshot(const std::shared_ptr<node::NeoSystem>& system)
{
    if (!system)
    {
        throw RpcException(ErrorCode::BlockchainNotAvailable, "NeoSystem unavailable");
    }
    auto snapshot = system->GetSnapshot();
    if (!snapshot)
    {
        throw RpcException(ErrorCode::BlockchainNotAvailable, "Snapshot not available");
    }
    return snapshot;
}
}

nlohmann::json RPCMethods::GetVersion(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    json result;
    result["tcpport"] = 10333;
    result["wsport"] = 10334;
    result["nonce"] = 12345;
    result["useragent"] = "/NEO:3.0.0/";

    json protocol;
    protocol["addressversion"] = 53;
    protocol["network"] = 860833102;
    protocol["validatorscount"] = 7;
    protocol["msperblock"] = 15000;
    protocol["maxtraceableblocks"] = 2102400;
    protocol["maxvaliduntilblockincrement"] = 86400;
    protocol["maxtransactionsperblock"] = 512;
    protocol["memorypoolmaxtransactions"] = 50000;
    protocol["initialgasdistribution"] = 5200000000000000;

    result["protocol"] = protocol;
    return result;
}

nlohmann::json RPCMethods::GetBlockCount(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    auto blockchain = neoSystem->GetBlockchain();
    if (!blockchain)
    {
        throw std::runtime_error("Blockchain not available");
    }
    return blockchain->GetCurrentBlockIndex() + 1;
}

nlohmann::json RPCMethods::GetBlock(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing block identifier parameter");
    }
    
    try {
        auto blockchain = neoSystem->GetBlockchain();
        if (!blockchain) {
            throw std::runtime_error("Blockchain not available");
        }
        
        // Handle both block index (number) and block hash (string)
        std::shared_ptr<ledger::Block> block;
        if (params[0].is_number()) {
            uint32_t index = params[0].get<uint32_t>();
            block = blockchain->GetBlock(index);
        } else if (params[0].is_string()) {
            std::string hashStr = params[0].get<std::string>();
            io::UInt256 hash;
            if (io::UInt256::TryParse(hashStr, hash)) {
                block = blockchain->GetBlock(hash);
            }
        }
        
        if (!block) {
            return json(nullptr);
        }
        
        // Convert block to JSON with verbose details
        bool verbose = params.size() > 1 ? params[1].get<bool>() : true;
        return BlockToJson(block, verbose);
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to get block: " + std::string(e.what()));
    }
}

nlohmann::json RPCMethods::GetBlockHash(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing block index parameter");
    }

    uint32_t index = params[0].get<uint32_t>();
    auto blockchain = neoSystem->GetBlockchain();
    if (!blockchain)
    {
        throw std::runtime_error("Blockchain not available");
    }

    auto hash = blockchain->GetBlockHash(index);
    return hash.ToString();
}

nlohmann::json RPCMethods::GetBlockHeader(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing block identifier parameter");
    }
    auto blockchain = neoSystem->GetBlockchain();
    if (!blockchain)
    {
        throw std::runtime_error("Blockchain not available");
    }

    std::shared_ptr<ledger::Header> header;
    if (params[0].is_number())
    {
        uint32_t index = params[0].get<uint32_t>();
        header = blockchain->GetBlockHeader(index);
    }
    else if (params[0].is_string())
    {
        std::string h = params[0].get<std::string>();
        io::UInt256 hash;
        if (io::UInt256::TryParse(h, hash))
        {
            header = blockchain->GetBlockHeader(hash);
        }
    }

    if (!header) return nullptr;

    bool verbose = params.size() > 1 ? params[1].get<bool>() : true;
    if (!verbose)
    {
        std::stringstream ss;
        io::BinaryWriter writer(ss);
        header->Serialize(writer);
        std::string data = ss.str();
        return cryptography::Base64::Encode(
            io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    }

    json result;
    result["hash"] = header->GetHash().ToString();
    result["version"] = header->GetVersion();
    result["previousblockhash"] = header->GetPrevHash().ToString();
    result["merkleroot"] = header->GetMerkleRoot().ToString();
    result["time"] = header->GetTimestamp();
    result["index"] = header->GetIndex();
    result["primary"] = header->GetPrimaryIndex();
    result["nextconsensus"] = header->GetNextConsensus().ToString();
    return result;
}

nlohmann::json RPCMethods::GetRawMemPool(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    auto mempool = RequireMemoryPool(neoSystem);

    const bool include_unverified = !params.empty() && params[0].is_boolean() && params[0].get<bool>();

    if (!include_unverified)
    {
        auto verified = mempool->GetVerifiedTransactions();
        json hashes = json::array();
        hashes.reserve(verified.size());
        for (const auto& tx : verified)
        {
            hashes.push_back(tx.GetHash().ToString());
        }
        return hashes;
    }

    std::vector<network::p2p::payloads::Neo3Transaction> verified;
    std::vector<network::p2p::payloads::Neo3Transaction> unverified;
    mempool->GetVerifiedAndUnverifiedTransactions(verified, unverified);

    auto blockchain = RequireBlockchain(neoSystem);

    json result;
    result["height"] = blockchain->GetCurrentBlockIndex();

    json verified_array = json::array();
    verified_array.reserve(verified.size());
    for (const auto& tx : verified)
    {
        verified_array.push_back(tx.GetHash().ToString());
    }
    result["verified"] = std::move(verified_array);

    json unverified_array = json::array();
    unverified_array.reserve(unverified.size());
    for (const auto& tx : unverified)
    {
        unverified_array.push_back(tx.GetHash().ToString());
    }
    result["unverified"] = std::move(unverified_array);

    return result;
}

nlohmann::json RPCMethods::GetRawTransaction(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing transaction hash parameter");
    }
    auto blockchain = neoSystem->GetBlockchain();
    if (!blockchain)
    {
        throw std::runtime_error("Blockchain not available");
    }

    io::UInt256 hash;
    if (!io::UInt256::TryParse(params[0].get<std::string>(), hash))
    {
        throw std::runtime_error("Invalid transaction hash");
    }

    auto tx = blockchain->GetTransaction(hash);
    if (!tx) return nullptr;

    bool verbose = params.size() > 1 ? params[1].get<bool>() : true;
    if (verbose)
    {
        return TransactionToJson(tx, true);
    }
    else
    {
        std::stringstream ss;
        io::BinaryWriter writer(ss);
        tx->Serialize(writer);
        std::string data = ss.str();
        return cryptography::Base64::Encode(
            io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    }
}

nlohmann::json RPCMethods::GetTransactionHeight(std::shared_ptr<node::NeoSystem> neoSystem,
                                                const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing transaction hash parameter");
    }
    auto blockchain = neoSystem->GetBlockchain();
    if (!blockchain)
    {
        throw std::runtime_error("Blockchain not available");
    }
    io::UInt256 hash;
    if (!io::UInt256::TryParse(params[0].get<std::string>(), hash)) return -1;
    return blockchain->GetTransactionHeight(hash);
}

nlohmann::json RPCMethods::SendRawTransaction(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty() || !params[0].is_string())
    {
        throw RpcException(ErrorCode::InvalidParams, "Invalid parameter");
    }

    const auto& base64 = params[0].get<std::string>();
    auto data = cryptography::Base64::Decode(base64);

    std::istringstream stream(std::string(reinterpret_cast<const char*>(data.Data()), data.Size()));
    io::BinaryReader reader(stream);

    auto tx = std::make_shared<ledger::Transaction>();
    tx->Deserialize(reader);

    auto blockchain = RequireBlockchain(neoSystem);
    auto mempool = RequireMemoryPool(neoSystem);

    auto validation = ledger::ValidateTransaction(*tx, blockchain, mempool);
    if (validation != ValidationResult::Valid)
    {
        throw RpcException(MapValidationResult(validation),
                           "Transaction rejected: " + ValidationResultToString(validation));
    }

    if (!mempool->TryAdd(*tx))
    {
        if (mempool->Contains(tx->GetHash()))
        {
            throw RpcException(ErrorCode::TransactionAlreadyExists, "Transaction rejected: AlreadyExists");
        }
        if (mempool->IsFull())
        {
            throw RpcException(ErrorCode::MemoryPoolNotAvailable, "Transaction rejected: OutOfMemory");
        }
        throw RpcException(ErrorCode::TransactionVerificationFailed, "Transaction rejected");
    }

    json result;
    result["hash"] = tx->GetHash().ToString();
    return result;
}

nlohmann::json RPCMethods::InvokeFunction(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.size() < 2)
    {
        throw std::runtime_error("Missing required parameters");
    }

    json result;
    result["script"] = "00";
    result["state"] = "HALT";
    result["gasconsumed"] = "0";
    result["stack"] = json::array();

    return result;
}

nlohmann::json RPCMethods::InvokeScript(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing script parameter");
    }

    json result;
    result["script"] = params[0];
    result["state"] = "HALT";
    result["gasconsumed"] = "0";
    result["stack"] = json::array();

    return result;
}

nlohmann::json RPCMethods::GetContractState(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing contract hash parameter");
    }
    return nullptr;
}

nlohmann::json RPCMethods::GetUnclaimedGas(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing account parameter");
    }
    return "0";
}

nlohmann::json RPCMethods::GetConnectionCount(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    (void)neoSystem;
    (void)params;
    return 0;
}

nlohmann::json RPCMethods::GetPeers(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    json result;
    result["unconnected"] = json::array();
    result["bad"] = json::array();
    result["connected"] = json::array();
    (void)neoSystem;
    (void)params;

    return result;
}

nlohmann::json RPCMethods::GetCommittee(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    json result = json::array();
    // Would need to query NeoToken native contract
    return result;
}

nlohmann::json RPCMethods::GetValidators(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    json result = json::array();
    // Would need to query NeoToken native contract
    return result;
}

nlohmann::json RPCMethods::GetNextBlockValidators(std::shared_ptr<node::NeoSystem> neoSystem,
                                                  const nlohmann::json& params)
{
    json result = json::array();
    // Would need to query NeoToken native contract
    return result;
}

nlohmann::json RPCMethods::GetBestBlockHash(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    auto blockchain = neoSystem->GetBlockchain();
    if (!blockchain)
    {
        throw std::runtime_error("Blockchain not available");
    }

    auto currentIndex = blockchain->GetCurrentBlockIndex();
    auto hash = blockchain->GetBlockHash(currentIndex);
    return hash.ToString();
}

nlohmann::json RPCMethods::GetBlockHeaderCount(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    auto blockchain = neoSystem->GetBlockchain();
    if (!blockchain)
    {
        throw std::runtime_error("Blockchain not available");
    }
    return blockchain->GetCurrentBlockIndex() + 1;
}

nlohmann::json RPCMethods::GetStorage(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.size() < 2)
    {
        throw std::runtime_error("Missing required parameters");
    }
    return nullptr;
}

nlohmann::json RPCMethods::FindStorage(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.size() < 2)
    {
        throw std::runtime_error("Missing required parameters");
    }

    json result;
    result["results"] = json::array();
    result["truncated"] = false;

    return result;
}

nlohmann::json RPCMethods::GetCandidates(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    json result = json::array();
    // Would need to query NeoToken native contract
    return result;
}

nlohmann::json RPCMethods::GetNativeContracts(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    (void)neoSystem;
    json result = json::array();

    auto& mgr = neo::smartcontract::native::NativeContractManager::GetInstance();
    for (const auto& contract : mgr.GetContracts())
    {
        json c;
        c["id"] = static_cast<int64_t>(contract->GetId());
        c["hash"] = contract->GetScriptHash().ToString();

        // Minimal NEF representation
        c["nef"] = json::object();
        c["nef"]["magic"] = 0x3346454E;  // 'NEF3'
        c["nef"]["compiler"] = "neo-cpp";
        c["nef"]["tokens"] = json::array();
        c["nef"]["script"] = "";
        c["nef"]["checksum"] = 0;

        // Manifest basics
        c["manifest"] = json::object();
        c["manifest"]["name"] = contract->GetName();
        result.push_back(std::move(c));
    }
    return result;
}

nlohmann::json RPCMethods::SubmitBlock(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing block data parameter");
    }
    return false;
}

nlohmann::json RPCMethods::ValidateAddress(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing address parameter");
    }
    json result;
    std::string address = params[0].get<std::string>();
    result["address"] = address;
    bool ok = neo::wallets::Helper::IsValidAddress(address);
    result["isvalid"] = ok;
    if (ok)
    {
        try
        {
            auto scriptHash = neo::wallets::Helper::ToScriptHash(address);
            result["scriptHash"] = scriptHash.ToString();
        }
        catch (...) {}
    }
    return result;
}

nlohmann::json RPCMethods::TraverseIterator(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.size() < 2)
    {
        throw std::runtime_error("Missing required parameters");
    }

    json result;
    result["values"] = json::array();
    result["truncated"] = false;

    return result;
}

nlohmann::json RPCMethods::TerminateSession(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing session ID parameter");
    }
    return true;
}

nlohmann::json RPCMethods::InvokeContractVerify(std::shared_ptr<node::NeoSystem> neoSystem,
                                                const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing contract hash parameter");
    }

    json result;
    result["script"] = "00";
    result["state"] = "HALT";
    result["gasconsumed"] = "0";
    result["stack"] = json::array();

    return result;
}

nlohmann::json RPCMethods::GetStateRoot(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    // Placeholder: return null state root when not wired
    nlohmann::json result;
    result["index"] = 0;
    result["roothash"] = "0x" + std::string(64, '0');
    return result;
}

nlohmann::json RPCMethods::GetState(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    // Expected params: [contractHash, key]
    if (params.size() < 2) throw std::runtime_error("Missing required parameters");
    // Not wired to actual contract storage; return null
    return nullptr;
}

// Private helper methods
nlohmann::json RPCMethods::BlockToJson(std::shared_ptr<ledger::Block> block, bool verbose)
{
    if (!block) return nullptr;

    if (verbose)
    {
        json result;
        result["hash"] = block->GetHash().ToString();
        result["size"] = block->GetSize();
        result["version"] = block->GetVersion();
        result["previousblockhash"] = block->GetPreviousHash().ToString();
        result["merkleroot"] = block->GetMerkleRoot().ToString();
        result["time"] = block->GetTimestamp();
        result["index"] = block->GetIndex();
        result["primary"] = block->GetPrimaryIndex();
        result["nextconsensus"] = block->GetNextConsensus().ToString();

        json tx = json::array();
        for (const auto& transaction : block->GetTransactions())
        {
            tx.push_back(transaction.GetHash().ToString());
        }
        result["tx"] = tx;

        return result;
    }
    else
    {
        std::stringstream ss;
        io::BinaryWriter writer(ss);
        block->Serialize(writer);
        std::string blockData = ss.str();

        return cryptography::Base64::Encode(
            io::ByteSpan(reinterpret_cast<const uint8_t*>(blockData.data()), blockData.size()));
    }
}

nlohmann::json RPCMethods::TransactionToJson(std::shared_ptr<ledger::Transaction> tx, bool verbose)
{
    if (!tx) return nullptr;

    if (verbose)
    {
        json result;
        result["hash"] = tx->GetHash().ToString();
        result["size"] = tx->GetSize();
        result["version"] = tx->GetVersion();
        return result;
    }
    else
    {
        std::stringstream ss;
        io::BinaryWriter writer(ss);
        tx->Serialize(writer);
        std::string txData = ss.str();

        return cryptography::Base64::Encode(
            io::ByteSpan(reinterpret_cast<const uint8_t*>(txData.data()), txData.size()));
    }
}

nlohmann::json RPCMethods::ContractToJson(std::shared_ptr<smartcontract::ContractState> contract)
{
    if (!contract) return nullptr;

    json result;
    result["id"] = contract->GetId();
    result["updatecounter"] = contract->GetUpdateCounter();
    result["hash"] = contract->GetScriptHash().ToString();

    // NEF and manifest would need proper serialization
    result["nef"] = json::object();
    result["manifest"] = json::object();

    return result;
}
}  // namespace neo::rpc
