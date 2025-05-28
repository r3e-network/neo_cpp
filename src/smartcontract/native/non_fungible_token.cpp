#include <neo/smartcontract/native/non_fungible_token.h>
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
    NonFungibleToken::NonFungibleToken(const char* name, uint32_t id)
        : NativeContract(name, id)
    {
    }

    uint8_t NonFungibleToken::GetDecimals() const
    {
        return 0;
    }

    int64_t NonFungibleToken::GetTotalSupply(std::shared_ptr<persistence::StoreView> snapshot) const
    {
        auto key = GetStorageKey(PREFIX_SUPPLY, io::ByteVector{});
        auto value = GetStorageValue(snapshot, key);
        if (value.IsEmpty())
            return 0;

        return *reinterpret_cast<const int64_t*>(value.Data());
    }

    int64_t NonFungibleToken::GetBalanceOf(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& account) const
    {
        auto key = GetStorageKey(PREFIX_BALANCE, account);
        auto value = GetStorageValue(snapshot, key);
        if (value.IsEmpty())
            return 0;

        return *reinterpret_cast<const int64_t*>(value.Data());
    }

    io::UInt160 NonFungibleToken::GetOwnerOf(std::shared_ptr<persistence::StoreView> snapshot, const io::ByteVector& tokenId) const
    {
        auto key = GetStorageKey(PREFIX_OWNER, tokenId);
        auto value = GetStorageValue(snapshot, key);
        if (value.IsEmpty())
            return io::UInt160{};

        io::UInt160 owner;
        std::memcpy(owner.Data(), value.Data(), owner.Size);
        return owner;
    }

    std::map<std::string, std::shared_ptr<vm::StackItem>> NonFungibleToken::GetProperties(std::shared_ptr<persistence::StoreView> snapshot, const io::ByteVector& tokenId) const
    {
        std::map<std::string, std::shared_ptr<vm::StackItem>> properties;
        auto key = GetStorageKey(PREFIX_PROPERTIES, tokenId);
        auto value = GetStorageValue(snapshot, key);
        if (value.IsEmpty())
            return properties;

        // Deserialize properties
        std::istringstream stream(std::string(reinterpret_cast<const char*>(value.Data()), value.Size()));
        io::BinaryReader reader(stream);
        int count = reader.ReadVarInt();
        for (int i = 0; i < count; i++)
        {
            std::string name = reader.ReadVarString();
            auto item = vm::StackItem::Deserialize(reader);
            properties[name] = item;
        }

        return properties;
    }

    std::vector<io::ByteVector> NonFungibleToken::GetTokens(std::shared_ptr<persistence::StoreView> snapshot) const
    {
        std::vector<io::ByteVector> tokens;
        auto prefix = GetStorageKey(PREFIX_TOKEN, io::ByteVector{});
        auto iterator = snapshot->Seek(prefix, persistence::SeekDirection::Forward);
        while (iterator->Next())
        {
            auto key = iterator->GetKey();
            if (!key.StartsWith(prefix))
                break;

            auto tokenId = key.GetData().SubVector(prefix.Size());
            tokens.push_back(tokenId);
        }

        return tokens;
    }

    std::vector<io::ByteVector> NonFungibleToken::GetTokensOf(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& account) const
    {
        std::vector<io::ByteVector> tokens;
        auto prefix = GetStorageKey(PREFIX_ACCOUNT_TOKEN, account);
        auto iterator = snapshot->Seek(prefix, persistence::SeekDirection::Forward);
        while (iterator->Next())
        {
            auto key = iterator->GetKey();
            if (!key.StartsWith(prefix))
                break;

            auto tokenId = key.GetData().SubVector(prefix.Size());
            tokens.push_back(tokenId);
        }

        return tokens;
    }

    bool NonFungibleToken::Transfer(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& from, const io::UInt160& to, const io::ByteVector& tokenId)
    {
        // Check if token exists
        auto owner = GetOwnerOf(snapshot, tokenId);
        if (owner.IsZero())
            return false;

        // Check if from is the owner
        if (owner != from)
            return false;

        // Update owner
        auto ownerKey = GetStorageKey(PREFIX_OWNER, tokenId);
        io::ByteVector ownerValue(io::ByteSpan(to.Data(), to.Size()));
        PutStorageValue(snapshot, ownerKey, ownerValue);

        // Update from balance
        auto fromBalanceKey = GetStorageKey(PREFIX_BALANCE, from);
        auto fromBalance = GetBalanceOf(snapshot, from);
        int64_t newFromBalance = fromBalance - 1;
        if (newFromBalance > 0)
        {
            io::ByteVector fromBalanceValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(&newFromBalance), sizeof(int64_t)));
            PutStorageValue(snapshot, fromBalanceKey, fromBalanceValue);
        }
        else
        {
            DeleteStorageValue(snapshot, fromBalanceKey);
        }

        // Update to balance
        auto toBalanceKey = GetStorageKey(PREFIX_BALANCE, to);
        auto toBalance = GetBalanceOf(snapshot, to);
        int64_t newToBalance = toBalance + 1;
        io::ByteVector toBalanceValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(&newToBalance), sizeof(int64_t)));
        PutStorageValue(snapshot, toBalanceKey, toBalanceValue);

        // Remove token from from's account
        io::ByteVector fromAccountData;
        fromAccountData.insert(fromAccountData.end(), from.Data(), from.Data() + from.Size);
        fromAccountData.insert(fromAccountData.end(), tokenId.begin(), tokenId.end());
        auto fromTokenKey = GetStorageKey(PREFIX_ACCOUNT_TOKEN, fromAccountData);
        DeleteStorageValue(snapshot, fromTokenKey);

        // Add token to to's account
        io::ByteVector toAccountData;
        toAccountData.insert(toAccountData.end(), to.Data(), to.Data() + to.Size);
        toAccountData.insert(toAccountData.end(), tokenId.begin(), tokenId.end());
        auto toTokenKey = GetStorageKey(PREFIX_ACCOUNT_TOKEN, toAccountData);
        PutStorageValue(snapshot, toTokenKey, io::ByteVector{});

        return true;
    }

    bool NonFungibleToken::Transfer(ApplicationEngine& engine, const io::UInt160& from, const io::UInt160& to, const io::ByteVector& tokenId, std::shared_ptr<vm::StackItem> data, bool callOnPayment)
    {
        // Check if the caller is the owner of the token
        if (!from.IsZero() && from != engine.GetCurrentScriptHash() && !engine.CheckWitness(from))
            return false;

        // Transfer token
        bool result = Transfer(engine.GetSnapshot(), from, to, tokenId);

        if (result)
        {
            // Call PostTransfer
            PostTransfer(engine, from, to, 1, tokenId, data, callOnPayment);
        }

        return result;
    }

    bool NonFungibleToken::Mint(std::shared_ptr<persistence::StoreView> snapshot, const io::ByteVector& tokenId, const io::UInt160& owner, const std::map<std::string, std::shared_ptr<vm::StackItem>>& properties)
    {
        // Check if token already exists
        auto existingOwner = GetOwnerOf(snapshot, tokenId);
        if (!existingOwner.IsZero())
            return false;

        // Update owner
        auto ownerKey = GetStorageKey(PREFIX_OWNER, tokenId);
        io::ByteVector ownerValue(io::ByteSpan(owner.Data(), owner.Size()));
        PutStorageValue(snapshot, ownerKey, ownerValue);

        // Update properties
        auto propertiesKey = GetStorageKey(PREFIX_PROPERTIES, tokenId);
        std::ostringstream stream;
        io::BinaryWriter writer(stream);
        writer.WriteVarInt(properties.size());
        for (const auto& [name, value] : properties)
        {
            writer.WriteVarString(name);
            vm::StackItem::Serialize(value, writer);
        }
        std::string data = stream.str();
        io::ByteVector propertiesValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
        PutStorageValue(snapshot, propertiesKey, propertiesValue);

        // Update balance
        auto balanceKey = GetStorageKey(PREFIX_BALANCE, owner);
        auto balance = GetBalanceOf(snapshot, owner);
        int64_t newBalance = balance + 1;
        io::ByteVector balanceValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(&newBalance), sizeof(int64_t)));
        PutStorageValue(snapshot, balanceKey, balanceValue);

        // Update total supply
        auto supplyKey = GetStorageKey(PREFIX_SUPPLY, io::ByteVector{});
        auto supply = GetTotalSupply(snapshot);
        int64_t newSupply = supply + 1;
        io::ByteVector supplyValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(&newSupply), sizeof(int64_t)));
        PutStorageValue(snapshot, supplyKey, supplyValue);

        // Add token to tokens list
        auto tokenKey = GetStorageKey(PREFIX_TOKEN, tokenId);
        PutStorageValue(snapshot, tokenKey, io::ByteVector{});

        // Add token to owner's account
        io::ByteVector accountData;
        accountData.insert(accountData.end(), owner.Data(), owner.Data() + owner.Size);
        accountData.insert(accountData.end(), tokenId.begin(), tokenId.end());
        auto accountTokenKey = GetStorageKey(PREFIX_ACCOUNT_TOKEN, accountData);
        PutStorageValue(snapshot, accountTokenKey, io::ByteVector{});

        return true;
    }

    bool NonFungibleToken::Mint(ApplicationEngine& engine, const io::ByteVector& tokenId, const io::UInt160& owner, const std::map<std::string, std::shared_ptr<vm::StackItem>>& properties, std::shared_ptr<vm::StackItem> data, bool callOnPayment)
    {
        // Mint token
        bool result = Mint(engine.GetSnapshot(), tokenId, owner, properties);

        if (result)
        {
            // Call PostTransfer
            io::UInt160 nullAddress;
            std::memset(nullAddress.Data(), 0, nullAddress.Size);

            PostTransfer(engine, nullAddress, owner, 1, tokenId, data, callOnPayment);
        }

        return result;
    }

    bool NonFungibleToken::Burn(std::shared_ptr<persistence::StoreView> snapshot, const io::ByteVector& tokenId)
    {
        // Check if token exists
        auto owner = GetOwnerOf(snapshot, tokenId);
        if (owner.IsZero())
            return false;

        // Update owner
        auto ownerKey = GetStorageKey(PREFIX_OWNER, tokenId);
        DeleteStorageValue(snapshot, ownerKey);

        // Update properties
        auto propertiesKey = GetStorageKey(PREFIX_PROPERTIES, tokenId);
        DeleteStorageValue(snapshot, propertiesKey);

        // Update balance
        auto balanceKey = GetStorageKey(PREFIX_BALANCE, owner);
        auto balance = GetBalanceOf(snapshot, owner);
        int64_t newBalance = balance - 1;
        if (newBalance > 0)
        {
            io::ByteVector balanceValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(&newBalance), sizeof(int64_t)));
            PutStorageValue(snapshot, balanceKey, balanceValue);
        }
        else
        {
            DeleteStorageValue(snapshot, balanceKey);
        }

        // Update total supply
        auto supplyKey = GetStorageKey(PREFIX_SUPPLY, io::ByteVector{});
        auto supply = GetTotalSupply(snapshot);
        int64_t newSupply = supply - 1;
        io::ByteVector supplyValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(&newSupply), sizeof(int64_t)));
        PutStorageValue(snapshot, supplyKey, supplyValue);

        // Remove token from tokens list
        auto tokenKey = GetStorageKey(PREFIX_TOKEN, tokenId);
        DeleteStorageValue(snapshot, tokenKey);

        // Remove token from owner's account
        io::ByteVector accountData;
        accountData.insert(accountData.end(), owner.Data(), owner.Data() + owner.Size);
        accountData.insert(accountData.end(), tokenId.begin(), tokenId.end());
        auto accountTokenKey = GetStorageKey(PREFIX_ACCOUNT_TOKEN, accountData);
        DeleteStorageValue(snapshot, accountTokenKey);

        return true;
    }

    bool NonFungibleToken::Burn(ApplicationEngine& engine, const io::ByteVector& tokenId)
    {
        // Check if token exists
        auto owner = GetOwnerOf(engine.GetSnapshot(), tokenId);
        if (owner.IsZero())
            return false;

        // Check if the caller is the owner of the token
        if (!engine.CheckWitness(owner))
            return false;

        // Burn token
        bool result = Burn(engine.GetSnapshot(), tokenId);

        if (result)
        {
            // Call PostTransfer
            io::UInt160 nullAddress;
            std::memset(nullAddress.Data(), 0, nullAddress.Size);

            PostTransfer(engine, owner, nullAddress, 1, tokenId, vm::StackItem::Null(), false);
        }

        return result;
    }

    bool NonFungibleToken::PostTransfer(ApplicationEngine& engine, const io::UInt160& from, const io::UInt160& to, int64_t amount, const io::ByteVector& tokenId, std::shared_ptr<vm::StackItem> data, bool callOnPayment)
    {
        // Create the state for the Transfer event
        std::vector<std::shared_ptr<vm::StackItem>> state;
        state.push_back(from.IsZero() ? vm::StackItem::Null() : vm::StackItem::Create(from));
        state.push_back(to.IsZero() ? vm::StackItem::Null() : vm::StackItem::Create(to));
        state.push_back(vm::StackItem::Create(amount));
        state.push_back(vm::StackItem::Create(tokenId));

        // Send notification
        engine.Notify(GetScriptHash(), "Transfer", state);

        // Check if it's a wallet or smart contract
        if (!callOnPayment || to.IsZero())
            return true;

        // Check if the recipient is a contract
        auto contractManagement = engine.GetNativeContract(ContractManagement::GetScriptHash());
        if (!contractManagement)
            return true;

        // Get the contract state
        std::vector<std::shared_ptr<vm::StackItem>> getContractArgs;
        getContractArgs.push_back(vm::StackItem::Create(to));
        auto contractState = static_cast<ContractManagement*>(contractManagement)->GetContract(engine.GetSnapshot(), to);
        if (!contractState)
            return true;

        // Call onNEP11Payment method
        std::vector<std::shared_ptr<vm::StackItem>> args;
        args.push_back(from.IsZero() ? vm::StackItem::Null() : vm::StackItem::Create(from));
        args.push_back(vm::StackItem::Create(amount));
        args.push_back(vm::StackItem::Create(tokenId));
        args.push_back(data);

        engine.CallContract(to, "onNEP11Payment", args);

        return true;
    }
}
