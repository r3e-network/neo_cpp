#include <neo/network/p2p/payloads/neo3_transaction.h>

#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>

#include <array>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace neo::network::p2p::payloads
{
namespace
{
template <typename T>
T SafeAdd(T lhs, T rhs)
{
    if constexpr (std::is_signed_v<T>)
    {
        if ((rhs > 0 && lhs > std::numeric_limits<T>::max() - rhs) ||
            (rhs < 0 && lhs < std::numeric_limits<T>::min() - rhs))
        {
            throw std::overflow_error("Neo3Transaction stub overflow");
        }
    }
    return lhs + rhs;
}

std::array<uint8_t, io::UInt256::Size> HashBytes(const Neo3Transaction& tx)
{
    std::array<uint8_t, io::UInt256::Size> result{};
    std::string seed;
    seed.reserve(64 + tx.GetScript().Size());

    const auto version = tx.GetVersion();
    seed.append(reinterpret_cast<const char*>(&version), sizeof(version));
    const auto nonce = tx.GetNonce();
    seed.append(reinterpret_cast<const char*>(&nonce), sizeof(nonce));
    const auto sysFee = tx.GetSystemFee();
    seed.append(reinterpret_cast<const char*>(&sysFee), sizeof(sysFee));
    const auto netFee = tx.GetNetworkFee();
    seed.append(reinterpret_cast<const char*>(&netFee), sizeof(netFee));
    const auto vub = tx.GetValidUntilBlock();
    seed.append(reinterpret_cast<const char*>(&vub), sizeof(vub));

    const auto& script = tx.GetScript();
    seed.append(reinterpret_cast<const char*>(script.Data()), script.Size());

    std::hash<std::string_view> hasher;
    auto mix = static_cast<uint64_t>(hasher(std::string_view(seed)));
    auto mix2 = static_cast<uint64_t>(hasher(std::string_view(
        reinterpret_cast<const char*>(script.Data()), script.Size())));

    auto write_chunk = [&result](uint64_t value, size_t offset) {
        for (size_t i = 0; i < sizeof(uint64_t) && offset + i < result.size(); ++i)
        {
            result[offset + i] = static_cast<uint8_t>((value >> (8 * i)) & 0xFF);
        }
    };

    write_chunk(mix, 0);
    write_chunk(mix2, 8);
    write_chunk(static_cast<uint64_t>(seed.size()), 16);
    write_chunk(static_cast<uint64_t>(seed.empty() ? 0 : seed.front()), 24);

    return result;
}
}  // namespace

Neo3Transaction::Neo3Transaction()
    : version_(0),
      nonce_(0),
      systemFee_(0),
      networkFee_(0),
      validUntilBlock_(0),
      hash_(io::UInt256::Zero()),
      hashCalculated_(false),
      size_(0),
      sizeCalculated_(false)
{
}

Neo3Transaction::Neo3Transaction(const Neo3Transaction&) = default;
Neo3Transaction::Neo3Transaction(Neo3Transaction&&) noexcept = default;
Neo3Transaction& Neo3Transaction::operator=(const Neo3Transaction&) = default;
Neo3Transaction& Neo3Transaction::operator=(Neo3Transaction&&) noexcept = default;
Neo3Transaction::~Neo3Transaction() = default;

uint8_t Neo3Transaction::GetVersion() const { return version_; }
void Neo3Transaction::SetVersion(uint8_t version)
{
    version_ = version;
    InvalidateCache();
}

uint32_t Neo3Transaction::GetNonce() const { return nonce_; }
void Neo3Transaction::SetNonce(uint32_t nonce)
{
    nonce_ = nonce;
    InvalidateCache();
}

int64_t Neo3Transaction::GetSystemFee() const { return systemFee_; }
void Neo3Transaction::SetSystemFee(int64_t systemFee)
{
    systemFee_ = systemFee;
    InvalidateCache();
}

int64_t Neo3Transaction::GetNetworkFee() const { return networkFee_; }
void Neo3Transaction::SetNetworkFee(int64_t networkFee)
{
    networkFee_ = networkFee;
    InvalidateCache();
}

int64_t Neo3Transaction::GetTotalFee() const { return SafeAdd(systemFee_, networkFee_); }

uint32_t Neo3Transaction::GetValidUntilBlock() const { return validUntilBlock_; }
void Neo3Transaction::SetValidUntilBlock(uint32_t validUntilBlock)
{
    validUntilBlock_ = validUntilBlock;
    InvalidateCache();
}

const std::vector<ledger::Signer>& Neo3Transaction::GetSigners() const { return signers_; }
void Neo3Transaction::SetSigners(const std::vector<ledger::Signer>&)
{
    // Stub keeps signers empty to avoid deep dependency graph
    signers_.clear();
    InvalidateCache();
}

const std::vector<std::shared_ptr<ledger::TransactionAttribute>>& Neo3Transaction::GetAttributes() const
{
    return attributes_;
}
void Neo3Transaction::SetAttributes(const std::vector<std::shared_ptr<ledger::TransactionAttribute>>& attributes)
{
    attributes_ = attributes;
    InvalidateCache();
}

const io::ByteVector& Neo3Transaction::GetScript() const { return script_; }
void Neo3Transaction::SetScript(const io::ByteVector& script)
{
    script_ = script;
    InvalidateCache();
}

const std::vector<ledger::Witness>& Neo3Transaction::GetWitnesses() const { return witnesses_; }
void Neo3Transaction::SetWitnesses(const std::vector<ledger::Witness>&)
{
    // Witnesses ignored in stub implementation
    witnesses_.clear();
    sizeCalculated_ = false;
}

io::UInt160 Neo3Transaction::GetSender() const { return io::UInt160(); }

int64_t Neo3Transaction::GetFeePerByte() const
{
    const auto size = GetSize();
    return size == 0 ? 0 : GetNetworkFee() / size;
}

InventoryType Neo3Transaction::GetInventoryType() const { return InventoryType::TX; }

std::vector<io::UInt160> Neo3Transaction::GetScriptHashesForVerifying() const
{
    return {};
}

void Neo3Transaction::Serialize(io::BinaryWriter& writer) const
{
    SerializeUnsigned(writer);
    writer.WriteVarInt(static_cast<int64_t>(witnesses_.size()));
    for (const auto& witness : witnesses_)
    {
        (void)witness;  // Stub ignores witness serialization contents
    }
}

void Neo3Transaction::Deserialize(io::BinaryReader& reader)
{
    DeserializeUnsigned(reader);
    const uint64_t witnessCount = reader.ReadVarInt(16);
    witnesses_.clear();
    witnesses_.reserve(witnessCount);
    for (uint64_t i = 0; i < witnessCount; ++i)
    {
        witnesses_.emplace_back();
    }
}

void Neo3Transaction::SerializeUnsigned(io::BinaryWriter& writer) const
{
    writer.Write(version_);
    writer.Write(nonce_);
    writer.Write(systemFee_);
    writer.Write(networkFee_);
    writer.Write(validUntilBlock_);

    writer.WriteVarInt(static_cast<int64_t>(0));  // signers omitted in stub

    writer.WriteVarInt(static_cast<int64_t>(attributes_.size()));
    for (const auto& attribute : attributes_)
    {
        (void)attribute;  // Stub ignores attribute serialization contents
    }

    writer.WriteVarBytes(script_);
}

void Neo3Transaction::DeserializeUnsigned(io::BinaryReader& reader)
{
    version_ = reader.ReadUInt8();
    nonce_ = reader.ReadUInt32();
    systemFee_ = reader.ReadInt64();
    networkFee_ = reader.ReadInt64();
    validUntilBlock_ = reader.ReadUInt32();

    const uint64_t signerCount = reader.ReadVarInt(16);
    for (uint64_t i = 0; i < signerCount; ++i)
    {
        io::UInt160 account;
        account.Deserialize(reader);
        (void)reader.ReadUInt8();
    }

    (void)reader.ReadVarInt(16);
    attributes_.clear();

    script_ = reader.ReadVarBytes(65536);
    InvalidateCache();
}

void Neo3Transaction::SerializeJson(io::JsonWriter& writer) const
{
    writer.WriteStartObject();
    writer.WriteProperty("hash", GetHash().ToString());
    writer.WriteProperty("size", GetSize());
    writer.WriteProperty("version", static_cast<int>(version_));
    writer.WriteProperty("nonce", nonce_);
    writer.WriteProperty("sysfee", GetSystemFee());
    writer.WriteProperty("netfee", GetNetworkFee());
    writer.WriteProperty("validuntilblock", validUntilBlock_);
    writer.WriteEndObject();
}

void Neo3Transaction::DeserializeJson(const io::JsonReader& reader)
{
    if (!reader.GetJson().is_object())
    {
        throw std::runtime_error("Neo3Transaction stub expected object json");
    }
    if (reader.HasKey("version")) version_ = static_cast<uint8_t>(reader.ReadInt32("version"));
    if (reader.HasKey("nonce")) nonce_ = reader.ReadUInt32("nonce");
    if (reader.HasKey("sysfee")) systemFee_ = reader.ReadInt64("sysfee");
    if (reader.HasKey("netfee")) networkFee_ = reader.ReadInt64("netfee");
    if (reader.HasKey("validuntilblock")) validUntilBlock_ = reader.ReadUInt32("validuntilblock");
    InvalidateCache();
}

io::UInt256 Neo3Transaction::GetHash() const
{
    if (!hashCalculated_)
    {
        CalculateHash();
    }
    return hash_;
}

int Neo3Transaction::GetSize() const
{
    if (!sizeCalculated_)
    {
        CalculateSize();
    }
    return size_;
}

bool Neo3Transaction::operator==(const Neo3Transaction& other) const
{
    return version_ == other.version_ && nonce_ == other.nonce_ && systemFee_ == other.systemFee_ &&
           networkFee_ == other.networkFee_ && validUntilBlock_ == other.validUntilBlock_ &&
           script_ == other.script_;
}

bool Neo3Transaction::operator!=(const Neo3Transaction& other) const { return !(*this == other); }

void Neo3Transaction::InvalidateCache() const
{
    hashCalculated_ = false;
    sizeCalculated_ = false;
}

void Neo3Transaction::CalculateHash() const
{
    hash_ = io::UInt256(HashBytes(*this));
    hashCalculated_ = true;
}

void Neo3Transaction::CalculateSize() const
{
    int total = HeaderSize;
    total += GetVarIntSize(signers_.size());
    for (const auto& signer : signers_)
    {
        total += GetSignerSize(signer);
    }

    total += GetVarIntSize(attributes_.size());
    for (const auto& attribute : attributes_)
    {
        (void)attribute;
        total += 1;
    }

    total += GetVarIntSize(script_.Size());
    total += static_cast<int>(script_.Size());

    total += GetVarIntSize(witnesses_.size());

    size_ = total;
    sizeCalculated_ = true;
}

int Neo3Transaction::GetVarIntSize(size_t value) const
{
    if (value < 0xFD) return 1;
    if (value <= 0xFFFF) return 3;
    if (value <= 0xFFFFFFFF) return 5;
    return 9;
}

int Neo3Transaction::GetSignerSize(const ledger::Signer&) const
{
    return static_cast<int>(io::UInt160::Size + 1);
}

int Neo3Transaction::GetAttributeSize(const ledger::TransactionAttribute&) const { return 1; }

int Neo3Transaction::GetWitnessSize(const ledger::Witness&) const { return 0; }

std::vector<ledger::TransactionAttribute> Neo3Transaction::DeserializeAttributes(io::BinaryReader&, int)
{
    return {};
}

std::vector<ledger::Signer> Neo3Transaction::DeserializeSigners(io::BinaryReader&, int) { return {}; }
}  // namespace neo::network::p2p::payloads
