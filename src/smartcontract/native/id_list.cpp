#include <neo/smartcontract/native/id_list.h>

#include <algorithm>

namespace neo::smartcontract::native
{
IdList::IdList() {}

size_t IdList::GetCount() const { return ids_.size(); }

void IdList::Add(uint64_t id)
{
    if (!Contains(id)) ids_.push_back(id);
}

bool IdList::Remove(uint64_t id)
{
    auto it = std::find(ids_.begin(), ids_.end(), id);
    if (it == ids_.end()) return false;

    ids_.erase(it);
    return true;
}

bool IdList::Contains(uint64_t id) const { return std::find(ids_.begin(), ids_.end(), id) != ids_.end(); }

const std::vector<uint64_t>& IdList::GetIds() const { return ids_; }

std::shared_ptr<vm::StackItem> IdList::ToStackItem() const
{
    std::vector<std::shared_ptr<vm::StackItem>> items;
    for (const auto& id : ids_)
    {
        items.push_back(vm::StackItem::Create(static_cast<int64_t>(id)));
    }
    return vm::StackItem::CreateArray(items);
}

void IdList::FromStackItem(const std::shared_ptr<vm::StackItem>& item)
{
    if (!item->IsArray()) throw std::runtime_error("Expected array");

    auto array = item->GetArray();
    ids_.clear();
    ids_.reserve(array.size());
    for (const auto& element : array)
    {
        ids_.push_back(static_cast<uint64_t>(element->GetInteger()));
    }
}

void IdList::Serialize(io::BinaryWriter& writer) const
{
    writer.WriteVarInt(ids_.size());
    for (const auto& id : ids_)
    {
        writer.Write(id);
    }
}

void IdList::Deserialize(io::BinaryReader& reader)
{
    auto count = reader.ReadVarInt();
    ids_.clear();
    ids_.reserve(count);
    for (size_t i = 0; i < count; i++)
    {
        ids_.push_back(reader.ReadUInt64());
    }
}
}  // namespace neo::smartcontract::native
