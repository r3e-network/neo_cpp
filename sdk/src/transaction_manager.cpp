// transaction_manager.cpp - Full implementation of Neo transaction management

#include <neo/sdk/transaction/transaction_manager.h>
#include <neo/sdk/crypto/crypto.h>
#include <openssl/sha.h>
#include <random>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace neo::sdk::transaction {

// Helper functions
namespace {
    // Convert bytes to hex string
    std::string BytesToHex(const std::vector<uint8_t>& bytes) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (uint8_t byte : bytes) {
            ss << std::setw(2) << static_cast<int>(byte);
        }
        return ss.str();
    }

    // Convert hex string to bytes
    std::vector<uint8_t> HexToBytes(const std::string& hex) {
        std::vector<uint8_t> bytes;
        for (size_t i = 0; i < hex.length(); i += 2) {
            std::string byteString = hex.substr(i, 2);
            uint8_t byte = static_cast<uint8_t>(std::strtoul(byteString.c_str(), nullptr, 16));
            bytes.push_back(byte);
        }
        return bytes;
    }

    // Write variable length integer
    void WriteVarInt(std::vector<uint8_t>& buffer, uint64_t value) {
        if (value < 0xFD) {
            buffer.push_back(static_cast<uint8_t>(value));
        } else if (value <= 0xFFFF) {
            buffer.push_back(0xFD);
            buffer.push_back(value & 0xFF);
            buffer.push_back((value >> 8) & 0xFF);
        } else if (value <= 0xFFFFFFFF) {
            buffer.push_back(0xFE);
            buffer.push_back(value & 0xFF);
            buffer.push_back((value >> 8) & 0xFF);
            buffer.push_back((value >> 16) & 0xFF);
            buffer.push_back((value >> 24) & 0xFF);
        } else {
            buffer.push_back(0xFF);
            for (int i = 0; i < 8; i++) {
                buffer.push_back((value >> (i * 8)) & 0xFF);
            }
        }
    }

    // Read variable length integer
    uint64_t ReadVarInt(const std::vector<uint8_t>& buffer, size_t& offset) {
        uint8_t first = buffer[offset++];
        if (first < 0xFD) {
            return first;
        } else if (first == 0xFD) {
            uint64_t value = buffer[offset] | (buffer[offset + 1] << 8);
            offset += 2;
            return value;
        } else if (first == 0xFE) {
            uint64_t value = buffer[offset] | (buffer[offset + 1] << 8) | 
                           (buffer[offset + 2] << 16) | (buffer[offset + 3] << 24);
            offset += 4;
            return value;
        } else {
            uint64_t value = 0;
            for (int i = 0; i < 8; i++) {
                value |= static_cast<uint64_t>(buffer[offset + i]) << (i * 8);
            }
            offset += 8;
            return value;
        }
    }

    // Write fixed size value
    template<typename T>
    void WriteFixed(std::vector<uint8_t>& buffer, T value) {
        for (size_t i = 0; i < sizeof(T); i++) {
            buffer.push_back((value >> (i * 8)) & 0xFF);
        }
    }

    // Read fixed size value
    template<typename T>
    T ReadFixed(const std::vector<uint8_t>& buffer, size_t& offset) {
        T value = 0;
        for (size_t i = 0; i < sizeof(T); i++) {
            value |= static_cast<T>(buffer[offset + i]) << (i * 8);
        }
        offset += sizeof(T);
        return value;
    }

    // Address to script hash conversion
    std::vector<uint8_t> AddressToScriptHash(const std::string& address) {
        // Validate address format
        if (address.empty() || (address[0] != 'N' && address[0] != 'A')) {
            throw std::invalid_argument("Invalid Neo address format");
        }
        
        // Base58 decode the address
        std::vector<uint8_t> decoded = crypto::Base58CheckDecode(address);
        
        // Extract the script hash (skip version byte)
        if (decoded.size() < 21) {
            throw std::invalid_argument("Invalid address length");
        }
        
        std::vector<uint8_t> scriptHash(decoded.begin() + 1, decoded.begin() + 21);
        return scriptHash;
    }

    // Script hash to address conversion
    std::string ScriptHashToAddress(const std::vector<uint8_t>& scriptHash) {
        if (scriptHash.size() != 20) {
            throw std::invalid_argument("Script hash must be 20 bytes");
        }
        
        // Add version byte (0x35 for mainnet N-addresses)
        std::vector<uint8_t> data;
        data.push_back(0x35);
        data.insert(data.end(), scriptHash.begin(), scriptHash.end());
        
        // Base58 encode with checksum
        return crypto::Base58CheckEncode(data);
    }
}

// Transaction implementation
Transaction::Transaction() {
    // Generate random nonce
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis;
    nonce = dis(gen);
}

uint64_t Transaction::CalculateNetworkFee(rpc::RpcClient& client) {
    // Calculate based on transaction size and witness verification
    uint64_t baseFee = 1000000;  // 0.001 GAS base fee
    uint64_t sizeFee = Serialize().size() * 1000;  // Fee per byte
    uint64_t witnessVerificationFee = witnesses.size() * 1000000;  // Fee per witness
    
    return baseFee + sizeFee + witnessVerificationFee;
}

uint64_t Transaction::CalculateSystemFee(rpc::RpcClient& client) {
    // Use RPC to calculate system fee
    std::string txHex = SerializeToHex();
    auto result = client.Call("calculatenetworkfee", {txHex});
    
    if (result.contains("systemfee")) {
        return result["systemfee"].get<uint64_t>();
    }
    
    // Default system fee
    return 0;
}

std::string Transaction::GetHash() const {
    // Serialize transaction without witnesses
    std::vector<uint8_t> data;
    
    // Version
    data.push_back(version);
    
    // Nonce
    WriteFixed(data, nonce);
    
    // System fee
    WriteFixed(data, systemFee);
    
    // Network fee
    WriteFixed(data, networkFee);
    
    // Valid until block
    WriteFixed(data, validUntilBlock);
    
    // Signers
    WriteVarInt(data, signers.size());
    for (const auto& signer : signers) {
        auto scriptHash = HexToBytes(signer.account);
        data.insert(data.end(), scriptHash.begin(), scriptHash.end());
        data.push_back(static_cast<uint8_t>(signer.scopes));
        
        if ((static_cast<uint8_t>(signer.scopes) & static_cast<uint8_t>(WitnessScope::CustomContracts)) != 0) {
            WriteVarInt(data, signer.allowedContracts.size());
            for (const auto& contract : signer.allowedContracts) {
                auto contractHash = HexToBytes(contract);
                data.insert(data.end(), contractHash.begin(), contractHash.end());
            }
        }
        
        if ((static_cast<uint8_t>(signer.scopes) & static_cast<uint8_t>(WitnessScope::CustomGroups)) != 0) {
            WriteVarInt(data, signer.allowedGroups.size());
            for (const auto& group : signer.allowedGroups) {
                auto groupKey = HexToBytes(group);
                data.insert(data.end(), groupKey.begin(), groupKey.end());
            }
        }
    }
    
    // Attributes
    WriteVarInt(data, attributes.size());
    for (const auto& attr : attributes) {
        data.push_back(static_cast<uint8_t>(attr.type));
        WriteVarInt(data, attr.data.size());
        data.insert(data.end(), attr.data.begin(), attr.data.end());
    }
    
    // Script
    WriteVarInt(data, script.size());
    data.insert(data.end(), script.begin(), script.end());
    
    // Calculate SHA256 hash
    uint8_t hash[SHA256_DIGEST_LENGTH];
    SHA256(data.data(), data.size(), hash);
    
    return BytesToHex(std::vector<uint8_t>(hash, hash + SHA256_DIGEST_LENGTH));
}

std::vector<uint8_t> Transaction::Serialize() const {
    std::vector<uint8_t> data;
    
    // Serialize transaction (same as GetHash but including witnesses)
    data.push_back(version);
    WriteFixed(data, nonce);
    WriteFixed(data, systemFee);
    WriteFixed(data, networkFee);
    WriteFixed(data, validUntilBlock);
    
    // Signers
    WriteVarInt(data, signers.size());
    for (const auto& signer : signers) {
        auto scriptHash = HexToBytes(signer.account);
        data.insert(data.end(), scriptHash.begin(), scriptHash.end());
        data.push_back(static_cast<uint8_t>(signer.scopes));
    }
    
    // Attributes
    WriteVarInt(data, attributes.size());
    for (const auto& attr : attributes) {
        data.push_back(static_cast<uint8_t>(attr.type));
        WriteVarInt(data, attr.data.size());
        data.insert(data.end(), attr.data.begin(), attr.data.end());
    }
    
    // Script
    WriteVarInt(data, script.size());
    data.insert(data.end(), script.begin(), script.end());
    
    // Witnesses
    WriteVarInt(data, witnesses.size());
    for (const auto& witness : witnesses) {
        WriteVarInt(data, witness.invocationScript.size());
        data.insert(data.end(), witness.invocationScript.begin(), witness.invocationScript.end());
        WriteVarInt(data, witness.verificationScript.size());
        data.insert(data.end(), witness.verificationScript.begin(), witness.verificationScript.end());
    }
    
    return data;
}

std::string Transaction::SerializeToHex() const {
    return BytesToHex(Serialize());
}

std::shared_ptr<Transaction> Transaction::Deserialize(const std::vector<uint8_t>& data) {
    auto tx = std::make_shared<Transaction>();
    size_t offset = 0;
    
    // Version
    tx->version = data[offset++];
    
    // Nonce
    tx->nonce = ReadFixed<uint32_t>(data, offset);
    
    // System fee
    tx->systemFee = ReadFixed<uint64_t>(data, offset);
    
    // Network fee
    tx->networkFee = ReadFixed<uint64_t>(data, offset);
    
    // Valid until block
    tx->validUntilBlock = ReadFixed<uint32_t>(data, offset);
    
    // Signers
    uint64_t signerCount = ReadVarInt(data, offset);
    for (uint64_t i = 0; i < signerCount; i++) {
        Signer signer;
        std::vector<uint8_t> scriptHash(data.begin() + offset, data.begin() + offset + 20);
        signer.account = BytesToHex(scriptHash);
        offset += 20;
        signer.scopes = static_cast<WitnessScope>(data[offset++]);
        tx->signers.push_back(signer);
    }
    
    // Attributes
    uint64_t attrCount = ReadVarInt(data, offset);
    for (uint64_t i = 0; i < attrCount; i++) {
        TransactionAttribute attr;
        attr.type = static_cast<TransactionAttributeType>(data[offset++]);
        uint64_t dataLen = ReadVarInt(data, offset);
        attr.data = std::vector<uint8_t>(data.begin() + offset, data.begin() + offset + dataLen);
        offset += dataLen;
        tx->attributes.push_back(attr);
    }
    
    // Script
    uint64_t scriptLen = ReadVarInt(data, offset);
    tx->script = std::vector<uint8_t>(data.begin() + offset, data.begin() + offset + scriptLen);
    offset += scriptLen;
    
    // Witnesses
    uint64_t witnessCount = ReadVarInt(data, offset);
    for (uint64_t i = 0; i < witnessCount; i++) {
        Witness witness;
        uint64_t invLen = ReadVarInt(data, offset);
        witness.invocationScript = std::vector<uint8_t>(data.begin() + offset, data.begin() + offset + invLen);
        offset += invLen;
        uint64_t verLen = ReadVarInt(data, offset);
        witness.verificationScript = std::vector<uint8_t>(data.begin() + offset, data.begin() + offset + verLen);
        offset += verLen;
        tx->witnesses.push_back(witness);
    }
    
    return tx;
}

std::shared_ptr<Transaction> Transaction::DeserializeFromHex(const std::string& hex) {
    return Deserialize(HexToBytes(hex));
}

void Transaction::AddWitness(const Witness& witness) {
    witnesses.push_back(witness);
}

void Transaction::Sign(const std::string& privateKey) {
    // Sign transaction with private key using ECDSA
    auto txHash = HexToBytes(GetHash());
    
    // Create key pair from private key
    auto keyPair = crypto::KeyPair::FromPrivateKey(HexToBytes(privateKey));
    
    // Sign the transaction hash
    auto signature = crypto::Sign(txHash, keyPair.GetPrivateKey());
    
    // Create invocation script (push signature)
    std::vector<uint8_t> invocationScript;
    invocationScript.push_back(0x40);  // PUSHBYTES64
    invocationScript.insert(invocationScript.end(), signature.begin(), signature.end());
    
    // Create verification script (push public key + CHECKSIG)
    std::vector<uint8_t> verificationScript;
    auto publicKey = keyPair.GetPublicKey();
    verificationScript.push_back(0x21);  // PUSHBYTES33
    verificationScript.insert(verificationScript.end(), publicKey.begin(), publicKey.end());
    verificationScript.push_back(0xAC);  // CHECKSIG
    
    // Create witness
    Witness witness;
    witness.invocationScript = invocationScript;
    witness.verificationScript = verificationScript;
    
    AddWitness(witness);
}

bool Transaction::Verify() const {
    // Verify all witnesses
    auto txHash = HexToBytes(GetHash());
    
    for (const auto& witness : witnesses) {
        if (witness.invocationScript.empty() || witness.verificationScript.empty()) {
            return false;
        }
        
        // Extract signature from invocation script
        if (witness.invocationScript.size() < 65 || witness.invocationScript[0] != 0x40) {
            return false;
        }
        std::vector<uint8_t> signature(witness.invocationScript.begin() + 1, 
                                       witness.invocationScript.begin() + 65);
        
        // Extract public key from verification script
        if (witness.verificationScript.size() < 35 || witness.verificationScript[0] != 0x21) {
            return false;
        }
        std::vector<uint8_t> publicKey(witness.verificationScript.begin() + 1,
                                       witness.verificationScript.begin() + 34);
        
        // Verify signature
        if (!crypto::Verify(txHash, signature, publicKey)) {
            return false;
        }
    }
    return true;
}

nlohmann::json Transaction::ToJSON() const {
    nlohmann::json json;
    json["version"] = version;
    json["nonce"] = nonce;
    json["sysfee"] = std::to_string(systemFee);
    json["netfee"] = std::to_string(networkFee);
    json["validuntilblock"] = validUntilBlock;
    json["script"] = BytesToHex(script);
    
    json["signers"] = nlohmann::json::array();
    for (const auto& signer : signers) {
        nlohmann::json signerJson;
        signerJson["account"] = signer.account;
        signerJson["scopes"] = static_cast<int>(signer.scopes);
        json["signers"].push_back(signerJson);
    }
    
    json["attributes"] = nlohmann::json::array();
    for (const auto& attr : attributes) {
        nlohmann::json attrJson;
        attrJson["type"] = static_cast<int>(attr.type);
        attrJson["data"] = BytesToHex(attr.data);
        json["attributes"].push_back(attrJson);
    }
    
    json["witnesses"] = nlohmann::json::array();
    for (const auto& witness : witnesses) {
        nlohmann::json witnessJson;
        witnessJson["invocation"] = BytesToHex(witness.invocationScript);
        witnessJson["verification"] = BytesToHex(witness.verificationScript);
        json["witnesses"].push_back(witnessJson);
    }
    
    return json;
}

std::shared_ptr<Transaction> Transaction::FromJSON(const nlohmann::json& json) {
    auto tx = std::make_shared<Transaction>();
    
    tx->version = json["version"];
    tx->nonce = json["nonce"];
    tx->systemFee = std::stoull(json["sysfee"].get<std::string>());
    tx->networkFee = std::stoull(json["netfee"].get<std::string>());
    tx->validUntilBlock = json["validuntilblock"];
    tx->script = HexToBytes(json["script"]);
    
    for (const auto& signerJson : json["signers"]) {
        Signer signer;
        signer.account = signerJson["account"];
        signer.scopes = static_cast<WitnessScope>(signerJson["scopes"].get<int>());
        tx->signers.push_back(signer);
    }
    
    if (json.contains("attributes")) {
        for (const auto& attrJson : json["attributes"]) {
            TransactionAttribute attr;
            attr.type = static_cast<TransactionAttributeType>(attrJson["type"].get<int>());
            attr.data = HexToBytes(attrJson["data"]);
            tx->attributes.push_back(attr);
        }
    }
    
    if (json.contains("witnesses")) {
        for (const auto& witnessJson : json["witnesses"]) {
            Witness witness;
            witness.invocationScript = HexToBytes(witnessJson["invocation"]);
            witness.verificationScript = HexToBytes(witnessJson["verification"]);
            tx->witnesses.push_back(witness);
        }
    }
    
    return tx;
}

// TransactionManager implementation
TransactionManager::TransactionManager(std::shared_ptr<rpc::RpcClient> rpcClient)
    : rpcClient_(rpcClient) {
}

std::shared_ptr<Transaction> TransactionManager::CreateTransferTransaction(
    const std::string& from,
    const std::string& to,
    const std::string& tokenHash,
    const std::string& amount) {
    
    auto tx = std::make_shared<Transaction>();
    
    // Set sender
    tx->sender = from;
    
    // Build transfer script
    tx->script = BuildTransferScript(tokenHash, from, to, amount);
    
    // Add signer
    Signer signer;
    signer.account = AddressToScriptHash(from);
    signer.scopes = WitnessScope::CalledByEntry;
    tx->signers.push_back(signer);
    
    // Set valid until block
    tx->validUntilBlock = GetCurrentBlockHeight() + 100;
    
    // Calculate fees
    SetOptimalFees(*tx);
    
    return tx;
}

std::shared_ptr<Transaction> TransactionManager::CreateContractTransaction(
    const std::string& contractHash,
    const std::string& method,
    const std::vector<std::string>& params,
    const std::string& sender) {
    
    auto tx = std::make_shared<Transaction>();
    
    // Set sender
    tx->sender = sender;
    
    // Build invocation script
    tx->script = BuildInvocationScript(contractHash, method, params);
    
    // Add signer
    Signer signer;
    signer.account = AddressToScriptHash(sender);
    signer.scopes = WitnessScope::CalledByEntry;
    tx->signers.push_back(signer);
    
    // Set valid until block
    tx->validUntilBlock = GetCurrentBlockHeight() + 100;
    
    // Calculate fees
    SetOptimalFees(*tx);
    
    return tx;
}

std::shared_ptr<Transaction> TransactionManager::CreateDeployTransaction(
    const std::vector<uint8_t>& nefFile,
    const std::string& manifest,
    const std::string& sender) {
    
    auto tx = std::make_shared<Transaction>();
    
    // Build deployment script
    ScriptBuilder sb;
    sb.Push(manifest);
    sb.Push(nefFile);
    sb.EmitSysCall("System.Contract.Deploy");
    tx->script = sb.Build();
    
    // Set sender
    tx->sender = sender;
    
    // Add signer
    Signer signer;
    signer.account = AddressToScriptHash(sender);
    signer.scopes = WitnessScope::CalledByEntry;
    tx->signers.push_back(signer);
    
    // Set valid until block
    tx->validUntilBlock = GetCurrentBlockHeight() + 100;
    
    // Calculate fees (deployment requires higher fees)
    tx->systemFee = 1000000000;  // 10 GAS for deployment
    tx->networkFee = CalculateNetworkFee(*tx);
    
    return tx;
}

std::shared_ptr<Transaction> TransactionManager::CreateMultiTransferTransaction(
    const std::vector<std::tuple<std::string, std::string, std::string, std::string>>& transfers) {
    
    auto tx = std::make_shared<Transaction>();
    
    // Build script for multiple transfers
    ScriptBuilder sb;
    for (const auto& [tokenHash, from, to, amount] : transfers) {
        auto transferScript = BuildTransferScript(tokenHash, from, to, amount);
        sb.script_.insert(sb.script_.end(), transferScript.begin(), transferScript.end());
    }
    tx->script = sb.Build();
    
    // Add signers for all unique senders
    std::set<std::string> uniqueSenders;
    for (const auto& [tokenHash, from, to, amount] : transfers) {
        uniqueSenders.insert(from);
    }
    
    for (const auto& sender : uniqueSenders) {
        Signer signer;
        signer.account = AddressToScriptHash(sender);
        signer.scopes = WitnessScope::CalledByEntry;
        tx->signers.push_back(signer);
    }
    
    // Set sender to first signer
    if (!uniqueSenders.empty()) {
        tx->sender = *uniqueSenders.begin();
    }
    
    // Set valid until block
    tx->validUntilBlock = GetCurrentBlockHeight() + 100;
    
    // Calculate fees
    SetOptimalFees(*tx);
    
    return tx;
}

uint64_t TransactionManager::EstimateGas(const Transaction& tx) {
    // Use RPC to estimate gas
    std::string script = BytesToHex(tx.script);
    auto result = rpcClient_->InvokeScript(script);
    
    if (result.contains("gasconsumed")) {
        return std::stoull(result["gasconsumed"].get<std::string>());
    }
    
    // Default estimation
    return 1000000;  // 0.01 GAS
}

void TransactionManager::SetOptimalFees(Transaction& tx) {
    // Calculate system fee
    tx.systemFee = EstimateGas(tx);
    
    // Calculate network fee
    tx.networkFee = tx.CalculateNetworkFee(*rpcClient_);
}

std::vector<uint8_t> TransactionManager::BuildTransferScript(
    const std::string& tokenHash,
    const std::string& from,
    const std::string& to,
    const std::string& amount) {
    
    ScriptBuilder sb;
    
    // Convert amount to integer
    uint64_t amountInt = std::stoull(amount);
    
    // Build transfer script
    sb.Push(amountInt);
    sb.Push(AddressToScriptHash(to));
    sb.Push(AddressToScriptHash(from));
    sb.Push(3);  // Number of parameters
    sb.EmitAppCall(tokenHash, "transfer");
    
    return sb.Build();
}

std::vector<uint8_t> TransactionManager::BuildInvocationScript(
    const std::string& contractHash,
    const std::string& method,
    const std::vector<std::string>& params) {
    
    ScriptBuilder sb;
    
    // Push parameters in reverse order
    for (auto it = params.rbegin(); it != params.rend(); ++it) {
        sb.Push(*it);
    }
    
    // Push parameter count
    sb.Push(static_cast<int64_t>(params.size()));
    
    // Call contract method
    sb.EmitAppCall(contractHash, method);
    
    return sb.Build();
}

std::string TransactionManager::SendTransaction(const Transaction& tx) {
    std::string rawTx = tx.SerializeToHex();
    return SendRawTransaction(rawTx);
}

std::string TransactionManager::SendRawTransaction(const std::string& rawTx) {
    return rpcClient_->SendRawTransaction(rawTx);
}

bool TransactionManager::WaitForTransaction(const std::string& txHash, std::chrono::seconds timeout) {
    auto start = std::chrono::steady_clock::now();
    
    while (std::chrono::steady_clock::now() - start < timeout) {
        try {
            auto height = GetTransactionHeight(txHash);
            if (height > 0) {
                return true;
            }
        } catch (...) {
            // Transaction not yet included
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return false;
}

nlohmann::json TransactionManager::GetTransactionResult(const std::string& txHash) {
    return rpcClient_->GetApplicationLog(txHash);
}

uint32_t TransactionManager::GetTransactionHeight(const std::string& txHash) {
    return rpcClient_->GetTransactionHeight(txHash);
}

std::vector<std::string> TransactionManager::SendBatchTransactions(const std::vector<Transaction>& transactions) {
    std::vector<std::string> txIds;
    
    for (const auto& tx : transactions) {
        try {
            std::string txId = SendTransaction(tx);
            txIds.push_back(txId);
        } catch (const std::exception& e) {
            // Log error and continue
            txIds.push_back("");
        }
    }
    
    return txIds;
}

std::shared_ptr<Transaction> TransactionManager::CreateClaimGasTransaction(const std::string& address) {
    // Create transaction to claim GAS
    auto tx = std::make_shared<Transaction>();
    
    // Build claim script
    ScriptBuilder sb;
    sb.Push(AddressToScriptHash(address));
    sb.Push(1);
    sb.EmitAppCall(TokenHash::NEO, "transfer");
    tx->script = sb.Build();
    
    // Set sender
    tx->sender = address;
    
    // Add signer
    Signer signer;
    signer.account = AddressToScriptHash(address);
    signer.scopes = WitnessScope::CalledByEntry;
    tx->signers.push_back(signer);
    
    // Set valid until block
    tx->validUntilBlock = GetCurrentBlockHeight() + 100;
    
    // Calculate fees
    SetOptimalFees(*tx);
    
    return tx;
}

std::shared_ptr<Transaction> TransactionManager::CreateVoteTransaction(const std::string& address, const std::string& candidate) {
    // Create voting transaction
    auto tx = std::make_shared<Transaction>();
    
    // Build vote script
    ScriptBuilder sb;
    sb.Push(HexToBytes(candidate));
    sb.Push(AddressToScriptHash(address));
    sb.Push(2);
    sb.EmitAppCall(TokenHash::NEO, "vote");
    tx->script = sb.Build();
    
    // Set sender and signer
    tx->sender = address;
    Signer signer;
    signer.account = AddressToScriptHash(address);
    signer.scopes = WitnessScope::CalledByEntry;
    tx->signers.push_back(signer);
    
    tx->validUntilBlock = GetCurrentBlockHeight() + 100;
    SetOptimalFees(*tx);
    
    return tx;
}

std::shared_ptr<Transaction> TransactionManager::CreateRegisterCandidateTransaction(const std::string& address) {
    // Create candidate registration transaction
    auto tx = std::make_shared<Transaction>();
    
    // Build registration script
    ScriptBuilder sb;
    sb.Push(AddressToScriptHash(address));
    sb.Push(1);
    sb.EmitAppCall(TokenHash::NEO, "registerCandidate");
    tx->script = sb.Build();
    
    // Set sender and signer
    tx->sender = address;
    Signer signer;
    signer.account = AddressToScriptHash(address);
    signer.scopes = WitnessScope::CalledByEntry;
    tx->signers.push_back(signer);
    
    tx->validUntilBlock = GetCurrentBlockHeight() + 100;
    
    // Registration requires 1000 GAS fee
    tx->systemFee = 100000000000;  // 1000 GAS
    tx->networkFee = tx->CalculateNetworkFee(*rpcClient_);
    
    return tx;
}

uint32_t TransactionManager::GetCurrentBlockHeight() {
    return rpcClient_->GetBlockCount();
}

std::string TransactionManager::AddressToScriptHash(const std::string& address) {
    auto scriptHash = ::AddressToScriptHash(address);
    return BytesToHex(scriptHash);
}

std::string TransactionManager::ScriptHashToAddress(const std::string& scriptHash) {
    auto scriptHashBytes = HexToBytes(scriptHash);
    return ::ScriptHashToAddress(scriptHashBytes);
}

// ScriptBuilder implementation
ScriptBuilder::ScriptBuilder() {
}

ScriptBuilder& ScriptBuilder::Push(int64_t value) {
    if (value == -1) {
        script_.push_back(0x4F);  // PUSHM1
    } else if (value == 0) {
        script_.push_back(0x10);  // PUSH0
    } else if (value >= 1 && value <= 16) {
        script_.push_back(0x51 + value - 1);  // PUSH1-PUSH16
    } else {
        // Push as bytes
        std::vector<uint8_t> bytes;
        for (int i = 0; i < 8; i++) {
            if (value == 0) break;
            bytes.push_back(value & 0xFF);
            value >>= 8;
        }
        
        if (bytes.size() <= 75) {
            script_.push_back(bytes.size());
            script_.insert(script_.end(), bytes.begin(), bytes.end());
        } else {
            script_.push_back(0x0C);  // PUSHDATA1
            script_.push_back(bytes.size());
            script_.insert(script_.end(), bytes.begin(), bytes.end());
        }
    }
    return *this;
}

ScriptBuilder& ScriptBuilder::Push(const std::string& value) {
    std::vector<uint8_t> bytes(value.begin(), value.end());
    return Push(bytes);
}

ScriptBuilder& ScriptBuilder::Push(const std::vector<uint8_t>& value) {
    if (value.size() <= 75) {
        script_.push_back(value.size());
        script_.insert(script_.end(), value.begin(), value.end());
    } else if (value.size() <= 255) {
        script_.push_back(0x0C);  // PUSHDATA1
        script_.push_back(value.size());
        script_.insert(script_.end(), value.begin(), value.end());
    } else if (value.size() <= 65535) {
        script_.push_back(0x0D);  // PUSHDATA2
        script_.push_back(value.size() & 0xFF);
        script_.push_back((value.size() >> 8) & 0xFF);
        script_.insert(script_.end(), value.begin(), value.end());
    } else {
        script_.push_back(0x0E);  // PUSHDATA4
        WriteFixed(script_, static_cast<uint32_t>(value.size()));
        script_.insert(script_.end(), value.begin(), value.end());
    }
    return *this;
}

ScriptBuilder& ScriptBuilder::PushNull() {
    script_.push_back(0x0B);  // PUSHNULL
    return *this;
}

ScriptBuilder& ScriptBuilder::PushTrue() {
    script_.push_back(0x11);  // PUSHTRUE
    return *this;
}

ScriptBuilder& ScriptBuilder::PushFalse() {
    script_.push_back(0x10);  // PUSHFALSE
    return *this;
}

ScriptBuilder& ScriptBuilder::Emit(uint8_t opcode) {
    script_.push_back(opcode);
    return *this;
}

ScriptBuilder& ScriptBuilder::EmitAppCall(const std::string& scriptHash, const std::string& method) {
    // Push method name
    Push(method);
    
    // Push script hash
    auto scriptHashBytes = HexToBytes(scriptHash);
    Push(scriptHashBytes);
    
    // CALLT opcode
    script_.push_back(0x41);  // SYSCALL
    
    // System.Contract.Call interop hash
    uint32_t hash = 0x627d5b52;  // Hash of "System.Contract.Call"
    WriteFixed(script_, hash);
    
    return *this;
}

ScriptBuilder& ScriptBuilder::EmitSysCall(const std::string& method) {
    script_.push_back(0x41);  // SYSCALL
    
    // Calculate interop service hash
    uint8_t hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const uint8_t*>(method.c_str()), method.size(), hash);
    
    // Use first 4 bytes of hash
    script_.insert(script_.end(), hash, hash + 4);
    
    return *this;
}

std::vector<uint8_t> ScriptBuilder::Build() {
    return script_;
}

// Helper function implementations
std::shared_ptr<Transaction> CreateTransfer(const std::string& from, const std::string& to, 
                                           uint64_t amount, const std::string& tokenHash) {
    // Create simple transfer transaction
    auto tx = std::make_shared<Transaction>();
    
    ScriptBuilder sb;
    sb.Push(static_cast<int64_t>(amount));
    sb.Push(AddressToScriptHash(to));
    sb.Push(AddressToScriptHash(from));
    sb.Push(3);
    sb.EmitAppCall(tokenHash == "NEO" ? TokenHash::NEO : TokenHash::GAS, "transfer");
    
    tx->script = sb.Build();
    tx->sender = from;
    
    Signer signer;
    signer.account = BytesToHex(AddressToScriptHash(from));
    signer.scopes = WitnessScope::CalledByEntry;
    tx->signers.push_back(signer);
    
    return tx;
}

std::shared_ptr<Transaction> ParseTransaction(const std::string& data) {
    // Try to parse as hex first
    try {
        return Transaction::DeserializeFromHex(data);
    } catch (...) {
        // Try to parse as JSON
        auto json = nlohmann::json::parse(data);
        return Transaction::FromJSON(json);
    }
}

std::pair<uint64_t, uint64_t> EstimateFees(const Transaction& tx, rpc::RpcClient& client) {
    uint64_t systemFee = tx.CalculateSystemFee(client);
    uint64_t networkFee = tx.CalculateNetworkFee(client);
    return {systemFee, networkFee};
}

void SignWithMultipleSigners(Transaction& tx, const std::vector<std::string>& privateKeys) {
    for (const auto& privateKey : privateKeys) {
        tx.Sign(privateKey);
    }
}

bool VerifyTransactionSignatures(const Transaction& tx) {
    return tx.Verify();
}

} // namespace neo::sdk::transaction