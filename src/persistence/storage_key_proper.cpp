// Copyright (C) 2015-2025 The Neo Project.
//
// storage_key_proper.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#include <algorithm>
#include <bit>
#include <mutex>
#include <neo/io/memory_stream.h>
#include <neo/persistence/data_cache.h>
#include <neo/persistence/storage_key.h>
#include <neo/smartcontract/native/contract_management.h>

using namespace neo::io;

// Helper function for byte swapping (C++20 compatible)
namespace
{
template <typename T>
constexpr T byteswap(T value) noexcept
{
    if constexpr (sizeof(T) == 2)
    {
        return ((value & 0xFF) << 8) | ((value >> 8) & 0xFF);
    }
    else if constexpr (sizeof(T) == 4)
    {
        return ((value & 0xFF) << 24) | (((value >> 8) & 0xFF) << 16) | (((value >> 16) & 0xFF) << 8) |
               ((value >> 24) & 0xFF);
    }
    else if constexpr (sizeof(T) == 8)
    {
        return ((value & 0xFF) << 56) | (((value >> 8) & 0xFF) << 48) | (((value >> 16) & 0xFF) << 40) |
               (((value >> 24) & 0xFF) << 32) | (((value >> 32) & 0xFF) << 24) | (((value >> 40) & 0xFF) << 16) |
               (((value >> 48) & 0xFF) << 8) | ((value >> 56) & 0xFF);
    }
    return value;
}

// Thread-safe contract ID cache
class ContractIdCache
{
  private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<UInt160, int32_t> cache_;
    static constexpr size_t MAX_CACHE_SIZE = 1000;

  public:
    static ContractIdCache& GetInstance()
    {
        static ContractIdCache instance;
        return instance;
    }

    std::optional<int32_t> Get(const UInt160& hash) const
    {
        std::shared_lock lock(mutex_);
        auto it = cache_.find(hash);
        if (it != cache_.end())
        {
            return it->second;
        }
        return std::nullopt;
    }

    void Put(const UInt160& hash, int32_t id)
    {
        std::unique_lock lock(mutex_);

        // Evict oldest entries if cache is full
        if (cache_.size() >= MAX_CACHE_SIZE)
        {
            // Simple eviction: remove first element
            cache_.erase(cache_.begin());
        }

        cache_[hash] = id;
    }

    void Clear()
    {
        std::unique_lock lock(mutex_);
        cache_.clear();
    }
};
}  // namespace

namespace neo::persistence
{
StorageKey::StorageKey() = default;

StorageKey::StorageKey(int32_t contractId) : id_(contractId) {}

StorageKey::StorageKey(int32_t contractId, const io::ByteVector& key) : id_(contractId), key_(key) {}

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

StorageKey::StorageKey(const io::UInt160& scriptHash) : scriptHash_(scriptHash)
{
    // For Neo 3.x - need to look up the contract ID from ContractManagement
    // First check cache
    auto& cache = ContractIdCache::GetInstance();
    auto cachedId = cache.Get(scriptHash);

    if (cachedId.has_value())
    {
        id_ = cachedId.value();
    }
    else
    {
        // If not in cache, use a temporary invalid ID
        // The actual lookup should be done through DataCache context
        id_ = -1;
        requiresLookup_ = true;
    }
}

StorageKey::StorageKey(const io::UInt160& scriptHash, const io::ByteVector& key) : key_(key), scriptHash_(scriptHash)
{
    // For Neo 3.x - need to look up the contract ID from ContractManagement
    // First check cache
    auto& cache = ContractIdCache::GetInstance();
    auto cachedId = cache.Get(scriptHash);

    if (cachedId.has_value())
    {
        id_ = cachedId.value();
    }
    else
    {
        // If not in cache, use a temporary invalid ID
        // The actual lookup should be done through DataCache context
        id_ = -1;
        requiresLookup_ = true;
    }
}

int32_t StorageKey::ResolveContractId(const DataCache& dataCache) const
{
    if (!requiresLookup_ || !scriptHash_.has_value())
    {
        return id_;
    }

    // Look up the contract from ContractManagement
    auto contract = smartcontract::native::ContractManagement::GetContract(dataCache, *scriptHash_);
    if (contract)
    {
        id_ = contract->GetId();
        requiresLookup_ = false;

        // Cache the result
        auto& cache = ContractIdCache::GetInstance();
        cache.Put(*scriptHash_, id_);

        return id_;
    }

    // Contract not found
    throw std::runtime_error("Contract not found for hash: " + scriptHash_->ToString());
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

StorageKey StorageKey::Create(int32_t id, uint8_t prefix, const std::span<const uint8_t>& content)
{
    auto data = io::ByteVector(PREFIX_LENGTH + content.size());
    FillHeader(std::span<uint8_t>(data.Data(), PREFIX_LENGTH), id, prefix);
    std::copy(content.begin(), content.end(), data.Data() + PREFIX_LENGTH);
    return StorageKey(id, io::ByteVector(data.Data() + sizeof(int32_t), data.Size() - sizeof(int32_t)));
}

StorageKey StorageKey::Create(const io::UInt160& scriptHash, uint8_t prefix)
{
    StorageKey key(scriptHash);
    key.key_.Push(prefix);
    return key;
}

StorageKey StorageKey::CreateWithContract(const DataCache& dataCache, const io::UInt160& scriptHash, uint8_t prefix)
{
    // Look up the contract ID
    auto contract = smartcontract::native::ContractManagement::GetContract(dataCache, scriptHash);
    if (!contract)
    {
        throw std::runtime_error("Contract not found for hash: " + scriptHash.ToString());
    }

    return Create(contract->GetId(), prefix);
}

int32_t StorageKey::GetContractId() const
{
    if (requiresLookup_)
    {
        // If lookup is required, the caller must provide DataCache context
        throw std::runtime_error("Contract ID requires DataCache context for resolution");
    }
    return id_;
}

const io::ByteVector& StorageKey::GetKey() const
{
    return key_;
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
    writer.Write(data.AsSpan());
}

void StorageKey::Deserialize(io::BinaryReader& reader)
{
    id_ = reader.ReadInt32();
    auto available = reader.Available();
    if (available > 0)
    {
        key_.Resize(available);
        reader.ReadBytes(key_.Data(), available);
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


io::ByteVector StorageKey::Build() const
{
    io::ByteVector result;
    result.Reserve(sizeof(int32_t) + key_.Size());

    // Write contract ID (little-endian)
    result.Push(static_cast<uint8_t>(id_ & 0xFF));
    result.Push(static_cast<uint8_t>((id_ >> 8) & 0xFF));
    result.Push(static_cast<uint8_t>((id_ >> 16) & 0xFF));
    result.Push(static_cast<uint8_t>((id_ >> 24) & 0xFF));

    // Append key data
    result.Append(key_.AsSpan());

    return result;
}

void StorageKey::FillHeader(std::span<uint8_t> data, int32_t id, uint8_t prefix)
{
    if (data.size() < PREFIX_LENGTH)
        throw std::invalid_argument("Buffer too small for header");

    // Write contract ID (little-endian)
    data[0] = static_cast<uint8_t>(id & 0xFF);
    data[1] = static_cast<uint8_t>((id >> 8) & 0xFF);
    data[2] = static_cast<uint8_t>((id >> 16) & 0xFF);
    data[3] = static_cast<uint8_t>((id >> 24) & 0xFF);

    // Write prefix
    data[4] = prefix;
}

void StorageKey::DeserializeFromArray(const std::span<const uint8_t>& data)
{
    if (data.size() < 4)
    {
        throw std::invalid_argument("Data too short for contract ID");
    }
    // Read contract ID (little-endian)
    id_ = static_cast<int32_t>(data[0]) | (static_cast<int32_t>(data[1]) << 8) | 
          (static_cast<int32_t>(data[2]) << 16) | (static_cast<int32_t>(data[3]) << 24);
    
    // Read key data (if any)
    if (data.size() > 4)
    {
        key_ = ByteVector(data.data() + 4, data.size() - 4);
    }
    else
    {
        key_.Clear();
    }
}

UInt160 StorageKey::GetScriptHash() const
{
    // For Neo N3, we need to resolve contract ID to script hash
    // This would normally require blockchain state access
    // For now, we'll create a placeholder implementation that returns a valid UInt160
    return UInt160::Zero();
}

StorageKey StorageKey::Create(int32_t id, uint8_t prefix, const io::UInt256& hash, const io::UInt160& signer)
{
    auto data = ByteVector(PREFIX_LENGTH + UInt256::SIZE + UInt160::SIZE);
    FillHeader(std::span<uint8_t>(data.Data(), PREFIX_LENGTH), id, prefix);
    
    // Write hash and signer
    MemoryStream stream(data.Data() + PREFIX_LENGTH, UInt256::SIZE + UInt160::SIZE);
    BinaryWriter writer(stream);
    hash.Serialize(writer);
    signer.Serialize(writer);
    
    return StorageKey(id, ByteVector(data.Data() + sizeof(int32_t), data.Size() - sizeof(int32_t)));
}

}  // namespace neo::persistence