#include <neo/sdk/tx/transaction_builder.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/signer.h>
#include <neo/ledger/witness.h>
#include <neo/ledger/transaction_attribute.h>
#include <neo/smartcontract/contract_parameter.h>
#include <neo/vm/script_builder.h>
#include <neo/logging/logger.h>
#include <neo/io/binary_writer.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>

namespace neo::sdk::tx {

// Asset script hashes (MainNet)
static const io::UInt160 NEO_TOKEN = io::UInt160::Parse("0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5");
static const io::UInt160 GAS_TOKEN = io::UInt160::Parse("0xd2a4cff31913016155e38e474a2c06d08be276cf");

class TransactionBuilder::Impl {
public:
    std::unique_ptr<ledger::Transaction> transaction;
    std::vector<uint8_t> script;
    
    Impl() {
        transaction = std::make_unique<ledger::Transaction>();
        transaction->SetVersion(0);
        transaction->SetNonce(static_cast<uint32_t>(std::rand()));
    }
    
    void AppendScript(const std::vector<uint8_t>& newScript) {
        script.insert(script.end(), newScript.begin(), newScript.end());
    }
};

TransactionBuilder::TransactionBuilder() : impl_(std::make_unique<Impl>()) {}
TransactionBuilder::~TransactionBuilder() = default;

TransactionBuilder& TransactionBuilder::SetSender(const io::UInt160& sender) {
    // Add sender as first signer with fee-only scope
    ledger::Signer signer;
    signer.account_ = sender;
    signer.scopes_ = static_cast<ledger::WitnessScope>(0x01);  // FeeOnly scope
    
    auto signers = impl_->transaction->GetSigners();
    if (signers.empty()) {
        signers.push_back(signer);
    } else {
        signers[0] = signer;  // Replace first signer
    }
    impl_->transaction->SetSigners(signers);
    
    return *this;
}

TransactionBuilder& TransactionBuilder::SetSystemFee(uint64_t fee) {
    impl_->transaction->SetSystemFee(fee);
    return *this;
}

TransactionBuilder& TransactionBuilder::SetNetworkFee(uint64_t fee) {
    impl_->transaction->SetNetworkFee(fee);
    return *this;
}

TransactionBuilder& TransactionBuilder::SetValidUntilBlock(uint32_t block) {
    impl_->transaction->SetValidUntilBlock(block);
    return *this;
}

TransactionBuilder& TransactionBuilder::AddAttribute(const ledger::TransactionAttribute& attr) {
    auto attributes = impl_->transaction->GetAttributes();
    // Convert SDK attribute to core attribute
    ledger::TransactionAttribute coreAttr;
    coreAttr.usage = attr.usage;
    coreAttr.data = attr.data;
    attributes.push_back(coreAttr);
    impl_->transaction->SetAttributes(attributes);
    return *this;
}

TransactionBuilder& TransactionBuilder::AddWitness(const ledger::Witness& witness) {
    auto witnesses = impl_->transaction->GetWitnesses();
    witnesses.push_back(witness);
    impl_->transaction->SetWitnesses(witnesses);
    return *this;
}

TransactionBuilder& TransactionBuilder::AddSigner(const ledger::Signer& signer) {
    auto signers = impl_->transaction->GetSigners();
    signers.push_back(signer);
    impl_->transaction->SetSigners(signers);
    return *this;
}

TransactionBuilder& TransactionBuilder::SetScript(const std::vector<uint8_t>& script) {
    impl_->script = script;
    return *this;
}

TransactionBuilder& TransactionBuilder::InvokeContract(
    const io::UInt160& scriptHash,
    const std::string& method,
    const std::vector<smartcontract::ContractParameter>& params) {
    
    neo::vm::ScriptBuilder sb;
    
    // Build parameters in reverse order
    for (auto it = params.rbegin(); it != params.rend(); ++it) {
        const auto& param = *it;
        switch (param.type) {
            case smartcontract::ContractParameter::INTEGER:
                sb.EmitPush(*reinterpret_cast<const int64_t*>(param.value.data()));
                break;
            case smartcontract::ContractParameter::BOOLEAN:
                sb.EmitPush(param.value[0] != 0);
                break;
            case smartcontract::ContractParameter::STRING:
                sb.EmitPush(std::string(param.value.begin(), param.value.end()));
                break;
            case smartcontract::ContractParameter::HASH160:
                sb.EmitPush(io::UInt160::FromBytes(param.value));
                break;
            case smartcontract::ContractParameter::HASH256:
                sb.EmitPush(io::UInt256::FromBytes(param.value));
                break;
            case smartcontract::ContractParameter::BYTE_ARRAY:
                sb.EmitPush(param.value);
                break;
            case smartcontract::ContractParameter::VOID:
                sb.Emit(neo::vm::OpCode::PUSHNULL);
                break;
            default:
                NEO_LOG_ERROR(std::string("Unsupported parameter type: ") + param.type);
                break;
        }
    }
    
    // Emit parameter count
    sb.EmitPush(static_cast<int64_t>(params.size()));
    
    // Emit method name
    sb.EmitPush(method);
    
    // Emit contract call (use SYSCALL with contract hash)
    sb.Emit(neo::vm::OpCode::SYSCALL);
    sb.EmitPush(scriptHash.ToArray());
    
    impl_->AppendScript(sb.ToArray());
    return *this;
}

TransactionBuilder& TransactionBuilder::Transfer(
    const io::UInt160& from,
    const io::UInt160& to,
    const std::string& asset,
    uint64_t amount) {
    
    // Determine token contract
    io::UInt160 tokenHash;
    if (asset == "NEO") {
        tokenHash = NEO_TOKEN;
    } else if (asset == "GAS") {
        tokenHash = GAS_TOKEN;
    } else {
        // Assume it's a script hash
        tokenHash = io::UInt160::Parse(asset);
    }
    
    // Build transfer invocation
    return InvokeContract(tokenHash, "transfer", {
        smartcontract::ContractParameter::FromHash160(from),
        smartcontract::ContractParameter::FromHash160(to),
        smartcontract::ContractParameter::FromInteger(amount),
        smartcontract::ContractParameter::Null()  // No data
    });
}

TransactionBuilder& TransactionBuilder::TransferWithData(
    const io::UInt160& from,
    const io::UInt160& to,
    const std::string& asset,
    uint64_t amount,
    const std::vector<uint8_t>& data) {
    
    // Determine token contract
    io::UInt160 tokenHash;
    if (asset == "NEO") {
        tokenHash = NEO_TOKEN;
    } else if (asset == "GAS") {
        tokenHash = GAS_TOKEN;
    } else {
        tokenHash = io::UInt160::Parse(asset);
    }
    
    // Build transfer invocation with data
    return InvokeContract(tokenHash, "transfer", {
        smartcontract::ContractParameter::FromHash160(from),
        smartcontract::ContractParameter::FromHash160(to),
        smartcontract::ContractParameter::FromInteger(amount),
        smartcontract::ContractParameter::FromByteArray(data)
    });
}

TransactionBuilder& TransactionBuilder::AddCosigner(
    const io::UInt160& account,
    uint8_t scopes) {
    
    ledger::Signer signer;
    signer.account_ = account;
    signer.scopes_ = static_cast<ledger::WitnessScope>(scopes);
    return AddSigner(signer);
}

TransactionBuilder& TransactionBuilder::SetNonce(uint32_t nonce) {
    impl_->transaction->SetNonce(nonce);
    return *this;
}

std::shared_ptr<ledger::Transaction> TransactionBuilder::Build() {
    // Set the script
    impl_->transaction->SetScript(impl_->script);
    
    // Validate transaction
    if (impl_->transaction->GetSigners().empty()) {
        throw std::runtime_error("Transaction must have at least one signer");
    }
    
    if (impl_->script.empty()) {
        throw std::runtime_error("Transaction script cannot be empty");
    }
    
    // Return a copy of the transaction
    return std::make_shared<ledger::Transaction>(*impl_->transaction);
}

std::shared_ptr<ledger::Transaction> TransactionBuilder::BuildAndSign(wallet::Wallet& wallet) {
    auto tx = Build();
    
    // Sign with wallet
    if (!wallet.SignTransaction(tx)) {
        throw std::runtime_error("Failed to sign transaction");
    }
    
    return tx;
}

TransactionBuilder& TransactionBuilder::Reset() {
    impl_ = std::make_unique<Impl>();
    return *this;
}

uint64_t TransactionBuilder::CalculateNetworkFee() const {
    // Calculate based on transaction size and witness complexity
    // Base fee + size * fee per byte + signature verification cost
    
    // Estimate transaction size
    auto tx = std::make_shared<ledger::Transaction>(*impl_->transaction);
    tx->SetScript(impl_->script);
    auto size = tx->ToArray().size();
    
    // Network fee calculation (simplified)
    uint64_t baseFee = 100000;  // 0.001 GAS base fee
    uint64_t feePerByte = 1000;  // 0.00001 GAS per byte
    uint64_t signatureCost = 1000000;  // 0.01 GAS per signature
    
    auto signerCount = tx->GetSigners().size();
    
    return baseFee + (size * feePerByte) + (signerCount * signatureCost);
}

uint64_t TransactionBuilder::EstimateSystemFee() const {
    // This would require RPC connection to invoke the script
    // and get the actual gas consumption
    NEO_LOG_ERROR("System fee estimation requires RPC connection. Using default.");
    return 1000000;  // Default 0.01 GAS
}

} // namespace neo::sdk::tx