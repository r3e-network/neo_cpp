#include <neo/persistence/storage_item.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>

namespace neo::persistence
{
    StorageItem::StorageItem() = default;

    StorageItem::StorageItem(const io::ByteVector& value)
        : value_(value)
    {
    }

    const io::ByteVector& StorageItem::GetValue() const
    {
        return value_;
    }

    void StorageItem::SetValue(const io::ByteVector& value)
    {
        value_ = value;
    }

    void StorageItem::Serialize(io::BinaryWriter& writer) const
    {
        writer.WriteVarBytes(value_.AsSpan());
    }

    void StorageItem::Deserialize(io::BinaryReader& reader)
    {
        value_ = reader.ReadVarBytes();
    }

    bool StorageItem::operator==(const StorageItem& other) const
    {
        return value_ == other.value_;
    }

    bool StorageItem::operator!=(const StorageItem& other) const
    {
        return !(*this == other);
    }
}
