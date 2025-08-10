#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/smartcontract/native/neo_account_state.h>
#include <neo/vm/compound_items.h>
#include <neo/vm/stack_item.h>

namespace neo::smartcontract::native
{
NeoAccountState::NeoAccountState() : AccountState(0), balanceHeight_(0), lastGasPerVote_(0) {}

NeoAccountState::NeoAccountState(int64_t balance) : AccountState(balance), balanceHeight_(0), lastGasPerVote_(0) {}

NeoAccountState::~NeoAccountState() = default;

uint32_t NeoAccountState::GetBalanceHeight() const { return balanceHeight_; }

void NeoAccountState::SetBalanceHeight(uint32_t height) { balanceHeight_ = height; }

const cryptography::ecc::ECPoint& NeoAccountState::GetVoteTo() const { return voteTo_; }

void NeoAccountState::SetVoteTo(const cryptography::ecc::ECPoint& voteTo) { voteTo_ = voteTo; }

int64_t NeoAccountState::GetLastGasPerVote() const { return lastGasPerVote_; }

void NeoAccountState::SetLastGasPerVote(int64_t lastGasPerVote) { lastGasPerVote_ = lastGasPerVote; }

void NeoAccountState::Deserialize(io::BinaryReader& reader)
{
    AccountState::Deserialize(reader);
    balanceHeight_ = reader.ReadUInt32();

    bool hasVoteTo = reader.ReadBool();
    if (hasVoteTo)
    {
        auto voteToBytes = reader.ReadVarBytes(33);
        voteTo_ = cryptography::ecc::ECPoint::FromBytes(voteToBytes.AsSpan(), "secp256r1");
    }

    lastGasPerVote_ = reader.ReadInt64();
}

void NeoAccountState::Serialize(io::BinaryWriter& writer) const
{
    AccountState::Serialize(writer);
    writer.Write(balanceHeight_);

    bool hasVoteTo = !voteTo_.IsInfinity();
    writer.Write(hasVoteTo);
    if (hasVoteTo)
    {
        auto voteToBytes = voteTo_.ToArray();
        writer.WriteVarBytes(voteToBytes.AsSpan());
    }

    writer.Write(lastGasPerVote_);
}

std::shared_ptr<vm::StackItem> NeoAccountState::ToStackItem() const
{
    std::vector<std::shared_ptr<vm::StackItem>> items;
    items.push_back(vm::StackItem::Create(GetBalance()));
    items.push_back(vm::StackItem::Create(static_cast<int64_t>(balanceHeight_)));

    if (voteTo_.IsInfinity())
        items.push_back(vm::StackItem::Null());
    else
        items.push_back(vm::StackItem::Create(voteTo_.ToArray()));

    items.push_back(vm::StackItem::Create(lastGasPerVote_));

    return std::make_shared<vm::StructItem>(items, nullptr);
}

void NeoAccountState::FromStackItem(const std::shared_ptr<vm::StackItem>& item)
{
    if (!item->IsStruct()) throw std::runtime_error("Invalid stack item type");

    auto structItem = std::dynamic_pointer_cast<vm::Struct>(item);
    if (structItem->Count() < 4) throw std::runtime_error("Invalid stack item count");

    SetBalance(structItem->Get(0)->GetInteger());
    balanceHeight_ = static_cast<uint32_t>(structItem->Get(1)->GetInteger());

    if (structItem->Get(2)->IsNull())
        voteTo_ = cryptography::ecc::ECPoint::Infinity();
    else
    {
        auto voteToBytes = structItem->Get(2)->GetByteArray();
        voteTo_ = cryptography::ecc::ECPoint::FromBytes(voteToBytes.AsSpan(), "secp256r1");
    }

    lastGasPerVote_ = structItem->Get(3)->GetInteger();
}
}  // namespace neo::smartcontract::native
