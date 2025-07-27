#include <neo/smartcontract/native/fungible_token.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/contract_management.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/storage_item.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <cmath>
#include <cstring>

namespace neo::smartcontract::native
{
    FungibleToken::FungibleToken(const char* name, uint32_t id)
        : NativeContract(name, id)
    {
    }

    int64_t FungibleToken::GetFactor() const
    {
        return static_cast<int64_t>(std::pow(10, GetDecimals()));
    }

    int64_t FungibleToken::GetTotalSupply(std::shared_ptr<persistence::StoreView> snapshot) const
    {
        auto key = GetStorageKey(PREFIX_TOTAL_SUPPLY, io::ByteVector{});
        auto value = GetStorageValue(snapshot, key);
        if (value.IsEmpty())
            return 0;

        return *reinterpret_cast<const int64_t*>(value.Data());
    }

    int64_t FungibleToken::GetBalance(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& account) const
    {
        auto key = GetStorageKey(PREFIX_BALANCE, account);
        auto value = GetStorageValue(snapshot, key);
        if (value.IsEmpty())
            return 0;

        return *reinterpret_cast<const int64_t*>(value.Data());
    }

    bool FungibleToken::Transfer(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& from, const io::UInt160& to, int64_t amount)
    {
        if (amount <= 0)
            return false;

        // Check if from account has enough balance
        int64_t fromBalance = GetBalance(snapshot, from);
        if (fromBalance < amount)
            return false;

        // Update from account balance
        int64_t newFromBalance = fromBalance - amount;
        auto fromKey = GetStorageKey(PREFIX_BALANCE, from);
        if (newFromBalance > 0)
        {
            io::ByteVector fromValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(&newFromBalance), sizeof(int64_t)));
            PutStorageValue(snapshot, fromKey, fromValue);
        }
        else
        {
            DeleteStorageValue(snapshot, fromKey);
        }

        // Update to account balance
        int64_t toBalance = GetBalance(snapshot, to);
        int64_t newToBalance = toBalance + amount;
        auto toKey = GetStorageKey(PREFIX_BALANCE, to);
        io::ByteVector toValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(&newToBalance), sizeof(int64_t)));
        PutStorageValue(snapshot, toKey, toValue);

        return true;
    }

    bool FungibleToken::Transfer(ApplicationEngine& engine, const io::UInt160& from, const io::UInt160& to, int64_t amount, std::shared_ptr<vm::StackItem> data, bool callOnPayment)
    {
        if (amount <= 0)
            return false;

        // Check if the caller is the owner of the tokens
        if (!from.IsZero() && from != engine.GetCurrentScriptHash() && !engine.CheckWitness(from))
            return false;

        // Transfer tokens
        bool result = Transfer(engine.GetSnapshot(), from, to, amount);

        if (result)
        {
            // Call PostTransfer
            PostTransfer(engine, from, to, amount, data, callOnPayment);
        }

        return result;
    }

    bool FungibleToken::Mint(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& account, int64_t amount)
    {
        if (amount <= 0)
            return false;

        // Update account balance
        int64_t balance = GetBalance(snapshot, account);
        int64_t newBalance = balance + amount;
        auto key = GetStorageKey(PREFIX_BALANCE, account);
        io::ByteVector value(io::ByteSpan(reinterpret_cast<const uint8_t*>(&newBalance), sizeof(int64_t)));
        PutStorageValue(snapshot, key, value);

        // Update total supply
        int64_t totalSupply = GetTotalSupply(snapshot);
        int64_t newTotalSupply = totalSupply + amount;
        auto totalSupplyKey = GetStorageKey(PREFIX_TOTAL_SUPPLY, io::ByteVector{});
        io::ByteVector totalSupplyValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(&newTotalSupply), sizeof(int64_t)));
        PutStorageValue(snapshot, totalSupplyKey, totalSupplyValue);

        return true;
    }

    bool FungibleToken::Mint(ApplicationEngine& engine, const io::UInt160& account, int64_t amount, bool callOnPayment)
    {
        if (amount <= 0)
            return false;

        // Mint tokens
        bool result = Mint(engine.GetSnapshot(), account, amount);

        if (result)
        {
            // Call PostTransfer
            io::UInt160 nullAddress;
            std::memset(nullAddress.Data(), 0, 20); // UInt160 is always 20 bytes

            PostTransfer(engine, nullAddress, account, amount, vm::StackItem::Null(), callOnPayment);
        }

        return result;
    }

    bool FungibleToken::Burn(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& account, int64_t amount)
    {
        if (amount <= 0)
            return false;

        // Check if account has enough balance
        int64_t balance = GetBalance(snapshot, account);
        if (balance < amount)
            return false;

        // Update account balance
        int64_t newBalance = balance - amount;
        auto key = GetStorageKey(PREFIX_BALANCE, account);
        if (newBalance > 0)
        {
            io::ByteVector value(io::ByteSpan(reinterpret_cast<const uint8_t*>(&newBalance), sizeof(int64_t)));
            PutStorageValue(snapshot, key, value);
        }
        else
        {
            DeleteStorageValue(snapshot, key);
        }

        // Update total supply
        int64_t totalSupply = GetTotalSupply(snapshot);
        int64_t newTotalSupply = totalSupply - amount;
        auto totalSupplyKey = GetStorageKey(PREFIX_TOTAL_SUPPLY, io::ByteVector{});
        io::ByteVector totalSupplyValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(&newTotalSupply), sizeof(int64_t)));
        PutStorageValue(snapshot, totalSupplyKey, totalSupplyValue);

        return true;
    }

    bool FungibleToken::Burn(ApplicationEngine& engine, const io::UInt160& account, int64_t amount)
    {
        if (amount <= 0)
            return false;

        // Burn tokens
        bool result = Burn(engine.GetSnapshot(), account, amount);

        if (result)
        {
            // Call PostTransfer
            io::UInt160 nullAddress;
            std::memset(nullAddress.Data(), 0, 20); // UInt160 is always 20 bytes

            PostTransfer(engine, account, nullAddress, amount, vm::StackItem::Null(), false);
        }

        return result;
    }

    bool FungibleToken::PostTransfer(ApplicationEngine& engine, const io::UInt160& from, const io::UInt160& to, int64_t amount, std::shared_ptr<vm::StackItem> data, bool callOnPayment)
    {
        // Create the state for the Transfer event
        std::vector<std::shared_ptr<vm::StackItem>> state;
        state.push_back(from.IsZero() ? vm::StackItem::Null() : vm::StackItem::Create(from));
        state.push_back(to.IsZero() ? vm::StackItem::Null() : vm::StackItem::Create(to));
        state.push_back(vm::StackItem::Create(amount));

        // Send notification
        engine.Notify(GetScriptHash(), "Transfer", state);

        // Check if it's a wallet or smart contract
        if (!callOnPayment || to.IsZero())
            return true;

        // Check if the recipient is a contract
        auto contractManagement = dynamic_cast<ContractManagement*>(engine.GetNativeContract(ContractManagement::GetInstance()->GetScriptHash()));
        if (!contractManagement)
            return true;

        auto contract = static_cast<ContractManagement*>(contractManagement)->GetContract(engine.GetSnapshot(), to);
        if (!contract)
            return true;

        // Call onNEP17Payment method
        std::vector<std::shared_ptr<vm::StackItem>> args;
        args.push_back(from.IsZero() ? vm::StackItem::Null() : vm::StackItem::Create(from));
        args.push_back(vm::StackItem::Create(amount));
        args.push_back(data);

        engine.CallContract(to, "onNEP17Payment", args, CallFlags::All);

        return true;
    }

    void FungibleToken::OnBalanceChanging(ApplicationEngine& engine, const io::UInt160& account, int64_t amount)
    {
        // Do nothing by default
    }
}
