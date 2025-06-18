#include <neo/persistence/storage_key.h>
#include <neo/io/memory_stream.h>
#include <algorithm>
#include <bit>

namespace neo::persistence
{
    StorageKey::StorageKey() = default;

    StorageKey::StorageKey(int32_t contractId)
        : id_(contractId)
    {
    }

    StorageKey::StorageKey(int32_t contractId, const io::ByteVector& key)
        : id_(contractId), key_(key)
    {
    }

    StorageKey::StorageKey(const io::ByteVector& data)
    {
        if (data.Size() < sizeof(int32_t))
            throw std::invalid_argument("Invalid storage key data");
        
        // Read contract ID (little-endian)
        std::memcpy(&id_, data.Data(), sizeof(int32_t));
        
        // Read key data (rest of the bytes)
        if (data.Size() > sizeof(int32_t))
        {
            key_ = io::ByteVector(data.Data() + sizeof(int32_t), data.Size() - sizeof(int32_t));
        }
    }

    size_t StorageKey::GetLength() const
    {
        if (!cacheValid_)
        {
            cache_ = Build();
            cacheValid_ = true;
        }
        return cache_.Size();
    }

    StorageKey StorageKey::Create(int32_t id, uint8_t prefix)
    {
        auto data = io::ByteVector(PREFIX_LENGTH);
        FillHeader(std::span<uint8_t>(data.Data(), data.Size()), id, prefix);
        return StorageKey(id, io::ByteVector(data.Data() + sizeof(int32_t), data.Size() - sizeof(int32_t)));
    }

    StorageKey StorageKey::Create(int32_t id, uint8_t prefix, uint8_t content)
    {
        auto data = io::ByteVector(PREFIX_LENGTH + sizeof(uint8_t));
        FillHeader(std::span<uint8_t>(data.Data(), PREFIX_LENGTH), id, prefix);
        data[PREFIX_LENGTH] = content;
        return StorageKey(id, io::ByteVector(data.Data() + sizeof(int32_t), data.Size() - sizeof(int32_t)));
    }

    StorageKey StorageKey::Create(int32_t id, uint8_t prefix, const io::UInt160& hash)
    {
        auto data = io::ByteVector(PREFIX_LENGTH + io::UInt160::SIZE);
        FillHeader(std::span<uint8_t>(data.Data(), PREFIX_LENGTH), id, prefix);
        
        io::MemoryStream stream(data.Data() + PREFIX_LENGTH, io::UInt160::SIZE);
        io::BinaryWriter writer(stream);
        hash.Serialize(writer);
        
        return StorageKey(id, io::ByteVector(data.Data() + sizeof(int32_t), data.Size() - sizeof(int32_t)));
    }

    StorageKey StorageKey::Create(int32_t id, uint8_t prefix, const io::UInt256& hash)
    {
        auto data = io::ByteVector(PREFIX_LENGTH + io::UInt256::SIZE);
        FillHeader(std::span<uint8_t>(data.Data(), PREFIX_LENGTH), id, prefix);
        
        io::MemoryStream stream(data.Data() + PREFIX_LENGTH, io::UInt256::SIZE);
        io::BinaryWriter writer(stream);
        hash.Serialize(writer);
        
        return StorageKey(id, io::ByteVector(data.Data() + sizeof(int32_t), data.Size() - sizeof(int32_t)));
    }

    StorageKey StorageKey::Create(int32_t id, uint8_t prefix, const cryptography::ecc::ECPoint& publicKey)
    {
        auto keyData = publicKey.ToArray();
        return Create(id, prefix, std::span<const uint8_t>(keyData.Data(), keyData.Size()));
    }

    StorageKey StorageKey::Create(int32_t id, uint8_t prefix, int32_t bigEndian)
    {
        auto data = io::ByteVector(PREFIX_LENGTH + sizeof(int32_t));
        FillHeader(std::span<uint8_t>(data.Data(), PREFIX_LENGTH), id, prefix);
        
        // Write as big-endian
        int32_t beValue = static_cast<int32_t>(std::byteswap(static_cast<uint32_t>(bigEndian)));
        std::memcpy(data.Data() + PREFIX_LENGTH, &beValue, sizeof(int32_t));
        
        return StorageKey(id, io::ByteVector(data.Data() + sizeof(int32_t), data.Size() - sizeof(int32_t)));
    }

    StorageKey StorageKey::Create(int32_t id, uint8_t prefix, uint32_t bigEndian)
    {
        auto data = io::ByteVector(PREFIX_LENGTH + sizeof(uint32_t));
        FillHeader(std::span<uint8_t>(data.Data(), PREFIX_LENGTH), id, prefix);
        
        // Write as big-endian
        uint32_t beValue = std::byteswap(bigEndian);
        std::memcpy(data.Data() + PREFIX_LENGTH, &beValue, sizeof(uint32_t));
        
        return StorageKey(id, io::ByteVector(data.Data() + sizeof(int32_t), data.Size() - sizeof(int32_t)));
    }

    StorageKey StorageKey::Create(int32_t id, uint8_t prefix, int64_t bigEndian)
    {
        auto data = io::ByteVector(PREFIX_LENGTH + sizeof(int64_t));
        FillHeader(std::span<uint8_t>(data.Data(), PREFIX_LENGTH), id, prefix);
        
        // Write as big-endian
        int64_t beValue = static_cast<int64_t>(std::byteswap(static_cast<uint64_t>(bigEndian)));
        std::memcpy(data.Data() + PREFIX_LENGTH, &beValue, sizeof(int64_t));
        
        return StorageKey(id, io::ByteVector(data.Data() + sizeof(int32_t), data.Size() - sizeof(int32_t)));
    }

    StorageKey StorageKey::Create(int32_t id, uint8_t prefix, uint64_t bigEndian)
    {
        auto data = io::ByteVector(PREFIX_LENGTH + sizeof(uint64_t));
        FillHeader(std::span<uint8_t>(data.Data(), PREFIX_LENGTH), id, prefix);
        
        // Write as big-endian
        uint64_t beValue = std::byteswap(bigEndian);
        std::memcpy(data.Data() + PREFIX_LENGTH, &beValue, sizeof(uint64_t));
        
        return StorageKey(id, io::ByteVector(data.Data() + sizeof(int32_t), data.Size() - sizeof(int32_t)));
    }

    StorageKey StorageKey::Create(int32_t id, uint8_t prefix, const std::span<const uint8_t>& content)
    {
        auto data = io::ByteVector(PREFIX_LENGTH + content.size());
        FillHeader(std::span<uint8_t>(data.Data(), PREFIX_LENGTH), id, prefix);
        std::memcpy(data.Data() + PREFIX_LENGTH, content.data(), content.size());
        
        return StorageKey(id, io::ByteVector(data.Data() + sizeof(int32_t), data.Size() - sizeof(int32_t)));
    }

    StorageKey StorageKey::Create(int32_t id, uint8_t prefix, const io::UInt256& hash, const io::UInt160& signer)
    {
        auto data = io::ByteVector(PREFIX_LENGTH + io::UInt256::SIZE + io::UInt160::SIZE);
        FillHeader(std::span<uint8_t>(data.Data(), PREFIX_LENGTH), id, prefix);
        
        // Write UInt256
        io::MemoryStream stream1(data.Data() + PREFIX_LENGTH, io::UInt256::SIZE);
        io::BinaryWriter writer1(stream1);
        hash.Serialize(writer1);
        
        // Write UInt160
        io::MemoryStream stream2(data.Data() + PREFIX_LENGTH + io::UInt256::SIZE, io::UInt160::SIZE);
        io::BinaryWriter writer2(stream2);
        signer.Serialize(writer2);
        
        return StorageKey(id, io::ByteVector(data.Data() + sizeof(int32_t), data.Size() - sizeof(int32_t)));
    }

    io::ByteVector StorageKey::CreateSearchPrefix(int32_t id, const std::span<const uint8_t>& prefix)
    {
        auto buffer = io::ByteVector(sizeof(int32_t) + prefix.size());
        std::memcpy(buffer.Data(), &id, sizeof(int32_t));
        std::memcpy(buffer.Data() + sizeof(int32_t), prefix.data(), prefix.size());
        return buffer;
    }

    io::ByteVector StorageKey::ToArray() const
    {
        if (!cacheValid_)
        {
            cache_ = Build();
            cacheValid_ = true;
        }
        return cache_;
    }

    void StorageKey::Serialize(io::BinaryWriter& writer) const
    {
        auto data = ToArray();
        writer.WriteBytes(data.AsSpan());
    }

    void StorageKey::Deserialize(io::BinaryReader& reader)
    {
        // Read contract ID
        id_ = reader.ReadInt32();
        
        // Read remaining bytes as key
        auto remaining = reader.Available();
        if (remaining > 0)
        {
            key_ = reader.ReadBytes(remaining);
        }
        else
        {
            key_ = io::ByteVector();
        }
        
        cacheValid_ = false;
    }

    bool StorageKey::operator==(const StorageKey& other) const
    {
        return id_ == other.id_ && key_ == other.key_;
    }

    bool StorageKey::operator!=(const StorageKey& other) const
    {
        return !(*this == other);
    }

    bool StorageKey::operator<(const StorageKey& other) const
    {
        if (id_ != other.id_)
            return id_ < other.id_;
        return key_ < other.key_;
    }

    void StorageKey::FillHeader(std::span<uint8_t> data, int32_t id, uint8_t prefix)
    {
        // Write contract ID as little-endian
        std::memcpy(data.data(), &id, sizeof(int32_t));
        data[sizeof(int32_t)] = prefix;
    }

    io::ByteVector StorageKey::Build() const
    {
        auto buffer = io::ByteVector(sizeof(int32_t) + key_.Size());
        std::memcpy(buffer.Data(), &id_, sizeof(int32_t));
        if (!key_.IsEmpty())
        {
            std::memcpy(buffer.Data() + sizeof(int32_t), key_.Data(), key_.Size());
        }
        return buffer;
    }
}