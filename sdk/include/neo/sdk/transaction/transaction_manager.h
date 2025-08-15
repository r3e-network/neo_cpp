#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include "../types/common.h"
#include "../rpc/rpc_client.h"

namespace neo {
namespace sdk {
namespace transaction {

// Transaction attribute types
enum class TransactionAttributeType : uint8_t {
    HighPriority = 0x01,
    OracleResponse = 0x11,
    NotValidBefore = 0x20,
    Conflicts = 0x21
};

// Witness scope flags
enum class WitnessScope : uint8_t {
    None = 0x00,
    CalledByEntry = 0x01,
    CustomContracts = 0x10,
    CustomGroups = 0x20,
    WitnessRules = 0x40,
    Global = 0x80
};

// Signer structure
struct Signer {
    std::string account;  // Script hash of the signer
    WitnessScope scopes = WitnessScope::CalledByEntry;
    std::vector<std::string> allowedContracts;
    std::vector<std::string> allowedGroups;
    std::vector<std::string> rules;
};

// Witness structure
struct Witness {
    std::vector<uint8_t> invocationScript;
    std::vector<uint8_t> verificationScript;
};

// Transaction attribute
struct TransactionAttribute {
    TransactionAttributeType type;
    std::vector<uint8_t> data;
};

// Main transaction class
class Transaction {
public:
    // Constructor
    Transaction();
    
    // Transaction properties
    uint8_t version = 0;
    uint32_t nonce;
    std::string sender;  // Fee payer
    uint64_t systemFee = 0;
    uint64_t networkFee = 0;
    uint32_t validUntilBlock;
    std::vector<Signer> signers;
    std::vector<TransactionAttribute> attributes;
    std::vector<uint8_t> script;
    std::vector<Witness> witnesses;
    
    // Calculate fees
    uint64_t CalculateNetworkFee(rpc::RpcClient& client);
    uint64_t CalculateSystemFee(rpc::RpcClient& client);
    
    // Get transaction hash
    std::string GetHash() const;
    
    // Serialize transaction
    std::vector<uint8_t> Serialize() const;
    std::string SerializeToHex() const;
    
    // Deserialize transaction
    static std::shared_ptr<Transaction> Deserialize(const std::vector<uint8_t>& data);
    static std::shared_ptr<Transaction> DeserializeFromHex(const std::string& hex);
    
    // Sign transaction
    void AddWitness(const Witness& witness);
    void Sign(const std::string& privateKey);
    
    // Verify transaction
    bool Verify() const;
    
    // Convert to JSON
    nlohmann::json ToJSON() const;
    static std::shared_ptr<Transaction> FromJSON(const nlohmann::json& json);
};

// Transaction manager for advanced operations
class TransactionManager {
public:
    TransactionManager(std::shared_ptr<rpc::RpcClient> rpcClient);
    
    // Create transactions
    std::shared_ptr<Transaction> CreateTransferTransaction(
        const std::string& from,
        const std::string& to,
        const std::string& tokenHash,
        const std::string& amount
    );
    
    std::shared_ptr<Transaction> CreateContractTransaction(
        const std::string& contractHash,
        const std::string& method,
        const std::vector<std::string>& params,
        const std::string& sender
    );
    
    std::shared_ptr<Transaction> CreateDeployTransaction(
        const std::vector<uint8_t>& nefFile,
        const std::string& manifest,
        const std::string& sender
    );
    
    std::shared_ptr<Transaction> CreateMultiTransferTransaction(
        const std::vector<std::tuple<std::string, std::string, std::string, std::string>>& transfers
        // Vector of (tokenHash, from, to, amount)
    );
    
    // Fee calculation
    uint64_t EstimateGas(const Transaction& tx);
    void SetOptimalFees(Transaction& tx);
    
    // Transaction building helpers
    std::vector<uint8_t> BuildTransferScript(
        const std::string& tokenHash,
        const std::string& from,
        const std::string& to,
        const std::string& amount
    );
    
    std::vector<uint8_t> BuildInvocationScript(
        const std::string& contractHash,
        const std::string& method,
        const std::vector<std::string>& params
    );
    
    // Send transaction
    std::string SendTransaction(const Transaction& tx);
    std::string SendRawTransaction(const std::string& rawTx);
    
    // Transaction tracking
    bool WaitForTransaction(const std::string& txHash, std::chrono::seconds timeout = std::chrono::seconds(60));
    nlohmann::json GetTransactionResult(const std::string& txHash);
    uint32_t GetTransactionHeight(const std::string& txHash);
    
    // Batch transactions
    std::vector<std::string> SendBatchTransactions(const std::vector<Transaction>& transactions);
    
    // Advanced features
    std::shared_ptr<Transaction> CreateClaimGasTransaction(const std::string& address);
    std::shared_ptr<Transaction> CreateVoteTransaction(const std::string& address, const std::string& candidate);
    std::shared_ptr<Transaction> CreateRegisterCandidateTransaction(const std::string& address);
    
private:
    std::shared_ptr<rpc::RpcClient> rpcClient_;
    
    // Helper methods
    uint32_t GetCurrentBlockHeight();
    std::string AddressToScriptHash(const std::string& address);
    std::string ScriptHashToAddress(const std::string& scriptHash);
};

// Script builder for creating transaction scripts
class ScriptBuilder {
public:
    ScriptBuilder();
    
    // Push data onto stack
    ScriptBuilder& Push(int64_t value);
    ScriptBuilder& Push(const std::string& value);
    ScriptBuilder& Push(const std::vector<uint8_t>& value);
    ScriptBuilder& PushNull();
    ScriptBuilder& PushTrue();
    ScriptBuilder& PushFalse();
    
    // Operations
    ScriptBuilder& Emit(uint8_t opcode);
    ScriptBuilder& EmitAppCall(const std::string& scriptHash, const std::string& method);
    ScriptBuilder& EmitSysCall(const std::string& method);
    
    // Build script
    std::vector<uint8_t> Build();
    
private:
    std::vector<uint8_t> script_;
};

// Helper functions

// Create simple transfer transaction
std::shared_ptr<Transaction> CreateTransfer(
    const std::string& from,
    const std::string& to,
    uint64_t amount,
    const std::string& tokenHash = "NEO"
);

// Parse transaction from various formats
std::shared_ptr<Transaction> ParseTransaction(const std::string& data);

// Estimate fees for transaction
std::pair<uint64_t, uint64_t> EstimateFees(
    const Transaction& tx,
    rpc::RpcClient& client
);

// Sign transaction with multiple signers
void SignWithMultipleSigners(
    Transaction& tx,
    const std::vector<std::string>& privateKeys
);

// Verify transaction signatures
bool VerifyTransactionSignatures(const Transaction& tx);

// Constants for common token hashes
namespace TokenHash {
    const std::string NEO = "0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5";
    const std::string GAS = "0xd2a4cff31913016155e38e474a2c06d08be276cf";
}

} // namespace transaction
} // namespace sdk
} // namespace neo