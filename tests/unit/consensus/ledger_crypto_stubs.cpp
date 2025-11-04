#include <neo/cryptography/base58.h>
#include <neo/cryptography/hash.h>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/ledger/signer.h>
#include <neo/ledger/transaction_attribute.h>
#include <neo/ledger/transaction_verification_context.h>
#include <neo/ledger/witness.h>
#include <neo/ledger/witness_rule.h>

#include <string>
#include <utility>
#include <vector>

namespace neo::cryptography
{
io::UInt256 Hash::Hash256(const io::ByteSpan&) { return io::UInt256::Zero(); }

io::UInt160 Hash::Hash160(const io::ByteSpan&) { return io::UInt160(); }

namespace ecc
{
ECPoint::ECPoint() : curveName_("secp256r1"), isInfinity_(false), x_(), y_() {}

ECPoint::~ECPoint() = default;

ECPoint::ECPoint(const std::string& curveName) : curveName_(curveName), isInfinity_(false), x_(), y_() {}

const std::string& ECPoint::GetCurveName() const { return curveName_; }

void ECPoint::SetCurveName(const std::string& curveName) { curveName_ = curveName; }

bool ECPoint::IsInfinity() const { return isInfinity_; }

void ECPoint::SetInfinity(bool isInfinity) { isInfinity_ = isInfinity; }

const io::UInt256& ECPoint::GetX() const { return x_; }

void ECPoint::SetX(const io::UInt256& x) { x_ = x; }

const io::UInt256& ECPoint::GetY() const { return y_; }

void ECPoint::SetY(const io::UInt256& y) { y_ = y; }

io::ByteVector ECPoint::ToBytes(bool) const { return io::ByteVector(); }

io::ByteVector ECPoint::ToArray() const { return io::ByteVector(); }

std::string ECPoint::ToHex(bool) const { return std::string(); }

ECPoint ECPoint::FromBytes(const io::ByteSpan&, const std::string& curveName)
{
    ECPoint point(curveName);
    point.SetInfinity(true);
    return point;
}

ECPoint ECPoint::FromHex(const std::string&, const std::string& curveName) { return ECPoint(curveName); }

ECPoint ECPoint::Infinity(const std::string& curveName)
{
    ECPoint point(curveName);
    point.SetInfinity(true);
    return point;
}

bool ECPoint::operator==(const ECPoint& other) const
{
    return curveName_ == other.curveName_ && isInfinity_ == other.isInfinity_;
}

bool ECPoint::operator!=(const ECPoint& other) const { return !(*this == other); }

bool ECPoint::operator<(const ECPoint& other) const { return curveName_ < other.curveName_; }

bool ECPoint::operator>(const ECPoint& other) const { return curveName_ > other.curveName_; }

bool ECPoint::operator<=(const ECPoint& other) const { return !(*this > other); }

bool ECPoint::operator>=(const ECPoint& other) const { return !(*this < other); }

ECPoint ECPoint::Add(const ECPoint& other) const
{
    (void)other;
    return *this;
}

ECPoint ECPoint::Multiply(const io::UInt256&) const { return *this; }

ECPoint ECPoint::Negate() const { return *this; }

void ECPoint::Serialize(io::BinaryWriter&) const {}

void ECPoint::Deserialize(io::BinaryReader&) {}
}  // namespace ecc

std::string Base58::EncodeCheck(const std::vector<uint8_t>&) { return {}; }

std::vector<uint8_t> Base58::DecodeCheck(const std::string&) { return {}; }
}  // namespace neo::cryptography

namespace neo::ledger
{
TransactionAttribute::TransactionAttribute() : usage_(Usage::Remark), data_() {}

TransactionAttribute::TransactionAttribute(Usage usage, const io::ByteVector& data)
    : usage_(usage), data_(data)
{
}

TransactionAttribute::Usage TransactionAttribute::GetUsage() const { return usage_; }

void TransactionAttribute::SetUsage(Usage usage) { usage_ = usage; }

const io::ByteVector& TransactionAttribute::GetData() const { return data_; }

void TransactionAttribute::SetData(const io::ByteVector& data) { data_ = data; }

void TransactionAttribute::Serialize(io::BinaryWriter& writer) const
{
    writer.Write(static_cast<uint8_t>(usage_));
    writer.WriteVarBytes(data_);
}

void TransactionAttribute::Deserialize(io::BinaryReader& reader)
{
    usage_ = static_cast<Usage>(reader.ReadUInt8());
    data_ = reader.ReadVarBytes();
}

void TransactionAttribute::SerializeJson(io::JsonWriter&) const {}

void TransactionAttribute::DeserializeJson(const io::JsonReader&) {}

bool TransactionAttribute::operator==(const TransactionAttribute& other) const
{
    return usage_ == other.usage_ && data_ == other.data_;
}

bool TransactionAttribute::operator!=(const TransactionAttribute& other) const { return !(*this == other); }

Witness::Witness() = default;

Witness::Witness(const io::ByteVector& invocationScript, const io::ByteVector& verificationScript)
    : invocationScript_(invocationScript), verificationScript_(verificationScript)
{
}

const io::ByteVector& Witness::GetInvocationScript() const { return invocationScript_; }

void Witness::SetInvocationScript(const io::ByteVector& invocationScript) { invocationScript_ = invocationScript; }

const io::ByteVector& Witness::GetVerificationScript() const { return verificationScript_; }

void Witness::SetVerificationScript(const io::ByteVector& verificationScript)
{
    verificationScript_ = verificationScript;
}

io::UInt160 Witness::GetScriptHash() const { return io::UInt160(); }

int Witness::GetSize() const
{
    return static_cast<int>(invocationScript_.Size() + verificationScript_.Size() + 2);
}

void Witness::Serialize(io::BinaryWriter& writer) const
{
    writer.WriteVarBytes(invocationScript_.AsSpan());
    writer.WriteVarBytes(verificationScript_.AsSpan());
}

void Witness::Deserialize(io::BinaryReader& reader)
{
    invocationScript_ = reader.ReadVarBytes();
    verificationScript_ = reader.ReadVarBytes();
}

void Witness::SerializeJson(io::JsonWriter&) const {}

void Witness::DeserializeJson(const io::JsonReader&) {}

bool Witness::operator==(const Witness& other) const
{
    return invocationScript_ == other.invocationScript_ && verificationScript_ == other.verificationScript_;
}

bool Witness::operator!=(const Witness& other) const { return !(*this == other); }

WitnessRule::WitnessRule() : action_(WitnessRuleAction::Deny), condition_(nullptr) {}

WitnessRule::WitnessRule(WitnessRuleAction action, std::shared_ptr<WitnessCondition> condition)
    : action_(action), condition_(std::move(condition))
{
}

void WitnessRule::Serialize(io::BinaryWriter& writer) const { writer.Write(static_cast<uint8_t>(action_)); }

void WitnessRule::Deserialize(io::BinaryReader& reader)
{
    action_ = static_cast<WitnessRuleAction>(reader.ReadUInt8());
    condition_.reset();
}

void WitnessRule::SerializeJson(io::JsonWriter&) const {}

void WitnessRule::DeserializeJson(const io::JsonReader&) {}

bool WitnessRule::operator==(const WitnessRule& other) const
{
    return action_ == other.action_;
}

bool WitnessRule::operator!=(const WitnessRule& other) const { return !(*this == other); }

std::shared_ptr<WitnessCondition> WitnessCondition::DeserializeFrom(io::BinaryReader&, uint8_t)
{
    return nullptr;
}

Signer::Signer() : account_(), scopes_(WitnessScope::None), allowedContracts_(), allowedGroups_(), rules_() {}

Signer::Signer(const io::UInt160& account, WitnessScope scopes)
    : account_(account), scopes_(scopes), allowedContracts_(), allowedGroups_(), rules_()
{
}

const io::UInt160& Signer::GetAccount() const { return account_; }

void Signer::SetAccount(const io::UInt160& account) { account_ = account; }

WitnessScope Signer::GetScopes() const { return scopes_; }

void Signer::SetScopes(WitnessScope scopes) { scopes_ = scopes; }

const std::vector<io::UInt160>& Signer::GetAllowedContracts() const { return allowedContracts_; }

void Signer::SetAllowedContracts(const std::vector<io::UInt160>& allowedContracts)
{
    allowedContracts_ = allowedContracts;
}

const std::vector<cryptography::ecc::ECPoint>& Signer::GetAllowedGroups() const { return allowedGroups_; }

void Signer::SetAllowedGroups(const std::vector<cryptography::ecc::ECPoint>& allowedGroups)
{
    allowedGroups_ = allowedGroups;
}

const std::vector<WitnessRule>& Signer::GetRules() const { return rules_; }

void Signer::SetRules(const std::vector<WitnessRule>& rules) { rules_ = rules; }

void Signer::Serialize(io::BinaryWriter& writer) const
{
    account_.Serialize(writer);
    writer.Write(static_cast<uint8_t>(scopes_));
    writer.WriteVarInt(allowedContracts_.size());
    for (const auto& contract : allowedContracts_)
    {
        contract.Serialize(writer);
    }

    writer.WriteVarInt(allowedGroups_.size());
    for (const auto& group : allowedGroups_)
    {
        auto bytes = group.ToArray();
        writer.WriteVarBytes(bytes);
    }

    writer.WriteVarInt(rules_.size());
    for (const auto& rule : rules_)
    {
        rule.Serialize(writer);
    }
}

void Signer::Deserialize(io::BinaryReader& reader)
{
    account_.Deserialize(reader);
    scopes_ = static_cast<WitnessScope>(reader.ReadUInt8());

    allowedContracts_.clear();
    int64_t contractCount = reader.ReadVarInt();
    for (int64_t i = 0; i < contractCount; ++i)
    {
        io::UInt160 contract;
        contract.Deserialize(reader);
        allowedContracts_.push_back(contract);
    }

    allowedGroups_.clear();
    int64_t groupCount = reader.ReadVarInt();
    for (int64_t i = 0; i < groupCount; ++i)
    {
        allowedGroups_.push_back(cryptography::ecc::ECPoint::Infinity());
    }

    rules_.clear();
    int64_t ruleCount = reader.ReadVarInt();
    for (int64_t i = 0; i < ruleCount; ++i)
    {
        WitnessRule rule;
        rule.Deserialize(reader);
        rules_.push_back(rule);
    }
}

void Signer::SerializeJson(io::JsonWriter&) const {}

void Signer::DeserializeJson(const io::JsonReader&) {}

bool Signer::operator==(const Signer& other) const
{
    return account_ == other.account_ && scopes_ == other.scopes_;
}

bool Signer::operator!=(const Signer& other) const { return !(*this == other); }

TransactionVerificationContext::TransactionVerificationContext() = default;

TransactionVerificationContext::~TransactionVerificationContext() = default;

bool TransactionVerificationContext::CheckTransaction(std::shared_ptr<Transaction>) { return true; }

void TransactionVerificationContext::AddTransaction(std::shared_ptr<Transaction>) {}

bool TransactionVerificationContext::IsConflicted(std::shared_ptr<Transaction>) const { return false; }

void TransactionVerificationContext::Reset()
{
    account_conflicts_.clear();
    transaction_hashes_.clear();
}

void TransactionVerificationContext::Clear()
{
    account_conflicts_.clear();
    transaction_hashes_.clear();
}

size_t TransactionVerificationContext::GetTransactionCount() const { return 0; }

bool TransactionVerificationContext::HasOutputConflict(std::shared_ptr<Transaction>) const { return false; }

bool TransactionVerificationContext::HasAccountConflict(std::shared_ptr<Transaction>) const { return false; }
}  // namespace neo::ledger
