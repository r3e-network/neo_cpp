#include <neo/network/p2p/payloads/ping_payload.h>

#include <chrono>
#include <random>

namespace neo::network::p2p::payloads
{
PingPayload::PingPayload() : lastBlockIndex_(0), timestamp_(0), nonce_(0) {}

PingPayload::PingPayload(uint32_t lastBlockIndex, uint32_t timestamp, uint32_t nonce)
    : lastBlockIndex_(lastBlockIndex), timestamp_(timestamp), nonce_(nonce)
{
}

uint32_t PingPayload::GetTimestamp() const { return timestamp_; }

void PingPayload::SetTimestamp(uint32_t timestamp) { timestamp_ = timestamp; }

uint32_t PingPayload::GetNonce() const { return nonce_; }

void PingPayload::SetNonce(uint32_t nonce) { nonce_ = nonce; }

uint32_t PingPayload::GetLastBlockIndex() const { return lastBlockIndex_; }

void PingPayload::SetLastBlockIndex(uint32_t lastBlockIndex) { lastBlockIndex_ = lastBlockIndex; }

size_t PingPayload::GetSize() const
{
    // Size calculation matching C# implementation:
    return sizeof(uint32_t) +  // LastBlockIndex
           sizeof(uint32_t) +  // Timestamp
           sizeof(uint32_t);   // Nonce
}

PingPayload PingPayload::Create(uint32_t height)
{
    // Random generator to match C# Random.Next()
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(0, UINT32_MAX);

    return Create(height, dis(gen));
}

PingPayload PingPayload::Create(uint32_t height, uint32_t nonce)
{
    PingPayload payload;

    // Set fields in exact same order as C# implementation
    payload.SetLastBlockIndex(height);

    // Get timestamp exactly as in C# TimeProvider.Current.UtcNow.ToTimestamp()
    payload.SetTimestamp(static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()));

    payload.SetNonce(nonce);

    return payload;
}

void PingPayload::Serialize(io::BinaryWriter& writer) const
{
    // Match serialization order in C# exactly: LastBlockIndex, Timestamp, Nonce
    writer.Write(lastBlockIndex_);
    writer.Write(timestamp_);
    writer.Write(nonce_);
}

void PingPayload::Deserialize(io::BinaryReader& reader)
{
    // Match deserialization order in C# exactly: LastBlockIndex, Timestamp, Nonce
    lastBlockIndex_ = reader.ReadUInt32();
    timestamp_ = reader.ReadUInt32();
    nonce_ = reader.ReadUInt32();
}

void PingPayload::SerializeJson(io::JsonWriter& writer) const
{
    writer.Write("lastBlockIndex", lastBlockIndex_);
    writer.Write("timestamp", timestamp_);
    writer.Write("nonce", nonce_);
}

void PingPayload::DeserializeJson(const io::JsonReader& reader)
{
    lastBlockIndex_ = reader.ReadUInt32("lastBlockIndex");
    timestamp_ = reader.ReadUInt32("timestamp");
    nonce_ = reader.ReadUInt32("nonce");
}
}  // namespace neo::network::p2p::payloads
