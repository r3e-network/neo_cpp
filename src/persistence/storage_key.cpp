#include <neo/persistence/storage_key.h>
#include <neo/io/memory_stream.h>
#include <algorithm>
#include <bit>

// Helper function for byte swapping (C++20 compatible)
namespace {
    template<typename T>
    constexpr T byteswap(T value) noexcept {
        if constexpr (sizeof(T) == 2) {
            return ((value & 0xFF) << 8) | ((value >> 8) & 0xFF);
        } else if constexpr (sizeof(T) == 4) {
            return ((value & 0xFF) << 24) | 
                   (((value >> 8) & 0xFF) << 16) | 
                   (((value >> 16) & 0xFF) << 8) | 
                   ((value >> 24) & 0xFF);
        } else if constexpr (sizeof(T) == 8) {
            return ((value & 0xFF) << 56) | 
                   (((value >> 8) & 0xFF) << 48) | 
                   (((value >> 16) & 0xFF) << 40) | 
                   (((value >> 24) & 0xFF) << 32) | 
                   (((value >> 32) & 0xFF) << 24) | 
                   (((value >> 40) & 0xFF) << 16) | 
                   (((value >> 48) & 0xFF) << 8) | 
                   ((value >> 56) & 0xFF);
        }
        return value;
    }
}

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

    StorageKey::StorageKey(const io::UInt160& scriptHash)
        : scriptHash_(scriptHash)
    {
        // For Neo 2.x compatibility - convert UInt160 to contract ID
        // This is a simplified conversion - in practice would need proper mapping
        id_ = *reinterpret_cast<const int32_t*>(scriptHash.Data());
    }

    StorageKey::StorageKey(const io::UInt160& scriptHash, const io::ByteVector& key)
        : key_(key), scriptHash_(scriptHash)
    {
        // For Neo 2.x compatibility - convert UInt160 to contract ID
        // This is a simplified conversion - in practice would need proper mapping
        id_ = *reinterpret_cast<const int32_t*>(scriptHash.Data());
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
        int32_t beValue = static_cast<int32_t>(byteswap(static_cast<uint32_t>(bigEndian)));
        std::memcpy(data.Data() + PREFIX_LENGTH, &beValue, sizeof(int32_t));
        
        return StorageKey(id, io::ByteVector(data.Data() + sizeof(int32_t), data.Size() - sizeof(int32_t)));
    }

    StorageKey StorageKey::Create(int32_t id, uint8_t prefix, uint32_t bigEndian)
    {
        auto data = io::ByteVector(PREFIX_LENGTH + sizeof(uint32_t));
        FillHeader(std::span<uint8_t>(data.Data(), PREFIX_LENGTH), id, prefix);
        
        // Write as big-endian
        uint32_t beValue = byteswap(bigEndian);
        std::memcpy(data.Data() + PREFIX_LENGTH, &beValue, sizeof(uint32_t));
        
        return StorageKey(id, io::ByteVector(data.Data() + sizeof(int32_t), data.Size() - sizeof(int32_t)));
    }

    StorageKey StorageKey::Create(int32_t id, uint8_t prefix, int64_t bigEndian)
    {
        auto data = io::ByteVector(PREFIX_LENGTH + sizeof(int64_t));
        FillHeader(std::span<uint8_t>(data.Data(), PREFIX_LENGTH), id, prefix);
        
        // Write as big-endian
        int64_t beValue = static_cast<int64_t>(byteswap(static_cast<uint64_t>(bigEndian)));
        std::memcpy(data.Data() + PREFIX_LENGTH, &beValue, sizeof(int64_t));
        
        return StorageKey(id, io::ByteVector(data.Data() + sizeof(int32_t), data.Size() - sizeof(int32_t)));
    }

    StorageKey StorageKey::Create(int32_t id, uint8_t prefix, uint64_t bigEndian)
    {
        auto data = io::ByteVector(PREFIX_LENGTH + sizeof(uint64_t));
        FillHeader(std::span<uint8_t>(data.Data(), PREFIX_LENGTH), id, prefix);
        
        // Write as big-endian
        uint64_t beValue = byteswap(bigEndian);
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

    io::UInt160 StorageKey::GetScriptHash() const
    {
        // If we have a stored script hash (from Neo 2.x compatibility constructors), return it
        if (!scriptHash_.IsZero())
        {
            return scriptHash_;
        }
        
        // TODO: Implement proper contract ID to script hash lookup via ContractManagement
        // For now, return a placeholder hash derived from the contract ID
        // In production, this would require access to blockchain state:
        // 1. Look up ContractManagement storage with key [Prefix_ContractHash, id_]
        // 2. Return the UInt160 script hash stored at that key
        
        // Placeholder implementation: create a deterministic hash from the contract ID
        io::ByteVector data(20); // UInt160 is 20 bytes
        auto id_bytes = reinterpret_cast<const uint8_t*>(&id_);
        for (size_t i = 0; i < 20; ++i)
        {
            data[i] = id_bytes[i % sizeof(int32_t)];
        }
        return io::UInt160(data.AsSpan());
    }

    void StorageKey::Serialize(io::BinaryWriter& writer) const
    {
        // Write contract ID
        writer.Write(id_);
        
        // Write key data
        writer.WriteVarBytes(key_.AsSpan());
        
        // Write script hash
        if (!scriptHash_.IsZero())
        {
            writer.WriteBool(true);
            scriptHash_.Serialize(writer);
        }
        else
        {
            writer.WriteBool(false);
        }
    }

    void StorageKey::Deserialize(io::BinaryReader& reader)
    {
        // Read contract ID
        id_ = reader.ReadInt32();
        
        // Read key data
        auto keyData = reader.ReadVarBytes();
        key_ = io::ByteVector(keyData.Data(), keyData.Size());
        
        // Read script hash flag
        bool hasScriptHash = reader.ReadBool();
        if (hasScriptHash)
        {
            scriptHash_.Deserialize(reader);
        }
        else
        {
            scriptHash_ = io::UInt160();
        }
        
        cacheValid_ = false;
    }

    void StorageKey::DeserializeFromArray(const std::span<const uint8_t>& data)
    {
        if (data.size() < sizeof(int32_t) + 1) // At least ID + flag byte
            throw std::invalid_argument("Invalid storage key data");
        
        size_t offset = 0;
        
        // Read contract ID (little-endian)
        std::memcpy(&id_, data.data() + offset, sizeof(int32_t));
        offset += sizeof(int32_t);
        
        // Check if there's a script hash flag at the end
        bool hasScriptHash = false;
        size_t keySize = 0;
        
        if (data.size() >= offset + 1 + io::UInt160::Size) // enough space for flag + hash
        {
            // Check if the last part looks like hash flag + hash
            uint8_t flag = data[data.size() - 1 - io::UInt160::Size];
            if (flag == 1 && data.size() >= offset + 1 + io::UInt160::Size)
            {
                hasScriptHash = true;
                keySize = data.size() - offset - 1 - io::UInt160::Size;
            }
            else if (data[data.size() - 1] == 0) // flag = 0 (no hash)
            {
                keySize = data.size() - offset - 1;
            }
            else
            {
                // Old format - no script hash info
                keySize = data.size() - offset;
            }
        }
        else if (data.size() > offset && data[data.size() - 1] == 0)
        {
            // New format without script hash
            keySize = data.size() - offset - 1;
        }
        else
        {
            // Old format - no script hash info
            keySize = data.size() - offset;
        }
        
        // Read key data
        if (keySize > 0)
        {
            key_ = io::ByteVector(data.data() + offset, keySize);
        }
        else
        {
            key_ = io::ByteVector();
        }
        offset += keySize;
        
        // Read script hash if present
        if (hasScriptHash)
        {
            offset += 1; // Skip flag byte
            scriptHash_ = io::UInt160(io::ByteSpan(data.data() + offset, io::UInt160::Size));
        }
        else
        {
            scriptHash_ = io::UInt160();
        }
        
        cacheValid_ = false;
    }

    bool StorageKey::operator==(const StorageKey& other) const
    {
        // If both have script hashes, compare them first (more specific)
        if (!scriptHash_.IsZero() && !other.scriptHash_.IsZero())
        {
            return scriptHash_ == other.scriptHash_ && key_ == other.key_;
        }
        
        // If only one has a script hash, they're different
        if (!scriptHash_.IsZero() || !other.scriptHash_.IsZero())
        {
            return false;
        }
        
        // Neither has script hash, compare by contract ID and key
        return id_ == other.id_ && key_ == other.key_;
    }

    bool StorageKey::operator!=(const StorageKey& other) const
    {
        return !(*this == other);
    }

    bool StorageKey::operator<(const StorageKey& other) const
    {
        // If both have script hashes, compare them first (more specific)
        if (!scriptHash_.IsZero() && !other.scriptHash_.IsZero())
        {
            if (scriptHash_ != other.scriptHash_)
                return scriptHash_ < other.scriptHash_;
            return key_ < other.key_;
        }
        
        // If only one has a script hash, the one with script hash comes first
        if (!scriptHash_.IsZero())
            return true;
        if (!other.scriptHash_.IsZero())
            return false;
        
        // Neither has script hash, compare by contract ID and key
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
        size_t totalSize = sizeof(int32_t) + key_.Size();
        bool hasScriptHash = !scriptHash_.IsZero();
        
        if (hasScriptHash)
        {
            totalSize += 1 + io::UInt160::Size; // 1 byte flag + hash size
        }
        else
        {
            totalSize += 1; // 1 byte flag
        }
        
        auto buffer = io::ByteVector(totalSize);
        size_t offset = 0;
        
        // Write contract ID
        std::memcpy(buffer.Data() + offset, &id_, sizeof(int32_t));
        offset += sizeof(int32_t);
        
        // Write key data
        if (!key_.IsEmpty())
        {
            std::memcpy(buffer.Data() + offset, key_.Data(), key_.Size());
            offset += key_.Size();
        }
        
        // Write script hash flag and data
        if (hasScriptHash)
        {
            buffer[offset] = 1; // Has script hash
            offset += 1;
            std::memcpy(buffer.Data() + offset, scriptHash_.Data(), io::UInt160::Size);
        }
        else
        {
            buffer[offset] = 0; // No script hash
        }
        
        return buffer;
    }
}