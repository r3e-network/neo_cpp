#include <neo/smartcontract/native/account_state.h>
#include <neo/vm/compound_items.h>

namespace neo::smartcontract::native
{
AccountState::AccountState() : balance_(0) {}

AccountState::AccountState(int64_t balance) : balance_(balance) {}

int64_t AccountState::GetBalance() const
{
    return balance_;
}

void AccountState::SetBalance(int64_t balance)
{
    balance_ = balance;
}

void AccountState::Deserialize(io::BinaryReader& reader)
{
    balance_ = reader.ReadInt64();
}

void AccountState::Serialize(io::BinaryWriter& writer) const
{
    writer.Write(balance_);
}

std::shared_ptr<vm::StackItem> AccountState::ToStackItem() const
{
    std::vector<std::shared_ptr<vm::StackItem>> items;
    items.push_back(vm::StackItem::Create(balance_));
    return std::make_shared<vm::StructItem>(items, nullptr);
}

void AccountState::FromStackItem(const std::shared_ptr<vm::StackItem>& item)
{
    if (!item->IsArray())
        throw std::runtime_error("Expected array");

    auto array = item->GetArray();
    if (array.empty())
        throw std::runtime_error("Invalid array size");

    balance_ = array[0]->GetInteger();
}
}  // namespace neo::smartcontract::native
