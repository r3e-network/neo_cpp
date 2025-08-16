// nep17_token.cpp - Implementation of NEP-17 token standard for Neo blockchain

#include <neo/sdk/contract/nep17_token.h>
#include <neo/sdk/contract/contract_invoker.h>
#include <neo/sdk/utils/serializer.h>
#include <neo/sdk/crypto/crypto.h>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace neo::sdk::contract {

// Constructor
NEP17Token::NEP17Token(const core::UInt160& contractHash, std::shared_ptr<rpc::RpcClient> client)
    : contractHash_(contractHash), client_(client) {
    invoker_ = std::make_shared<ContractInvoker>(contractHash, client);
}

// Get token symbol
std::string NEP17Token::Symbol() {
    if (!cachedInfo_) {
        RefreshTokenInfo();
    }
    return cachedInfo_->symbol;
}

// Get token decimals
uint8_t NEP17Token::Decimals() {
    if (!cachedInfo_) {
        RefreshTokenInfo();
    }
    return cachedInfo_->decimals;
}

// Get total supply
uint64_t NEP17Token::TotalSupply() {
    if (!cachedInfo_) {
        RefreshTokenInfo();
    }
    return cachedInfo_->totalSupply;
}

// Get token name
std::string NEP17Token::Name() {
    if (!cachedInfo_) {
        RefreshTokenInfo();
    }
    return cachedInfo_->name;
}

// Get complete token information
TokenInfo NEP17Token::GetTokenInfo() {
    if (!cachedInfo_) {
        RefreshTokenInfo();
    }
    return *cachedInfo_;
}

// Get balance of an account
uint64_t NEP17Token::BalanceOf(const core::UInt160& account) {
    // Prepare parameters
    std::vector<ContractParameter> params;
    params.push_back(ContractParameter::Hash160(account));
    
    // Invoke balanceOf method
    auto result = invoker_->InvokeFunction("balanceOf", params);
    
    // Parse result
    if (result && result->type == ContractParameterType::Integer) {
        return result->value.integerValue;
    }
    
    return 0;
}

// Get balance of an address
uint64_t NEP17Token::BalanceOf(const std::string& address) {
    // Convert address to script hash
    auto scriptHash = crypto::AddressToScriptHash(address);
    return BalanceOf(scriptHash);
}

// Transfer tokens
core::UInt256 NEP17Token::Transfer(
    const core::UInt160& from,
    const core::UInt160& to,
    uint64_t amount,
    wallet::Wallet& wallet,
    const std::string& data) {
    
    // Prepare parameters
    std::vector<ContractParameter> params;
    params.push_back(ContractParameter::Hash160(from));
    params.push_back(ContractParameter::Hash160(to));
    params.push_back(ContractParameter::Integer(amount));
    params.push_back(ContractParameter::String(data));
    
    // Create and sign transaction
    auto tx = invoker_->CreateTransaction("transfer", params, from);
    
    // Sign with wallet
    wallet.SignTransaction(tx);
    
    // Send transaction
    auto txId = client_->SendRawTransaction(tx.ToHexString());
    
    return core::UInt256::Parse(txId);
}

// Transfer tokens using addresses
core::UInt256 NEP17Token::Transfer(
    const std::string& fromAddress,
    const std::string& toAddress,
    uint64_t amount,
    wallet::Wallet& wallet,
    const std::string& data) {
    
    auto from = crypto::AddressToScriptHash(fromAddress);
    auto to = crypto::AddressToScriptHash(toAddress);
    
    return Transfer(from, to, amount, wallet, data);
}

// Multi-transfer to multiple recipients
core::UInt256 NEP17Token::MultiTransfer(
    const core::UInt160& from,
    const std::vector<std::pair<core::UInt160, uint64_t>>& recipients,
    wallet::Wallet& wallet) {
    
    // Build script for multiple transfers
    utils::ScriptBuilder sb;
    
    for (const auto& [recipient, amount] : recipients) {
        // Push parameters for each transfer
        sb.Push(std::string(""));  // Empty data
        sb.Push(amount);
        sb.Push(recipient.ToBytes());
        sb.Push(from.ToBytes());
        sb.Push(4);  // Number of parameters
        sb.Push(contractHash_.ToBytes());
        sb.Push("transfer");
        sb.EmitSysCall("System.Contract.Call");
        
        // Check result
        sb.EmitOpCode(OpCode::ASSERT);
    }
    
    // Create transaction with the script
    tx::Transaction tx;
    tx.script = sb.ToArray();
    tx.signers.push_back({from, tx::WitnessScope::CalledByEntry});
    tx.validUntilBlock = client_->GetBlockCount() + 100;
    
    // Calculate fees
    auto systemFee = client_->CalculateSystemFee(tx.ToHexString());
    auto networkFee = client_->CalculateNetworkFee(tx.ToHexString());
    tx.systemFee = systemFee;
    tx.networkFee = networkFee;
    
    // Sign and send
    wallet.SignTransaction(tx);
    auto txId = client_->SendRawTransaction(tx.ToHexString());
    
    return core::UInt256::Parse(txId);
}

// Get transfer history for an account
std::vector<TransferEvent> NEP17Token::GetTransferHistory(
    const core::UInt160& account,
    uint32_t limit) {
    
    std::vector<TransferEvent> events;
    
    // Query NEP-17 transfers from RPC
    auto address = crypto::ScriptHashToAddress(account);
    auto transfers = client_->GetNep17Transfers(address);
    
    // Parse transfer events
    if (transfers.contains("sent")) {
        for (const auto& transfer : transfers["sent"]) {
            if (transfer["assethash"] == contractHash_.ToString()) {
                TransferEvent event;
                event.from = account;
                event.to = core::UInt160::Parse(transfer["transferaddress"].get<std::string>());
                event.amount = std::stoull(transfer["amount"].get<std::string>());
                event.txId = core::UInt256::Parse(transfer["txhash"].get<std::string>());
                event.blockIndex = transfer["blockindex"];
                event.timestamp = transfer["timestamp"];
                events.push_back(event);
                
                if (events.size() >= limit) break;
            }
        }
    }
    
    if (transfers.contains("received") && events.size() < limit) {
        for (const auto& transfer : transfers["received"]) {
            if (transfer["assethash"] == contractHash_.ToString()) {
                TransferEvent event;
                event.from = core::UInt160::Parse(transfer["transferaddress"].get<std::string>());
                event.to = account;
                event.amount = std::stoull(transfer["amount"].get<std::string>());
                event.txId = core::UInt256::Parse(transfer["txhash"].get<std::string>());
                event.blockIndex = transfer["blockindex"];
                event.timestamp = transfer["timestamp"];
                events.push_back(event);
                
                if (events.size() >= limit) break;
            }
        }
    }
    
    // Sort by timestamp (newest first)
    std::sort(events.begin(), events.end(), 
        [](const TransferEvent& a, const TransferEvent& b) {
            return a.timestamp > b.timestamp;
        });
    
    return events;
}

// Calculate transfer fee
std::pair<uint64_t, uint64_t> NEP17Token::CalculateTransferFee(
    const core::UInt160& from,
    const core::UInt160& to,
    uint64_t amount) {
    
    // Create test transaction
    std::vector<ContractParameter> params;
    params.push_back(ContractParameter::Hash160(from));
    params.push_back(ContractParameter::Hash160(to));
    params.push_back(ContractParameter::Integer(amount));
    params.push_back(ContractParameter::String(""));
    
    auto tx = invoker_->CreateTransaction("transfer", params, from);
    
    // Calculate fees
    auto systemFee = client_->CalculateSystemFee(tx.ToHexString());
    auto networkFee = client_->CalculateNetworkFee(tx.ToHexString());
    
    return {systemFee, networkFee};
}

// Refresh cached token information
void NEP17Token::RefreshTokenInfo() const {
    TokenInfo info;
    
    // Get symbol
    auto symbolResult = invoker_->InvokeFunction("symbol", {});
    if (symbolResult && symbolResult->type == ContractParameterType::String) {
        info.symbol = symbolResult->value.stringValue;
    }
    
    // Get decimals
    auto decimalsResult = invoker_->InvokeFunction("decimals", {});
    if (decimalsResult && decimalsResult->type == ContractParameterType::Integer) {
        info.decimals = static_cast<uint8_t>(decimalsResult->value.integerValue);
    }
    
    // Get total supply
    auto totalSupplyResult = invoker_->InvokeFunction("totalSupply", {});
    if (totalSupplyResult && totalSupplyResult->type == ContractParameterType::Integer) {
        info.totalSupply = totalSupplyResult->value.integerValue;
    }
    
    // Try to get name (optional)
    try {
        auto nameResult = invoker_->InvokeFunction("name", {});
        if (nameResult && nameResult->type == ContractParameterType::String) {
            info.name = nameResult->value.stringValue;
        }
    } catch (...) {
        // Name method might not exist, use symbol as fallback
        info.name = info.symbol;
    }
    
    cachedInfo_ = info;
}

// Static methods for well-known tokens
core::UInt160 NEP17Token::WellKnownTokens::NEO() {
    // NEO native contract hash
    return core::UInt160::Parse("0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5");
}

core::UInt160 NEP17Token::WellKnownTokens::GAS() {
    // GAS native contract hash
    return core::UInt160::Parse("0xd2a4cff31913016155e38e474a2c06d08be276cf");
}

core::UInt160 NEP17Token::WellKnownTokens::GetBySymbol(const std::string& symbol) {
    if (symbol == "NEO") {
        return NEO();
    } else if (symbol == "GAS") {
        return GAS();
    }
    throw std::runtime_error("Unknown token symbol: " + symbol);
}

// Convert amount with decimals
double NEP17Token::ToDecimalAmount(uint64_t amount, uint8_t decimals) {
    return static_cast<double>(amount) / std::pow(10, decimals);
}

// Convert from decimal amount
uint64_t NEP17Token::FromDecimalAmount(double decimalAmount, uint8_t decimals) {
    return static_cast<uint64_t>(decimalAmount * std::pow(10, decimals));
}

// BigInteger implementation
BigInteger::BigInteger(uint64_t value) {
    // Store as little-endian bytes
    for (int i = 0; i < 8; i++) {
        if (value == 0 && !data_.empty()) break;
        data_.push_back(value & 0xFF);
        value >>= 8;
    }
    if (data_.empty()) {
        data_.push_back(0);
    }
}

BigInteger::BigInteger(const std::string& value) {
    // Parse from string
    if (value.empty() || value == "0") {
        data_.push_back(0);
        return;
    }
    
    // Simple parsing for positive integers
    uint64_t num = 0;
    for (char c : value) {
        if (c >= '0' && c <= '9') {
            num = num * 10 + (c - '0');
        }
    }
    
    // Convert to bytes
    for (int i = 0; i < 8; i++) {
        if (num == 0 && !data_.empty()) break;
        data_.push_back(num & 0xFF);
        num >>= 8;
    }
    if (data_.empty()) {
        data_.push_back(0);
    }
}

std::string BigInteger::ToString() const {
    // Convert to uint64
    uint64_t value = ToUint64();
    return std::to_string(value);
}

uint64_t BigInteger::ToUint64() const {
    uint64_t value = 0;
    for (size_t i = 0; i < data_.size() && i < 8; i++) {
        value |= static_cast<uint64_t>(data_[i]) << (i * 8);
    }
    return value;
}

BigInteger BigInteger::operator+(const BigInteger& other) const {
    // Simplified addition
    return BigInteger(ToUint64() + other.ToUint64());
}

BigInteger BigInteger::operator-(const BigInteger& other) const {
    uint64_t a = ToUint64();
    uint64_t b = other.ToUint64();
    if (a >= b) {
        return BigInteger(a - b);
    }
    // Handle negative values
    return BigInteger(0);
}

BigInteger BigInteger::operator*(const BigInteger& other) const {
    return BigInteger(ToUint64() * other.ToUint64());
}

BigInteger BigInteger::operator/(const BigInteger& other) const {
    uint64_t divisor = other.ToUint64();
    if (divisor == 0) {
        throw std::runtime_error("Division by zero");
    }
    return BigInteger(ToUint64() / divisor);
}

bool BigInteger::operator==(const BigInteger& other) const {
    return data_ == other.data_ && negative_ == other.negative_;
}

bool BigInteger::operator!=(const BigInteger& other) const {
    return !(*this == other);
}

bool BigInteger::operator<(const BigInteger& other) const {
    if (negative_ != other.negative_) {
        return negative_;
    }
    return ToUint64() < other.ToUint64();
}

bool BigInteger::operator>(const BigInteger& other) const {
    return other < *this;
}

bool BigInteger::operator<=(const BigInteger& other) const {
    return !(*this > other);
}

bool BigInteger::operator>=(const BigInteger& other) const {
    return !(*this < other);
}

} // namespace neo::sdk::contract