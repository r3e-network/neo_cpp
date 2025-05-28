#include <neo/smartcontract/native/native_contract.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/cryptography/hash.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/storage_item.h>
#include <sstream>

namespace neo::smartcontract::native
{
    NativeContract::NativeContract(const std::string& name, uint32_t id)
        : name_(name), id_(id)
    {
        // Calculate script hash
        std::ostringstream stream;
        stream << name << id;
        std::string data = stream.str();
        scriptHash_ = cryptography::Hash::Hash160(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    }

    const std::string& NativeContract::GetName() const
    {
        return name_;
    }

    uint32_t NativeContract::GetId() const
    {
        return id_;
    }

    const io::UInt160& NativeContract::GetScriptHash() const
    {
        return scriptHash_;
    }

    const std::unordered_map<std::string, std::pair<CallFlags, std::function<std::shared_ptr<vm::StackItem>(ApplicationEngine&, const std::vector<std::shared_ptr<vm::StackItem>>&)>>>& NativeContract::GetMethods() const
    {
        return methods_;
    }

    std::shared_ptr<vm::StackItem> NativeContract::Invoke(ApplicationEngine& engine, const std::string& method, const std::vector<std::shared_ptr<vm::StackItem>>& args, CallFlags callFlags)
    {
        auto it = methods_.find(method);
        if (it == methods_.end())
            throw std::runtime_error("Method not found: " + method);

        if (!CheckCallFlags(method, callFlags))
            throw std::runtime_error("Call flags not allowed: " + method);

        return it->second.second(engine, args);
    }

    bool NativeContract::CheckCallFlags(const std::string& method, CallFlags callFlags) const
    {
        auto it = methods_.find(method);
        if (it == methods_.end())
            return false;

        return (static_cast<uint8_t>(it->second.first) & static_cast<uint8_t>(callFlags)) == static_cast<uint8_t>(callFlags);
    }

    io::ByteVector NativeContract::GetStoragePrefix() const
    {
        return io::ByteVector{static_cast<uint8_t>(id_)};
    }

    io::ByteVector NativeContract::GetStorageKey(uint8_t prefix, const io::ByteVector& key) const
    {
        io::ByteVector result = GetStoragePrefix();
        result.Push(prefix);
        result.Append(key.AsSpan());
        return result;
    }

    io::ByteVector NativeContract::GetStorageKey(uint8_t prefix, const io::UInt160& key) const
    {
        io::ByteVector result = GetStoragePrefix();
        result.Push(prefix);
        io::ByteSpan keySpan(key.Data(), io::UInt160::Size);
        result.Append(keySpan);
        return result;
    }

    io::ByteVector NativeContract::GetStorageKey(uint8_t prefix, const io::UInt256& key) const
    {
        io::ByteVector result = GetStoragePrefix();
        result.Push(prefix);
        io::ByteSpan keySpan(key.Data(), io::UInt256::Size);
        result.Append(keySpan);
        return result;
    }

    io::ByteVector NativeContract::GetStorageKey(uint8_t prefix, const std::string& key) const
    {
        io::ByteVector result = GetStoragePrefix();
        result.Push(prefix);
        io::ByteSpan keySpan(reinterpret_cast<const uint8_t*>(key.data()), key.size());
        result.Append(keySpan);
        return result;
    }

    io::ByteVector NativeContract::GetStorageValue(std::shared_ptr<persistence::StoreView> snapshot, const io::ByteVector& key) const
    {
        persistence::StorageKey storageKey(scriptHash_, key);
        auto item = snapshot->TryGet(storageKey);
        if (!item)
            return io::ByteVector();

        return item->GetValue();
    }

    void NativeContract::PutStorageValue(std::shared_ptr<persistence::StoreView> snapshot, const io::ByteVector& key, const io::ByteVector& value) const
    {
        persistence::StorageKey storageKey(scriptHash_, key);
        persistence::StorageItem item(value);
        snapshot->Add(storageKey, item);
    }

    void NativeContract::DeleteStorageValue(std::shared_ptr<persistence::StoreView> snapshot, const io::ByteVector& key) const
    {
        persistence::StorageKey storageKey(scriptHash_, key);
        snapshot->Delete(storageKey);
    }

    void NativeContract::RegisterMethod(const std::string& name, CallFlags callFlags, std::function<std::shared_ptr<vm::StackItem>(ApplicationEngine&, const std::vector<std::shared_ptr<vm::StackItem>>&)> handler)
    {
        methods_[name] = std::make_pair(callFlags, handler);
    }

    persistence::StorageKey NativeContract::CreateStorageKey(uint8_t prefix) const
    {
        io::ByteVector key;
        key.Push(prefix);
        return persistence::StorageKey(scriptHash_, key);
    }

    persistence::StorageKey NativeContract::CreateStorageKey(uint8_t prefix, const io::ByteVector& key) const
    {
        io::ByteVector fullKey;
        fullKey.Push(prefix);
        fullKey.Append(key.AsSpan());
        return persistence::StorageKey(scriptHash_, fullKey);
    }

    persistence::StorageKey NativeContract::CreateStorageKey(uint8_t prefix, const io::UInt160& key) const
    {
        io::ByteVector fullKey;
        fullKey.Push(prefix);
        io::ByteSpan keySpan(key.Data(), io::UInt160::Size);
        fullKey.Append(keySpan);
        return persistence::StorageKey(scriptHash_, fullKey);
    }

    persistence::StorageKey NativeContract::CreateStorageKey(uint8_t prefix, const io::UInt256& key) const
    {
        io::ByteVector fullKey;
        fullKey.Push(prefix);
        io::ByteSpan keySpan(key.Data(), io::UInt256::Size);
        fullKey.Append(keySpan);
        return persistence::StorageKey(scriptHash_, fullKey);
    }

    persistence::StorageKey NativeContract::CreateStorageKey(uint8_t prefix, uint32_t key) const
    {
        io::ByteVector fullKey;
        fullKey.Push(prefix);

        // Convert uint32_t to big-endian bytes (as per C# implementation)
        uint8_t keyBytes[4];
        keyBytes[0] = static_cast<uint8_t>((key >> 24) & 0xFF);
        keyBytes[1] = static_cast<uint8_t>((key >> 16) & 0xFF);
        keyBytes[2] = static_cast<uint8_t>((key >> 8) & 0xFF);
        keyBytes[3] = static_cast<uint8_t>(key & 0xFF);

        io::ByteSpan keySpan(keyBytes, 4);
        fullKey.Append(keySpan);
        return persistence::StorageKey(scriptHash_, fullKey);
    }
}
