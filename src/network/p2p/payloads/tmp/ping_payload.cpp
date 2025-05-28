#include <neo/network/payloads/ping_payload.h>
#include <random>
#include <chrono>

namespace neo::network::payloads
{
    PingPayload::PingPayload()
        : lastBlockIndex_(0), timestamp_(0), nonce_(0)
    {
    }

    PingPayload::PingPayload(uint32_t lastBlockIndex, uint32_t timestamp, uint32_t nonce)
        : lastBlockIndex_(lastBlockIndex), timestamp_(timestamp), nonce_(nonce)
    {
    }

    uint32_t PingPayload::GetLastBlockIndex() const
    {
        return lastBlockIndex_;
    }

    void PingPayload::SetLastBlockIndex(uint32_t lastBlockIndex)
    {
        lastBlockIndex_ = lastBlockIndex;
    }

    uint32_t PingPayload::GetTimestamp() const
    {
        return timestamp_;
    }

    void PingPayload::SetTimestamp(uint32_t timestamp)
    {
        timestamp_ = timestamp;
    }

    uint32_t PingPayload::GetNonce() const
    {
        return nonce_;
    }

    void PingPayload::SetNonce(uint32_t nonce)
    {
        nonce_ = nonce;
    }

    uint32_t PingPayload::GetSize() const
    {
        // Size calculation matching C# implementation:
        return sizeof(uint32_t) +  // LastBlockIndex
               sizeof(uint32_t) +  // Timestamp
               sizeof(uint32_t);   // Nonce
    }

    PingPayload PingPayload::Create(uint32_t height)
    {
        // Random generator to match C# random number generation
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> dist(0, UINT32_MAX);
        
        return Create(height, dist(gen));
    }

    PingPayload PingPayload::Create(uint32_t height, uint32_t nonce)
    {
        // Get current time as timestamp, matching C# TimeProvider.Current.UtcNow.ToTimestamp()
        auto now = std::chrono::system_clock::now();
        auto sinceEpoch = now.time_since_epoch();
        uint32_t timestamp = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(sinceEpoch).count());
        
        return PingPayload(height, timestamp, nonce);
    }

    void PingPayload::Serialize(io::BinaryWriter& writer) const
    {
        writer.Write(lastBlockIndex_);
        writer.Write(timestamp_);
        writer.Write(nonce_);
    }

    void PingPayload::Deserialize(io::BinaryReader& reader)
    {
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
}
