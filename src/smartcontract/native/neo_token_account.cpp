#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/neo_token_account.h>
#include <neo/vm/stack_item.h>
#include <sstream>
#include <stdexcept>

namespace neo::smartcontract::native
{
io::Fixed8 NeoTokenAccount::GetBalance(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot,
                                       const io::UInt160& account)
{
    auto state = GetAccountState(token, snapshot, account);
    return io::Fixed8(state.balance);
}

NeoToken::AccountState NeoTokenAccount::GetAccountState(const NeoToken& token,
                                                        std::shared_ptr<persistence::DataCache> snapshot,
                                                        const io::UInt160& account)
{
    persistence::StorageKey key =
        token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::Account), account);
    auto item = snapshot->TryGet(key);
    if (!item)
    {
        // Return default account state
        NeoToken::AccountState state;
        state.balance = 0;
        state.balanceHeight = 0;
        state.lastGasPerVote = 0;
        return state;
    }

    std::istringstream stream(
        std::string(reinterpret_cast<const char*>(item->GetValue().Data()), item->GetValue().Size()));
    io::BinaryReader reader(stream);

    NeoToken::AccountState state;
    state.Deserialize(reader);
    return state;
}

std::shared_ptr<vm::StackItem> NeoTokenAccount::OnBalanceOf(const NeoToken& token, ApplicationEngine& engine,
                                                            const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.size() < 1)
        throw std::runtime_error("Invalid number of arguments");

    auto accountItem = args[0];

    io::UInt160 account;
    auto accountBytes = accountItem->GetByteArray();
    if (accountBytes.Size() != 20)
        throw std::runtime_error("Invalid account");

    std::memcpy(account.Data(), accountBytes.Data(), 20);

    auto balance = GetBalance(token, engine.GetSnapshot(), account);

    return vm::StackItem::Create(balance.Value());
}

std::shared_ptr<vm::StackItem>
NeoTokenAccount::OnGetAccountState(const NeoToken& token, ApplicationEngine& engine,
                                   const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.size() < 1)
        throw std::runtime_error("Invalid number of arguments");

    auto accountItem = args[0];

    io::UInt160 account;
    auto accountBytes = accountItem->GetByteArray();
    if (accountBytes.Size() != 20)
        throw std::runtime_error("Invalid account");

    std::memcpy(account.Data(), accountBytes.Data(), 20);

    auto state = GetAccountState(token, engine.GetSnapshot(), account);

    // Create struct with account state
    std::vector<std::shared_ptr<vm::StackItem>> stateItems;
    stateItems.push_back(vm::StackItem::Create(static_cast<int64_t>(state.balance)));
    stateItems.push_back(vm::StackItem::Create(static_cast<int64_t>(state.balanceHeight)));
    stateItems.push_back(state.voteTo.IsInfinity() ? vm::StackItem::Null()
                                                   : vm::StackItem::Create(state.voteTo.ToArray()));
    stateItems.push_back(vm::StackItem::Create(static_cast<int64_t>(state.lastGasPerVote)));

    // Create a reference counter for the struct
    vm::ReferenceCounter refCounter;
    return vm::StackItem::CreateStruct(stateItems, refCounter);
}
}  // namespace neo::smartcontract::native
