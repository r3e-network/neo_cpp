#include <neo/persistence/storage_key.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <sstream>

namespace neo::persistence
{
    StorageKey::StorageKey() = default;

    StorageKey::StorageKey(const io::UInt160& scriptHash)
        : scriptHash_(scriptHash)
    {
    }

    StorageKey::StorageKey(const io::UInt160& scriptHash, const io::ByteVector& key)
        : scriptHash_(scriptHash), key_(key)
    {
    }

    StorageKey::StorageKey(const io::ByteVector& data)
    {
        std::stringstream ss;
        ss.write(reinterpret_cast<const char*>(data.Data()), data.Size());
        io::BinaryReader reader(ss);
        Deserialize(reader);
    }

    const io::UInt160& StorageKey::GetScriptHash() const
    {
        return scriptHash_;
    }

    const io::ByteVector& StorageKey::GetKey() const
    {
        return key_;
    }

    void StorageKey::Serialize(io::BinaryWriter& writer) const
    {
        writer.Write(scriptHash_);
        writer.WriteVarBytes(key_.AsSpan());
    }

    void StorageKey::Deserialize(io::BinaryReader& reader)
    {
        scriptHash_ = reader.ReadUInt160();
        key_ = reader.ReadVarBytes();
    }

    bool StorageKey::operator==(const StorageKey& other) const
    {
        return scriptHash_ == other.scriptHash_ && key_ == other.key_;
    }

    bool StorageKey::operator!=(const StorageKey& other) const
    {
        return !(*this == other);
    }

    bool StorageKey::operator<(const StorageKey& other) const
    {
        if (scriptHash_ < other.scriptHash_)
            return true;
        
        if (scriptHash_ > other.scriptHash_)
            return false;
        
        return key_ < other.key_;
    }
}
