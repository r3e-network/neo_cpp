#include <neo/rpc/rpc_methods.h>

#include <neo/cryptography/base64.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/io/json_writer.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/memory_pool.h>
#include <neo/ledger/transaction_validator.h>
#include <neo/network/ip_endpoint.h>
#include <neo/network/p2p/local_node.h>
#include <neo/network/p2p/payloads/neo3_transaction.h>
#include <neo/node/neo_system.h>
#include <neo/protocol_settings.h>
#include <neo/rpc/error_codes.h>
#include <neo/rpc/rpc_session_manager.h>
#include <neo/smartcontract/manifest/contract_manifest.h>
#include <neo/smartcontract/native/native_contract_manager.h>
#include <neo/smartcontract/native/contract_management.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/ledger_contract.h>
#include <neo/wallets/helper.h>
#include <neo/plugins/plugin_manager.h>

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <limits>
#include <optional>
#include <random>
#include <sstream>
#include <unordered_set>

namespace neo::rpc
{
namespace
{
using neo::ledger::ValidationResult;
using neo::ledger::VerifyResult;
using neo::ProtocolSettings;

constexpr size_t kFindStoragePageSize = 100;

nlohmann::json SerializeJsonObject(const io::IJsonSerializable& value)
{
    nlohmann::json result = nlohmann::json::object();
    io::JsonWriter writer(result);
    value.SerializeJson(writer);
    return result;
}

std::string FormatScriptHash(const io::UInt160& hash, const std::shared_ptr<ProtocolSettings>& settings)
{
    if (settings)
    {
        return hash.ToAddress(settings->GetAddressVersion());
    }
    std::string value = hash.ToString();
    if (value.rfind("0x", 0) != 0)
    {
        value.insert(0, "0x");
    }
    return value;
}

bool IsValidBase64Char(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' || c == '/' ||
           c == '=';
}

bool IsValidBase64String(const std::string& value)
{
    if (value.empty()) return true;
    if (value.size() % 4 != 0) return false;

    for (char c : value)
    {
        if (!IsValidBase64Char(c))
        {
            return false;
        }
    }

    auto paddingPos = value.find('=');
    if (paddingPos != std::string::npos)
    {
        const size_t paddingCount = value.size() - paddingPos;
        if (paddingCount > 2) return false;
        for (size_t i = paddingPos; i < value.size(); ++i)
        {
            if (value[i] != '=') return false;
        }
    }

    return true;
}

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
        case VerifyResult::OutOfMemory:
            return "OutOfMemory";
        case VerifyResult::UnableToVerify:
            return "UnableToVerify";
        case VerifyResult::Invalid:
            return "Invalid";
        case VerifyResult::InvalidScript:
            return "InvalidScript";
        case VerifyResult::InvalidAttribute:
            return "InvalidAttribute";
        case VerifyResult::InvalidSignature:
            return "InvalidSignature";
        case VerifyResult::OverSize:
            return "OverSize";
        case VerifyResult::Expired:
            return "Expired";
        case VerifyResult::InsufficientFunds:
            return "InsufficientFunds";
        case VerifyResult::PolicyFail:
            return "PolicyFail";
        case VerifyResult::HasConflicts:
            return "HasConflicts";
        case VerifyResult::Unknown:
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
        case VerifyResult::OutOfMemory:
            return ErrorCode::RpcMempoolCapReached;
        case VerifyResult::UnableToVerify:
        case VerifyResult::Invalid:
        case VerifyResult::Unknown:
            return ErrorCode::RpcVerificationFailed;
        case VerifyResult::InvalidScript:
            return ErrorCode::RpcInvalidTransactionScript;
        case VerifyResult::InvalidAttribute:
            return ErrorCode::RpcInvalidTransactionAttribute;
        case VerifyResult::InvalidSignature:
            return ErrorCode::RpcInvalidSignature;
        case VerifyResult::OverSize:
            return ErrorCode::RpcInvalidInventorySize;
        case VerifyResult::Expired:
            return ErrorCode::RpcExpiredTransaction;
        case VerifyResult::InsufficientFunds:
            return ErrorCode::RpcInsufficientFunds;
        case VerifyResult::PolicyFail:
        case VerifyResult::HasConflicts:
            return ErrorCode::RpcPolicyFailed;
    }
    return ErrorCode::RpcVerificationFailed;
}

[[noreturn]] void ThrowNotImplemented(std::string_view method)
{
    throw RpcException(ErrorCode::MethodNotFound, std::string(method) + " not implemented");
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
    auto memoryPool = system ? system->GetMemoryPool() : nullptr;
    if (!memoryPool)
    {
        throw RpcException(ErrorCode::MemoryPoolNotAvailable, "Memory pool not available");
    }
    return memoryPool;
}

std::shared_ptr<persistence::DataCache> RequireSnapshot(const std::shared_ptr<node::NeoSystem>& system)
{
    auto snapshot = system ? system->GetSnapshot() : nullptr;
    if (!snapshot)
    {
        throw RpcException(ErrorCode::InternalError, "Snapshot not available");
    }
    return snapshot;
}

std::shared_ptr<smartcontract::native::ContractManagement> RequireContractManagement()
{
    auto contractManagement = smartcontract::native::ContractManagement::GetInstance();
    if (!contractManagement)
    {
        throw RpcException(ErrorCode::InternalError, "ContractManagement not available");
    }
    return contractManagement;
}

int32_t ResolveContractId(const std::shared_ptr<node::NeoSystem>& system, const nlohmann::json& param)
{
    if (param.is_number_integer())
    {
        auto id = param.get<int64_t>();
        if (id < std::numeric_limits<int32_t>::min() || id > std::numeric_limits<int32_t>::max())
        {
            throw RpcException(ErrorCode::InvalidParams, "Contract id out of range");
        }
        return static_cast<int32_t>(id);
    }

    if (!param.is_string())
    {
        throw RpcException(ErrorCode::InvalidParams, "Invalid contract identifier");
    }

    const auto value = param.get<std::string>();
    auto snapshot = RequireSnapshot(system);
    auto contractManagement = RequireContractManagement();

    io::UInt160 scriptHash;
    bool hashParsed = io::UInt160::TryParse(value, scriptHash);

    if (!hashParsed)
    {
        try
        {
            scriptHash = wallets::Helper::ToScriptHash(value);
            hashParsed = true;
        }
        catch (...)
        {
            hashParsed = false;
        }
    }

    if (hashParsed)
    {
        auto contract =
            contractManagement->GetContract(std::static_pointer_cast<persistence::StoreView>(snapshot), scriptHash);
        if (!contract)
        {
            throw RpcException(ErrorCode::UnknownContract, "Contract not found");
        }
        return contract->GetId();
    }

    auto contracts = contractManagement->ListContracts(std::static_pointer_cast<persistence::StoreView>(snapshot));
    for (const auto& contract : contracts)
    {
        try
        {
            auto manifest = smartcontract::manifest::ContractManifest::Parse(contract->GetManifest());
            if (manifest.GetName() == value)
            {
                return contract->GetId();
            }
        }
        catch (const std::exception&)
        {
            // Ignore manifest parsing errors and continue searching.
        }
    }

    throw RpcException(ErrorCode::UnknownContract, "Contract not found");
}

nlohmann::json SerializeTransaction(const ledger::Transaction& tx, bool verbose,
                                    std::shared_ptr<ProtocolSettings> settings = nullptr,
                                    std::shared_ptr<ledger::Blockchain> blockchain = nullptr,
                                    std::optional<uint32_t> blockIndex = std::nullopt,
                                    std::optional<uint64_t> blockTimestamp = std::nullopt)
{
    if (!verbose)
    {
        io::ByteVector buffer;
        io::BinaryWriter writer(buffer);
        tx.Serialize(writer);
        return cryptography::Base64::Encode(buffer.AsSpan());
    }

    nlohmann::json json = nlohmann::json::object();
    json["hash"] = tx.GetHash().ToString();
    json["size"] = tx.GetSize();
    json["version"] = tx.GetVersion();
    json["nonce"] = tx.GetNonce();
    json["sender"] = FormatScriptHash(tx.GetSender(), settings);
    json["sysfee"] = std::to_string(tx.GetSystemFee());
    json["netfee"] = std::to_string(tx.GetNetworkFee());
    json["validuntilblock"] = tx.GetValidUntilBlock();

    nlohmann::json signers = nlohmann::json::array();
    for (const auto& signer : tx.GetSigners())
    {
        signers.push_back(SerializeJsonObject(signer));
    }
    json["signers"] = std::move(signers);

    nlohmann::json attributes = nlohmann::json::array();
    for (const auto& attribute : tx.GetAttributes())
    {
        if (attribute)
        {
            attributes.push_back(SerializeJsonObject(*attribute));
        }
    }
    json["attributes"] = std::move(attributes);

    json["script"] = cryptography::Base64::Encode(tx.GetScript().AsSpan());

    nlohmann::json witnesses = nlohmann::json::array();
    for (const auto& witness : tx.GetWitnesses())
    {
        witnesses.push_back(SerializeJsonObject(witness));
    }
    json["witnesses"] = std::move(witnesses);

    if (blockchain && blockIndex)
    {
        const auto blockHash = blockchain->GetBlockHash(*blockIndex);
        if (!blockHash.IsZero())
        {
            json["blockhash"] = blockHash.ToString();
        }

        const uint32_t currentHeight = blockchain->GetHeight();
        if (*blockIndex <= currentHeight)
        {
            json["confirmations"] = currentHeight - *blockIndex + 1;
        }
        else
        {
            json["confirmations"] = 0;
        }
    }

    if (blockTimestamp)
    {
        json["blocktime"] = *blockTimestamp;
    }

    return json;
}

nlohmann::json SerializeBlock(const ledger::Block& block, bool verbose,
                              std::shared_ptr<ProtocolSettings> settings = nullptr,
                              std::shared_ptr<ledger::Blockchain> blockchain = nullptr,
                              std::optional<uint32_t> currentHeight = std::nullopt)
{
    if (!verbose)
    {
        ledger::Block blockCopy = block;
        io::ByteVector buffer;
        io::BinaryWriter writer(buffer);
        blockCopy.Serialize(writer);
        return cryptography::Base64::Encode(buffer.AsSpan());
    }

    nlohmann::json json = nlohmann::json::object();
    json["hash"] = block.GetHash().ToString();
    json["size"] = block.GetSize();
    json["version"] = block.GetVersion();
    json["previousblockhash"] = block.GetPreviousHash().ToString();
    json["merkleroot"] = block.GetMerkleRoot().ToString();
    json["time"] = block.GetTimestamp();

    std::ostringstream nonceStream;
    nonceStream << std::uppercase << std::hex << std::setw(16) << std::setfill('0') << block.GetNonce();
    json["nonce"] = nonceStream.str();

    json["index"] = block.GetIndex();
    json["primary"] = block.GetPrimaryIndex();
    json["nextconsensus"] = FormatScriptHash(block.GetNextConsensus(), settings);

    nlohmann::json witnesses = nlohmann::json::array();
    witnesses.push_back(SerializeJsonObject(block.GetWitness()));
    json["witnesses"] = std::move(witnesses);

    nlohmann::json transactions = nlohmann::json::array();
    for (const auto& tx : block.GetTransactions())
    {
        transactions.push_back(
            SerializeTransaction(tx, true, settings, blockchain, block.GetIndex(), block.GetTimestamp()));
    }
    json["tx"] = std::move(transactions);

    uint32_t referenceHeight = currentHeight.value_or(blockchain ? blockchain->GetHeight() : block.GetIndex());
    if (blockchain)
    {
        if (block.GetIndex() <= referenceHeight)
        {
            json["confirmations"] = referenceHeight - block.GetIndex() + 1;
        }
        else
        {
            json["confirmations"] = 0;
        }

        if (block.GetIndex() < std::numeric_limits<uint32_t>::max())
        {
            const auto nextHash = blockchain->GetBlockHash(block.GetIndex() + 1);
            if (!nextHash.IsZero())
            {
                json["nextblockhash"] = nextHash.ToString();
            }
        }
    }
    else if (currentHeight)
    {
        if (block.GetIndex() <= *currentHeight)
        {
            json["confirmations"] = *currentHeight - block.GetIndex() + 1;
        }
        else
        {
            json["confirmations"] = 0;
        }
    }

    return json;
}

nlohmann::json SerializeBlockHeader(const ledger::BlockHeader& header, bool verbose,
                                    std::shared_ptr<ProtocolSettings> settings = nullptr,
                                    std::shared_ptr<ledger::Blockchain> blockchain = nullptr,
                                    std::optional<uint32_t> currentHeight = std::nullopt)
{
    if (!verbose)
    {
        ledger::BlockHeader headerCopy = header;
        io::ByteVector buffer;
        io::BinaryWriter writer(buffer);
        headerCopy.Serialize(writer);
        return cryptography::Base64::Encode(buffer.AsSpan());
    }

    nlohmann::json json = nlohmann::json::object();
    json["hash"] = header.GetHash().ToString();
    json["size"] = header.GetSize();
    json["version"] = header.GetVersion();
    json["previousblockhash"] = header.GetPrevHash().ToString();
    json["merkleroot"] = header.GetMerkleRoot().ToString();
    json["time"] = header.GetTimestamp();

    std::ostringstream nonceStream;
    nonceStream << std::uppercase << std::hex << std::setw(16) << std::setfill('0') << header.GetNonce();
    json["nonce"] = nonceStream.str();

    json["index"] = header.GetIndex();
    json["primary"] = header.GetPrimaryIndex();
    json["nextconsensus"] = FormatScriptHash(header.GetNextConsensus(), settings);

    nlohmann::json witnesses = nlohmann::json::array();
    witnesses.push_back(SerializeJsonObject(header.GetWitness()));
    json["witnesses"] = std::move(witnesses);

    uint32_t referenceHeight = currentHeight.value_or(blockchain ? blockchain->GetHeight() : header.GetIndex());
    if (blockchain)
    {
        if (header.GetIndex() <= referenceHeight)
        {
            json["confirmations"] = referenceHeight - header.GetIndex() + 1;
        }
        else
        {
            json["confirmations"] = 0;
        }

        if (header.GetIndex() < std::numeric_limits<uint32_t>::max())
        {
            const auto nextHash = blockchain->GetBlockHash(header.GetIndex() + 1);
            if (!nextHash.IsZero())
            {
                json["nextblockhash"] = nextHash.ToString();
            }
        }
    }
    else if (currentHeight)
    {
        if (header.GetIndex() <= *currentHeight)
        {
            json["confirmations"] = *currentHeight - header.GetIndex() + 1;
        }
        else
        {
            json["confirmations"] = 0;
        }
    }

    return json;
}

network::p2p::payloads::Neo3Transaction DeserializeNeo3Transaction(const std::string& base64)
{
    const auto data = cryptography::Base64::Decode(base64);
    if (data.empty())
    {
        throw RpcException(ErrorCode::InvalidParams, "Transaction payload is empty");
    }

    network::p2p::payloads::Neo3Transaction tx;
    io::BinaryReader reader(data);
    try
    {
        tx.Deserialize(reader);
    }
    catch (const std::exception& ex)
    {
        throw RpcException(ErrorCode::TransactionVerificationFailed,
                           std::string("Transaction rejected: ") + ex.what());
    }
    return tx;
}

}  // namespace

nlohmann::json RPCMethods::GetVersion(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    (void)params;
    nlohmann::json result = nlohmann::json::object();

    auto settings = neoSystem ? neoSystem->GetProtocolSettings() : nullptr;
    if (settings)
    {
        result["network"] = settings->GetNetwork();
    }
    else
    {
        result["network"] = nullptr;
    }

    result["port"] = 0;

    std::random_device rd;
    std::mt19937_64 gen(rd());
    result["nonce"] = static_cast<uint64_t>(gen());

    result["useragent"] = "/neo-cpp:0.1.0/";

    return result;
}

nlohmann::json RPCMethods::GetBlockCount(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    (void)params;
    auto blockchain = RequireBlockchain(neoSystem);
    return blockchain->GetHeight() + 1;
}

nlohmann::json RPCMethods::GetBlock(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw RpcException(ErrorCode::InvalidParams, "Missing block identifier");
    }

    auto blockchain = RequireBlockchain(neoSystem);

    bool verbose = true;
    if (params.size() > 1 && params[1].is_boolean())
    {
        verbose = params[1].get<bool>();
    }

    std::shared_ptr<ledger::Block> block;
    if (params[0].is_number_integer())
    {
        const auto indexValue = params[0].get<int64_t>();
        if (indexValue < 0)
        {
            throw RpcException(ErrorCode::InvalidParams, "Invalid block index");
        }
        block = blockchain->GetBlock(static_cast<uint32_t>(indexValue));
    }
    else if (params[0].is_string())
    {
        io::UInt256 hash;
        if (!io::UInt256::TryParse(params[0].get<std::string>(), hash))
        {
            throw RpcException(ErrorCode::InvalidParams, "Invalid block hash");
        }
        block = blockchain->GetBlock(hash);
    }
    else
    {
        throw RpcException(ErrorCode::InvalidParams, "Invalid block identifier");
    }

    if (!block)
    {
        throw RpcException(ErrorCode::UnknownBlock, "Block not found");
    }

    auto protocolSettings = neoSystem ? neoSystem->GetProtocolSettings() : nullptr;

    if (!verbose)
    {
        return SerializeBlock(*block, false, protocolSettings, blockchain);
    }

    const uint32_t currentHeight = blockchain->GetHeight();
    return SerializeBlock(*block, true, protocolSettings, blockchain, currentHeight);
}

nlohmann::json RPCMethods::GetBlockHash(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty() || !params[0].is_number_integer())
    {
        throw RpcException(ErrorCode::InvalidParams, "Missing block index parameter");
    }

    auto blockchain = RequireBlockchain(neoSystem);
    auto indexValue = params[0].get<int64_t>();
    if (indexValue < 0)
    {
        throw RpcException(ErrorCode::InvalidParams, "Invalid block index");
    }
    uint32_t index = static_cast<uint32_t>(indexValue);
    if (index > blockchain->GetHeight())
    {
        throw RpcException(ErrorCode::UnknownBlock, "Block height out of range");
    }
    return blockchain->GetBlockHash(index).ToString();
}

nlohmann::json RPCMethods::GetBlockHeader(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw RpcException(ErrorCode::InvalidParams, "Missing block identifier");
    }

    auto blockchain = RequireBlockchain(neoSystem);

    bool verbose = true;
    if (params.size() > 1 && params[1].is_boolean())
    {
        verbose = params[1].get<bool>();
    }

    std::shared_ptr<ledger::BlockHeader> header;
    if (params[0].is_number_integer())
    {
        auto indexValue = params[0].get<int64_t>();
        if (indexValue < 0)
        {
            throw RpcException(ErrorCode::InvalidParams, "Invalid block index");
        }
        header = blockchain->GetBlockHeader(static_cast<uint32_t>(indexValue));
    }
    else if (params[0].is_string())
    {
        io::UInt256 hash;
        if (!io::UInt256::TryParse(params[0].get<std::string>(), hash))
        {
            throw RpcException(ErrorCode::InvalidParams, "Invalid block hash");
        }
        header = blockchain->GetBlockHeader(hash);
    }
    else
    {
        throw RpcException(ErrorCode::InvalidParams, "Invalid block identifier");
    }

    if (!header)
    {
        throw RpcException(ErrorCode::UnknownBlock, "Block not found");
    }

    auto protocolSettings = neoSystem ? neoSystem->GetProtocolSettings() : nullptr;
    if (!verbose)
    {
        return SerializeBlockHeader(*header, false, protocolSettings, blockchain);
    }

    const uint32_t currentHeight = blockchain->GetHeight();
    return SerializeBlockHeader(*header, true, protocolSettings, blockchain, currentHeight);
}

nlohmann::json RPCMethods::GetRawMemPool(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    bool includeUnverified = false;
    if (!params.empty())
    {
        if (!params[0].is_boolean())
        {
            throw RpcException(ErrorCode::InvalidParams, "Invalid parameter for getrawmempool");
        }
        includeUnverified = params[0].get<bool>();
    }

    auto memoryPool = neoSystem ? neoSystem->GetMemoryPool() : nullptr;

    if (!includeUnverified)
    {
        nlohmann::json array = nlohmann::json::array();
        if (memoryPool)
        {
            for (const auto& tx : memoryPool->GetVerifiedTransactions())
            {
                array.push_back(tx.GetHash().ToString());
            }
        }
        return array;
    }

    auto blockchain = RequireBlockchain(neoSystem);
    nlohmann::json result;
    result["height"] = blockchain->GetHeight();
    nlohmann::json verified = nlohmann::json::array();
    nlohmann::json unverified = nlohmann::json::array();

    if (memoryPool)
    {
        for (const auto& tx : memoryPool->GetVerifiedTransactions())
        {
            verified.push_back(tx.GetHash().ToString());
        }
        for (const auto& tx : memoryPool->GetUnverifiedTransactions())
        {
            unverified.push_back(tx.GetHash().ToString());
        }
    }

    result["verified"] = std::move(verified);
    result["unverified"] = std::move(unverified);
    return result;
}

nlohmann::json RPCMethods::GetRawTransaction(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty() || !params[0].is_string())
    {
        throw RpcException(ErrorCode::InvalidParams, "Missing transaction hash parameter");
    }

    io::UInt256 hash;
    if (!io::UInt256::TryParse(params[0].get<std::string>(), hash))
    {
        throw RpcException(ErrorCode::InvalidParams, "Invalid transaction hash");
    }

    bool verbose = true;
    if (params.size() > 1)
    {
        if (!params[1].is_boolean())
        {
            throw RpcException(ErrorCode::InvalidParams, "Invalid verbose flag");
        }
        verbose = params[1].get<bool>();
    }

    auto blockchain = RequireBlockchain(neoSystem);
    auto memoryPool = neoSystem ? neoSystem->GetMemoryPool() : nullptr;
    auto protocolSettings = neoSystem ? neoSystem->GetProtocolSettings() : nullptr;

    const network::p2p::payloads::Neo3Transaction* memTx =
        memoryPool ? memoryPool->GetTransaction(hash) : nullptr;

    if (!verbose && memTx)
    {
        return SerializeTransaction(*memTx, false, protocolSettings);
    }

    auto tx = blockchain->GetTransaction(hash);
    if (!tx && !memTx)
    {
        throw RpcException(ErrorCode::UnknownTransaction, "Transaction not found");
    }

    std::optional<uint32_t> blockIndex;
    std::optional<uint64_t> blockTimestamp;
    if (tx)
    {
        int32_t height = blockchain->GetTransactionHeight(hash);
        if (height >= 0)
        {
            blockIndex = static_cast<uint32_t>(height);
            if (auto header = blockchain->GetBlockHeader(*blockIndex))
            {
                blockTimestamp = header->GetTimestamp();
            }
        }
    }

    const auto& txRef = tx ? *tx : *memTx;
    return SerializeTransaction(txRef, true, protocolSettings, blockchain, blockIndex, blockTimestamp);
}

nlohmann::json RPCMethods::GetTransactionHeight(std::shared_ptr<node::NeoSystem> neoSystem,
                                                const nlohmann::json& params)
{
    if (params.empty() || !params[0].is_string())
    {
        throw RpcException(ErrorCode::InvalidParams, "Missing transaction hash parameter");
    }

    auto blockchain = RequireBlockchain(neoSystem);

    io::UInt256 hash;
    if (!io::UInt256::TryParse(params[0].get<std::string>(), hash))
    {
        throw RpcException(ErrorCode::InvalidParams, "Invalid transaction hash");
    }

    int32_t height = blockchain->GetTransactionHeight(hash);
    if (height < 0)
    {
        throw RpcException(ErrorCode::UnknownTransaction, "Transaction not found");
    }
    return height;
}

nlohmann::json RPCMethods::SendRawTransaction(std::shared_ptr<node::NeoSystem> neoSystem,
                                              const nlohmann::json& params)
{
    if (!params.is_array() || params.empty() || !params[0].is_string())
    {
        throw RpcException(ErrorCode::InvalidParams, "sendrawtransaction requires the transaction payload");
    }

    auto blockchain = RequireBlockchain(neoSystem);
    auto memoryPool = RequireMemoryPool(neoSystem);

    auto tx = DeserializeNeo3Transaction(params[0].get<std::string>());

    if (blockchain->ContainsTransaction(tx.GetHash()) || memoryPool->Contains(tx.GetHash()))
    {
        throw RpcException(ErrorCode::TransactionAlreadyExists, "Transaction rejected: AlreadyExists");
    }

    if (tx.GetSigners().empty())
    {
        throw RpcException(ErrorCode::InvalidWitness, "Transaction rejected: InvalidWitness");
    }

    auto validation = ledger::ValidateTransaction(tx, blockchain, memoryPool);
    if (validation != ValidationResult::Valid)
    {
        throw RpcException(MapValidationResult(validation),
                           "Transaction rejected: " + ValidationResultToString(validation));
    }

    if (!memoryPool->TryAdd(tx))
    {
        if (memoryPool->Contains(tx.GetHash()))
        {
            throw RpcException(ErrorCode::TransactionAlreadyExists, "Transaction rejected: AlreadyExists");
        }
        if (memoryPool->IsFull())
        {
            throw RpcException(ErrorCode::MemoryPoolNotAvailable, "Transaction rejected: OutOfMemory");
        }
        throw RpcException(ErrorCode::TransactionVerificationFailed, "Transaction rejected");
    }

    return nlohmann::json{{"hash", tx.GetHash().ToString()}};
}

nlohmann::json RPCMethods::InvokeFunction(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    ThrowNotImplemented("invokefunction");
}

nlohmann::json RPCMethods::InvokeScript(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    ThrowNotImplemented("invokescript");
}

nlohmann::json RPCMethods::GetContractState(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw RpcException(ErrorCode::InvalidParams, "getcontractstate requires a contract identifier");
    }

    auto snapshot = RequireSnapshot(neoSystem);
    auto contractManagement = RequireContractManagement();
    auto storeView = std::static_pointer_cast<persistence::StoreView>(snapshot);

    auto serializeContract =
        [](const std::shared_ptr<smartcontract::ContractState>& contract) -> nlohmann::json
    {
        if (!contract)
        {
            throw RpcException(ErrorCode::UnknownContract, "Contract not found");
        }

        nlohmann::json result;
        result["id"] = contract->GetId();
        result["updatecounter"] = contract->GetUpdateCounter();
        result["hash"] = contract->GetScriptHash().ToString();

        nlohmann::json nefJson;
        nefJson["magic"] = 0x3346454E;  // 'NEF3'
        nefJson["compiler"] = "neo-cpp";
        nefJson["tokens"] = nlohmann::json::array();
        nefJson["script"] = cryptography::Base64::Encode(contract->GetScript().AsSpan());
        nefJson["checksum"] = 0;
        result["nef"] = std::move(nefJson);

        try
        {
            result["manifest"] = nlohmann::json::parse(contract->GetManifest());
        }
        catch (const std::exception& ex)
        {
            throw RpcException(ErrorCode::InternalError,
                               std::string("Failed to parse contract manifest: ") + ex.what());
        }

        return result;
    };

    auto serializeNative =
        [](const std::shared_ptr<smartcontract::native::NativeContract>& nativeContract) -> nlohmann::json
    {
        if (!nativeContract)
        {
            throw RpcException(ErrorCode::UnknownContract, "Contract not found");
        }

        nlohmann::json result;
        result["id"] = static_cast<int32_t>(nativeContract->GetId());
        result["updatecounter"] = 0;
        result["hash"] = nativeContract->GetScriptHash().ToString();

        nlohmann::json nefJson;
        nefJson["magic"] = 0x3346454E;  // 'NEF3'
        nefJson["compiler"] = "neo-cpp";
        nefJson["tokens"] = nlohmann::json::array();
        nefJson["script"] = "";
        nefJson["checksum"] = 0;
        result["nef"] = std::move(nefJson);

        nlohmann::json manifestJson;
        manifestJson["name"] = nativeContract->GetName();
        result["manifest"] = std::move(manifestJson);

        return result;
    };

    auto findContractById = [&](int32_t id) -> std::shared_ptr<smartcontract::ContractState>
    {
        auto contracts = contractManagement->ListContracts(storeView);
        for (const auto& contract : contracts)
        {
            if (contract && contract->GetId() == id)
            {
                return contract;
            }
        }
        return nullptr;
    };

    auto& nativeManager = smartcontract::native::NativeContractManager::GetInstance();
    nativeManager.Initialize();

    if (params[0].is_number_integer())
    {
        auto idValue = params[0].get<int64_t>();
        if (idValue < std::numeric_limits<int32_t>::min() || idValue > std::numeric_limits<int32_t>::max())
        {
            throw RpcException(ErrorCode::InvalidParams, "Contract id out of range");
        }

        auto contract = findContractById(static_cast<int32_t>(idValue));
        if (contract)
        {
            return serializeContract(contract);
        }

        auto nativeContract = nativeManager.GetContract(static_cast<uint32_t>(idValue));
        if (nativeContract)
        {
            return serializeNative(nativeContract);
        }

        throw RpcException(ErrorCode::UnknownContract, "Contract not found");
    }

    if (!params[0].is_string())
    {
        throw RpcException(ErrorCode::InvalidParams, "Contract identifier must be string or integer");
    }

    const auto identifier = params[0].get<std::string>();

    io::UInt160 scriptHash;
    bool hashParsed = io::UInt160::TryParse(identifier, scriptHash);
    if (!hashParsed)
    {
        try
        {
            scriptHash = wallets::Helper::ToScriptHash(identifier);
            hashParsed = true;
        }
        catch (...)
        {
            hashParsed = false;
        }
    }

    if (hashParsed)
    {
        auto contract = contractManagement->GetContract(storeView, scriptHash);
        if (contract)
        {
            return serializeContract(contract);
        }

        auto nativeContract = nativeManager.GetContract(scriptHash);
        if (nativeContract)
        {
            return serializeNative(nativeContract);
        }

        throw RpcException(ErrorCode::UnknownContract, "Contract not found");
    }

    // Try native contract lookup by name (case-insensitive)
    auto toLower = [](std::string value) {
        std::transform(value.begin(), value.end(), value.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return value;
    };

    auto identifierLower = toLower(identifier);

    if (auto nativeByName = nativeManager.GetContract(identifier); nativeByName)
    {
        return serializeNative(nativeByName);
    }

    for (const auto& nativeContract : nativeManager.GetContracts())
    {
        if (!nativeContract) continue;

        auto nameLower = toLower(nativeContract->GetName());
        if (nameLower == identifierLower)
        {
            return serializeNative(nativeContract);
        }
    }

    // Fallback: search deployed contracts by manifest name
    auto contracts = contractManagement->ListContracts(storeView);
    for (const auto& contract : contracts)
    {
        if (!contract) continue;

        try
        {
            auto manifest = nlohmann::json::parse(contract->GetManifest());
            auto it = manifest.find("name");
            if (it != manifest.end() && it->is_string())
            {
                auto manifestName = it->get<std::string>();
                if (manifestName == identifier || toLower(manifestName) == identifierLower)
                {
                    return serializeContract(contract);
                }
            }
        }
        catch (const std::exception&)
        {
            // Ignore malformed manifests during search
        }
    }

    throw RpcException(ErrorCode::UnknownContract, "Contract not found");
}

nlohmann::json RPCMethods::GetUnclaimedGas(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty() || !params[0].is_string())
    {
        throw RpcException(ErrorCode::InvalidParams, "getunclaimedgas requires an address string");
    }

    const auto address = params[0].get<std::string>();

    io::UInt160 scriptHash;
    bool parsed = io::UInt160::TryParse(address, scriptHash);
    if (!parsed)
    {
        try
        {
            scriptHash = wallets::Helper::ToScriptHash(address);
            parsed = true;
        }
        catch (...)
        {
            parsed = false;
        }
    }

    if (!parsed)
    {
        throw RpcException(ErrorCode::InvalidAddress, "Invalid address");
    }

    auto snapshot = RequireSnapshot(neoSystem);
    auto neoToken = smartcontract::native::NeoToken::GetInstance();
    auto ledgerContract = smartcontract::native::LedgerContract::GetInstance();

    if (!neoToken || !ledgerContract)
    {
        throw RpcException(ErrorCode::InternalError, "Native contracts unavailable");
    }

    uint32_t currentIndex = 0;
    try
    {
        currentIndex = ledgerContract->GetCurrentIndex(snapshot);
    }
    catch (const std::exception&)
    {
        currentIndex = 0;
    }

    int64_t unclaimed = 0;
    try
    {
        unclaimed = neoToken->GetUnclaimedGas(snapshot, scriptHash, currentIndex + 1);
    }
    catch (const std::exception&)
    {
        unclaimed = 0;
    }

    auto protocolSettings = neoSystem ? neoSystem->GetProtocolSettings() : nullptr;

    nlohmann::json result;
    result["unclaimed"] = std::to_string(unclaimed);
    result["address"] = FormatScriptHash(scriptHash, protocolSettings);

    return result;
}

nlohmann::json RPCMethods::GetConnectionCount(std::shared_ptr<node::NeoSystem> neoSystem,
                                              const nlohmann::json& params)
{
    (void)params;

    auto localNode = neoSystem ? neoSystem->GetLocalNode() : nullptr;
    if (!localNode)
    {
        return 0;
    }

    return static_cast<uint32_t>(localNode->GetConnectedCount());
}

nlohmann::json RPCMethods::GetPeers(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    (void)params;

    nlohmann::json peers;
    peers["unconnected"] = nlohmann::json::array();
    peers["connected"] = nlohmann::json::array();
    peers["bad"] = nlohmann::json::array();

    auto localNode = neoSystem ? neoSystem->GetLocalNode() : nullptr;
    if (!localNode)
    {
        return peers;
    }

    auto connected = localNode->GetConnectedPeers();
    for (const auto& peer : connected)
    {
        if (!peer) continue;
        nlohmann::json entry;
        const auto endpoint = peer->GetRemoteEndPoint();
        entry["address"] = endpoint.ToString();
        entry["port"] = endpoint.GetPort();
        entry["id"] = peer->GetConnection() ? peer->GetConnection()->GetId() : 0;
        peers["connected"].push_back(entry);
    }

    return peers;
}

nlohmann::json RPCMethods::GetCommittee(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    (void)params;

    auto snapshot = RequireSnapshot(neoSystem);
    auto neoToken = smartcontract::native::NeoToken::GetInstance();
    auto committee = neoToken->GetCommittee(snapshot);

    nlohmann::json result = nlohmann::json::array();
    for (const auto& member : committee)
    {
        result.push_back(member.ToString());
    }
    return result;
}

nlohmann::json RPCMethods::GetValidators(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    (void)params;

    auto snapshot = RequireSnapshot(neoSystem);
    auto neoToken = smartcontract::native::NeoToken::GetInstance();
    auto validators = neoToken->GetValidators(snapshot);

    nlohmann::json result = nlohmann::json::array();
    for (const auto& validator : validators)
    {
        result.push_back(validator.ToString());
    }
    return result;
}

nlohmann::json RPCMethods::GetNextBlockValidators(std::shared_ptr<node::NeoSystem> neoSystem,
                                                  const nlohmann::json& params)
{
    (void)params;

    auto snapshot = RequireSnapshot(neoSystem);
    auto neoToken = smartcontract::native::NeoToken::GetInstance();
    auto settings = neoSystem ? neoSystem->GetProtocolSettings() : nullptr;
    const int validatorCount = settings ? settings->GetValidatorsCount() : 0;

    auto validators = neoToken->GetNextBlockValidators(snapshot, validatorCount);
    nlohmann::json result = nlohmann::json::array();
    for (const auto& validator : validators)
    {
        nlohmann::json entry;
        entry["publickey"] = validator.ToString();
        entry["votes"] = neoToken->GetCandidateVote(snapshot, validator);
        result.push_back(std::move(entry));
    }
    return result;
}

nlohmann::json RPCMethods::GetBestBlockHash(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    (void)params;
    auto blockchain = RequireBlockchain(neoSystem);
    return blockchain->GetBestBlockHash().ToString();
}

nlohmann::json RPCMethods::GetBlockHeaderCount(std::shared_ptr<node::NeoSystem> neoSystem,
                                               const nlohmann::json& params)
{
    (void)params;
    auto blockchain = RequireBlockchain(neoSystem);
    return blockchain->GetHeight() + 1;
}

nlohmann::json RPCMethods::GetStorage(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.size() < 2)
    {
        throw RpcException(ErrorCode::InvalidParams, "getstorage requires contract identifier and key");
    }

    auto contractId = ResolveContractId(neoSystem, params[0]);

    if (!params[1].is_string())
    {
        throw RpcException(ErrorCode::InvalidParams, "Storage key must be a Base64 string");
    }

    const auto keyBase64 = params[1].get<std::string>();
    if (!IsValidBase64String(keyBase64))
    {
        throw RpcException(ErrorCode::InvalidParams, "Invalid Base64 storage key");
    }

    io::ByteVector key;
    try
    {
        key = cryptography::Base64::Decode(keyBase64);
    }
    catch (const std::exception&)
    {
        throw RpcException(ErrorCode::InvalidParams, "Invalid Base64 storage key");
    }

    auto snapshot = RequireSnapshot(neoSystem);
    persistence::StorageKey storageKey(contractId, key);
    auto item = snapshot->TryGet(storageKey);
    if (!item)
    {
        throw RpcException(ErrorCode::UnknownStorageItem, "Storage item not found");
    }

    return cryptography::Base64::Encode(item->GetValue().AsSpan());
}

nlohmann::json RPCMethods::FindStorage(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.size() < 2)
    {
        throw RpcException(ErrorCode::InvalidParams, "findstorage requires contract identifier and prefix");
    }

    auto contractId = ResolveContractId(neoSystem, params[0]);

    if (!params[1].is_string())
    {
        throw RpcException(ErrorCode::InvalidParams, "Storage prefix must be a Base64 string");
    }

    const auto prefixBase64 = params[1].get<std::string>();
    if (!IsValidBase64String(prefixBase64))
    {
        throw RpcException(ErrorCode::InvalidParams, "Invalid Base64 storage prefix");
    }

    io::ByteVector prefix;
    try
    {
        prefix = cryptography::Base64::Decode(prefixBase64);
    }
    catch (const std::exception&)
    {
        throw RpcException(ErrorCode::InvalidParams, "Invalid Base64 storage prefix");
    }

    int64_t start = 0;
    if (params.size() > 2)
    {
        if (!params[2].is_number_integer())
        {
            throw RpcException(ErrorCode::InvalidParams, "findstorage start index must be an integer");
        }
        start = params[2].get<int64_t>();
        if (start < 0)
        {
            throw RpcException(ErrorCode::InvalidParams, "findstorage start index must be non-negative");
        }
    }

    auto snapshot = RequireSnapshot(neoSystem);
    persistence::StorageKey prefixKey(contractId, prefix);
    auto entries = snapshot->Find(&prefixKey);

    nlohmann::json items = nlohmann::json::array();

    size_t begin = static_cast<size_t>(std::min<int64_t>(start, static_cast<int64_t>(entries.size())));
    size_t count = 0;
    for (size_t index = begin; index < entries.size() && count < kFindStoragePageSize; ++index, ++count)
    {
        const auto& entry = entries[index];
        const auto& key = entry.first.GetKey();
        const auto& value = entry.second.GetValue();

        nlohmann::json item;
        item["key"] = cryptography::Base64::Encode(key.AsSpan());
        item["value"] = cryptography::Base64::Encode(value.AsSpan());
        items.push_back(std::move(item));
    }

    nlohmann::json result;
    result["results"] = std::move(items);
    result["next"] = static_cast<int64_t>(begin + count);
    result["truncated"] = (begin + count) < entries.size();

    return result;
}

nlohmann::json RPCMethods::GetCandidates(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    (void)params;

    auto snapshot = RequireSnapshot(neoSystem);
    auto neoToken = smartcontract::native::NeoToken::GetInstance();
    auto settings = neoSystem ? neoSystem->GetProtocolSettings() : nullptr;
    const int validatorCount = settings ? settings->GetValidatorsCount() : 0;

    auto candidates = neoToken->GetCandidates(snapshot);
    auto nextValidators = neoToken->GetNextBlockValidators(snapshot, validatorCount);

    std::unordered_set<std::string> activeValidators;
    for (const auto& validator : nextValidators)
    {
        activeValidators.insert(validator.ToString());
    }

    nlohmann::json result = nlohmann::json::array();
    for (const auto& [publicKey, state] : candidates)
    {
        nlohmann::json entry;
        auto keyString = publicKey.ToString();
        entry["publickey"] = keyString;
        entry["votes"] = std::to_string(state.votes);
        entry["active"] = activeValidators.find(keyString) != activeValidators.end();
        result.push_back(std::move(entry));
    }

    return result;
}

nlohmann::json RPCMethods::GetNativeContracts(std::shared_ptr<node::NeoSystem> neoSystem,
                                              const nlohmann::json& params)
{
    (void)params;

    auto contractManagement = RequireContractManagement();
    auto snapshot = RequireSnapshot(neoSystem);
    auto storeView = std::static_pointer_cast<persistence::StoreView>(snapshot);
    auto& manager = smartcontract::native::NativeContractManager::GetInstance();
    manager.Initialize();

    nlohmann::json result = nlohmann::json::array();

    auto contracts = contractManagement->ListContracts(storeView);
    if (contracts.empty())
    {
        for (const auto& nativeContract : manager.GetContracts())
        {
            nlohmann::json entry;
            entry["id"] = static_cast<int32_t>(nativeContract->GetId());
            entry["updatecounter"] = 0;
            entry["hash"] = nativeContract->GetScriptHash().ToString();

            nlohmann::json nef = nlohmann::json::object();
            nef["magic"] = 0x3346454E;
            nef["compiler"] = "neo-cpp";
            nef["tokens"] = nlohmann::json::array();
            nef["script"] = "";
            nef["checksum"] = 0;
            entry["nef"] = std::move(nef);

            nlohmann::json manifestJson = nlohmann::json::object();
            manifestJson["name"] = nativeContract->GetName();
            entry["manifest"] = std::move(manifestJson);

            result.push_back(std::move(entry));
        }
        return result;
    }

    for (const auto& contract : contracts)
    {
        nlohmann::json entry;
        entry["id"] = contract->GetId();
        entry["updatecounter"] = contract->GetUpdateCounter();
        entry["hash"] = contract->GetScriptHash().ToString();

        nlohmann::json nef = nlohmann::json::object();
        nef["magic"] = 0x3346454E;  // 'NEF3'
        nef["compiler"] = "neo-cpp";
        nef["tokens"] = nlohmann::json::array();
        nef["script"] = cryptography::Base64::Encode(contract->GetScript().AsSpan());
        nef["checksum"] = 0;
        entry["nef"] = std::move(nef);

        try
        {
            entry["manifest"] = nlohmann::json::parse(contract->GetManifest());
        }
        catch (const std::exception& ex)
        {
            throw RpcException(ErrorCode::InternalError,
                               std::string("Failed to parse contract manifest: ") + ex.what());
        }

        result.push_back(std::move(entry));
    }

    return result;
}

nlohmann::json RPCMethods::SubmitBlock(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (!params.is_array() || params.empty() || !params[0].is_string())
    {
        throw RpcException(ErrorCode::InvalidParams, "submitblock requires a base64-encoded block payload");
    }

    bool relay = true;
    if (params.size() >= 2)
    {
        if (params[1].is_boolean())
        {
            relay = params[1].get<bool>();
        }
        else if (params[1].is_number_integer())
        {
            relay = params[1].get<int64_t>() != 0;
        }
        else
        {
            throw RpcException(ErrorCode::InvalidParams, "submitblock relay flag must be a boolean value");
        }
    }

    const auto& encodedBlock = params[0].get<std::string>();
    if (!IsValidBase64String(encodedBlock))
    {
        throw RpcException(ErrorCode::InvalidParams, "submitblock payload is not valid Base64");
    }

    io::ByteVector blockBytes;
    try
    {
        blockBytes = cryptography::Base64::Decode(encodedBlock);
    }
    catch (const std::exception& ex)
    {
        throw RpcException(ErrorCode::InvalidParams,
                           std::string("Failed to decode block payload: ") + ex.what());
    }

    if (blockBytes.IsEmpty())
    {
        throw RpcException(ErrorCode::InvalidParams, "Block payload is empty");
    }

    auto blockchain = RequireBlockchain(neoSystem);
    auto block = std::make_shared<ledger::Block>();
    try
    {
        io::BinaryReader reader(blockBytes);
        block->Deserialize(reader);
    }
    catch (const std::exception& ex)
    {
        throw RpcException(ErrorCode::InvalidParams,
                           std::string("Invalid block serialization: ") + ex.what());
    }

    auto verifyResult = blockchain->OnNewBlock(block);
    if (verifyResult != VerifyResult::Succeed)
    {
        throw RpcException(MapVerifyResult(verifyResult),
                           "Block rejected: " + VerifyResultToString(verifyResult));
    }

    if (relay && neoSystem)
    {
        auto localNode = neoSystem->GetLocalNode();
        if (localNode)
        {
            localNode->RelayBlock(block);
        }
    }

    return nlohmann::json{{"hash", block->GetHash().ToString()}};
}

nlohmann::json RPCMethods::ValidateAddress(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    (void)neoSystem;
    if (params.empty() || !params[0].is_string())
    {
        throw RpcException(ErrorCode::InvalidParams, "validateaddress requires an address string");
    }

    const auto address = params[0].get<std::string>();
    bool isValid = wallets::Helper::IsValidAddress(address);

    nlohmann::json result;
    result["address"] = address;
    result["isvalid"] = isValid;
    return result;
}

nlohmann::json RPCMethods::ListPlugins(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    (void)neoSystem;
    (void)params;

    auto& manager = plugins::PluginManager::GetInstance();
    nlohmann::json result = nlohmann::json::array();
    for (const auto& plugin : manager.GetPlugins())
    {
        if (!plugin) continue;
        nlohmann::json entry;
        entry["name"] = plugin->GetName();
        entry["description"] = plugin->GetDescription();
        entry["version"] = plugin->GetVersion();
        entry["author"] = plugin->GetAuthor();
        entry["running"] = plugin->IsRunning();
        result.push_back(std::move(entry));
    }
    return result;
}

nlohmann::json RPCMethods::TraverseIterator(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    (void)neoSystem;
    if (params.size() < 2)
    {
        throw RpcException(ErrorCode::InvalidParams, "traverseiterator requires session id and iterator id");
    }

    if (!params[0].is_string() || !params[1].is_string())
    {
        throw RpcException(ErrorCode::InvalidParams, "Session id and iterator id must be strings");
    }

    const auto sessionId = params[0].get<std::string>();
    const auto iteratorId = params[1].get<std::string>();

    size_t count = 100;
    if (params.size() >= 3)
    {
        if (!params[2].is_number_unsigned() && !params[2].is_number_integer())
        {
            throw RpcException(ErrorCode::InvalidParams, "Max item count must be a non-negative integer");
        }

        auto countValue = params[2].get<int64_t>();
        if (countValue < 0)
        {
            throw RpcException(ErrorCode::InvalidParams, "Max item count must be non-negative");
        }

        count = countValue == 0 ? static_cast<size_t>(100) : static_cast<size_t>(countValue);
    }

    auto& manager = RpcSessionManager::Instance();
    auto result = manager.Traverse(sessionId, iteratorId, count);
    if (!result.found)
    {
        throw RpcException(ErrorCode::UnknownIterator, "Iterator not found");
    }

    nlohmann::json response;
    response["values"] = result.items;
    response["truncated"] = result.hasMore;
    return response;
}

nlohmann::json RPCMethods::TerminateSession(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    (void)neoSystem;
    if (params.empty() || !params[0].is_string())
    {
        throw RpcException(ErrorCode::InvalidParams, "terminatesession requires a session id");
    }

    const auto sessionId = params[0].get<std::string>();
    auto& manager = RpcSessionManager::Instance();
    if (!manager.TerminateSession(sessionId))
    {
        throw RpcException(ErrorCode::UnknownSession, "Session not found");
    }
    return true;
}

nlohmann::json RPCMethods::InvokeContractVerify(std::shared_ptr<node::NeoSystem> neoSystem,
                                                const nlohmann::json& params)
{
    ThrowNotImplemented("invokecontractverify");
}

nlohmann::json RPCMethods::GetStateRoot(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    (void)neoSystem;
    (void)params;
    throw RpcException(ErrorCode::StateServiceNotEnabled, "State root service not available");
}

nlohmann::json RPCMethods::GetState(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    (void)neoSystem;
    (void)params;
    throw RpcException(ErrorCode::StateServiceNotEnabled, "State service not available");
}

nlohmann::json RPCMethods::BlockToJson(std::shared_ptr<ledger::Block> block, bool verbose)
{
    if (!block) return nullptr;
    return SerializeBlock(*block, verbose);
}

nlohmann::json RPCMethods::TransactionToJson(std::shared_ptr<ledger::Transaction> tx, bool verbose)
{
    if (!tx) return nullptr;
    return SerializeTransaction(*tx, verbose);
}

nlohmann::json RPCMethods::ContractToJson(std::shared_ptr<smartcontract::ContractState> contract)
{
    (void)contract;
    return nlohmann::json::object();
}
}  // namespace neo::rpc
