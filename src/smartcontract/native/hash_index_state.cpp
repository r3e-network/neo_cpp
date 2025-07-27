#include <cstring>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/smartcontract/native/hash_index_state.h>
#include <neo/vm/compound_items.h>
#include <neo/vm/stack_item.h>

namespace neo::smartcontract::native
{
HashIndexState::HashIndexState() : index_(0) {}

HashIndexState::HashIndexState(const io::UInt256& hash, uint32_t index) : hash_(hash), index_(index) {}

const io::UInt256& HashIndexState::GetHash() const
{
    return hash_;
}

void HashIndexState::SetHash(const io::UInt256& hash)
{
    hash_ = hash;
}

uint32_t HashIndexState::GetIndex() const
{
    return index_;
}

void HashIndexState::SetIndex(uint32_t index)
{
    index_ = index;
}

void HashIndexState::Deserialize(io::BinaryReader& reader)
{
    hash_ = reader.ReadSerializable<io::UInt256>();
    index_ = reader.ReadUInt32();
}

void HashIndexState::Serialize(io::BinaryWriter& writer) const
{
    writer.Write(hash_);
    writer.Write(index_);
}

std::shared_ptr<vm::StackItem> HashIndexState::ToStackItem() const
{
    std::vector<std::shared_ptr<vm::StackItem>> items;
    items.push_back(vm::StackItem::Create(io::ByteVector(io::ByteSpan(hash_.Data(), io::UInt256::Size))));
    items.push_back(vm::StackItem::Create(static_cast<int64_t>(index_)));
    return std::make_shared<vm::StructItem>(items, nullptr);
}

void HashIndexState::FromStackItem(const std::shared_ptr<vm::StackItem>& item)
{
    if (!item->IsStruct())
        throw std::runtime_error("Invalid stack item type");

    auto structItem = std::dynamic_pointer_cast<vm::Struct>(item);
    if (structItem->Count() < 2)
        throw std::runtime_error("Invalid stack item count");

    auto hashBytes = structItem->Get(0)->GetByteArray();
    if (hashBytes.Size() != 32)
        throw std::runtime_error("Invalid hash size");

    std::memcpy(hash_.Data(), hashBytes.Data(), 32);
    index_ = static_cast<uint32_t>(structItem->Get(1)->GetInteger());
}
}  // namespace neo::smartcontract::native
