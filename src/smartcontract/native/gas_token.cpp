#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/storage_item.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <sstream>

namespace neo::smartcontract::native
{
    GasToken::GasToken()
        : FungibleToken(NAME, ID)
    {
    }

    std::string GasToken::GetSymbol() const
    {
        return "GAS";
    }

    uint8_t GasToken::GetDecimals() const
    {
        return 8;
    }

    std::shared_ptr<GasToken> GasToken::GetInstance()
    {
        static std::shared_ptr<GasToken> instance = std::make_shared<GasToken>();
        return instance;
    }

    void GasToken::Initialize()
    {
        RegisterMethod("symbol", CallFlags::ReadStates, std::bind(&GasToken::OnSymbol, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("decimals", CallFlags::ReadStates, std::bind(&GasToken::OnDecimals, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("totalSupply", CallFlags::ReadStates, std::bind(&GasToken::OnTotalSupply, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("balanceOf", CallFlags::ReadStates, std::bind(&GasToken::OnBalanceOf, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("transfer", CallFlags::All, std::bind(&GasToken::OnTransfer, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("onNEP17Payment", CallFlags::All, std::bind(&GasToken::OnNEP17Payment, this, std::placeholders::_1, std::placeholders::_2));
    }

    bool GasToken::InitializeContract(ApplicationEngine& engine, uint32_t hardfork)
    {
        // Check if this is the initial deployment
        if (hardfork == 0)
        {
            // Get the BFT address from the standby validators
            auto settings = engine.GetProtocolSettings();
            if (!settings)
                return false;

            auto validators = settings->GetStandbyValidators();
            if (validators.empty())
                return false;

            // Create a multi-signature contract with the validators
            io::UInt160 account;
            // TODO: Implement GetBFTAddress to get the multi-signature contract address

            // Mint the initial GAS distribution
            int64_t initialGasDistribution = settings->GetInitialGasDistribution();
            if (initialGasDistribution <= 0)
                initialGasDistribution = 5200000 * FACTOR; // Default: 5.2M GAS

            return Mint(engine, account, initialGasDistribution, false);
        }

        return true;
    }

    int64_t GasToken::GetBalance(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& account) const
    {
        auto key = GetStorageKey(PREFIX_BALANCE, account);
        auto value = GetStorageValue(snapshot, key);
        if (value.IsEmpty())
            return 0;

        return *reinterpret_cast<const int64_t*>(value.Data());
    }

    int64_t GasToken::GetTotalSupply(std::shared_ptr<persistence::StoreView> snapshot) const
    {
        auto key = GetStorageKey(PREFIX_TOTAL_SUPPLY, io::ByteVector{});
        auto value = GetStorageValue(snapshot, key);
        if (value.IsEmpty())
            return TOTAL_SUPPLY;

        return *reinterpret_cast<const int64_t*>(value.Data());
    }

    bool GasToken::Transfer(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& from, const io::UInt160& to, int64_t amount)
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

    bool GasToken::Transfer(ApplicationEngine& engine, const io::UInt160& from, const io::UInt160& to, int64_t amount, std::shared_ptr<vm::StackItem> data, bool callOnPayment)
    {
        if (amount <= 0)
            return false;

        // Transfer GAS
        bool result = Transfer(engine.GetSnapshot(), from, to, amount);

        if (result)
        {
            // Call PostTransfer
            PostTransfer(engine, from, to, amount, data, callOnPayment);
        }

        return result;
    }

    bool GasToken::PostTransfer(ApplicationEngine& engine, const io::UInt160& from, const io::UInt160& to, int64_t amount, std::shared_ptr<vm::StackItem> data, bool callOnPayment)
    {
        // Create the state for the Transfer event
        std::vector<std::shared_ptr<vm::StackItem>> state = {
            from.IsZero() ? vm::StackItem::Null() : vm::StackItem::Create(from),
            to.IsZero() ? vm::StackItem::Null() : vm::StackItem::Create(to),
            vm::StackItem::Create(amount)
        };

        // Send notification
        engine.Notify(GetScriptHash(), "Transfer", state);

        // Check if it's a wallet or smart contract
        if (!callOnPayment || to.IsZero())
            return true;

        // Check if the recipient is a contract
        auto contractManagement = engine.GetNativeContract(ContractManagement::ID);
        if (!contractManagement)
            return true;

        // Get the contract state
        auto contractState = contractManagement->Call(engine, "getContract", { vm::StackItem::Create(to) });
        if (!contractState || contractState->IsNull())
            return true;

        // Call onNEP17Payment method
        std::vector<std::shared_ptr<vm::StackItem>> args = {
            from.IsZero() ? vm::StackItem::Null() : vm::StackItem::Create(from),
            vm::StackItem::Create(amount),
            data
        };

        engine.CallContract(to, "onNEP17Payment", args);

        return true;
    }

    bool GasToken::Mint(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& account, int64_t amount)
    {
        if (amount <= 0)
            return false;

        // Update total supply
        int64_t totalSupply = GetTotalSupply(snapshot);
        int64_t newTotalSupply = totalSupply + amount;
        auto totalSupplyKey = GetStorageKey(PREFIX_TOTAL_SUPPLY, io::ByteVector{});
        io::ByteVector totalSupplyValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(&newTotalSupply), sizeof(int64_t)));
        PutStorageValue(snapshot, totalSupplyKey, totalSupplyValue);

        // Update account balance
        int64_t balance = GetBalance(snapshot, account);
        int64_t newBalance = balance + amount;
        auto balanceKey = GetStorageKey(PREFIX_BALANCE, account);
        io::ByteVector balanceValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(&newBalance), sizeof(int64_t)));
        PutStorageValue(snapshot, balanceKey, balanceValue);

        return true;
    }

    bool GasToken::Mint(ApplicationEngine& engine, const io::UInt160& account, int64_t amount, bool callOnPayment)
    {
        if (amount <= 0)
            return false;

        // Mint GAS
        bool result = Mint(engine.GetSnapshot(), account, amount);

        if (result)
        {
            // Call PostTransfer
            io::UInt160 nullAddress;
            std::memset(nullAddress.Data(), 0, nullAddress.Size());

            PostTransfer(engine, nullAddress, account, amount, vm::StackItem::Null(), callOnPayment);
        }

        return result;
    }

    bool GasToken::Burn(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& account, int64_t amount)
    {
        if (amount <= 0)
            return false;

        // Check if account has enough balance
        int64_t balance = GetBalance(snapshot, account);
        if (balance < amount)
            return false;

        // Update total supply
        int64_t totalSupply = GetTotalSupply(snapshot);
        int64_t newTotalSupply = totalSupply - amount;
        auto totalSupplyKey = GetStorageKey(PREFIX_TOTAL_SUPPLY, io::ByteVector{});
        io::ByteVector totalSupplyValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(&newTotalSupply), sizeof(int64_t)));
        PutStorageValue(snapshot, totalSupplyKey, totalSupplyValue);

        // Update account balance
        int64_t newBalance = balance - amount;
        auto balanceKey = GetStorageKey(PREFIX_BALANCE, account);
        if (newBalance > 0)
        {
            io::ByteVector balanceValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(&newBalance), sizeof(int64_t)));
            PutStorageValue(snapshot, balanceKey, balanceValue);
        }
        else
        {
            DeleteStorageValue(snapshot, balanceKey);
        }

        return true;
    }

    bool GasToken::Burn(ApplicationEngine& engine, const io::UInt160& account, int64_t amount)
    {
        if (amount <= 0)
            return false;

        // Burn GAS
        bool result = Burn(engine.GetSnapshot(), account, amount);

        if (result)
        {
            // Call PostTransfer
            io::UInt160 nullAddress;
            std::memset(nullAddress.Data(), 0, nullAddress.Size());

            PostTransfer(engine, account, nullAddress, amount, vm::StackItem::Null(), false);
        }

        return result;
    }

    int64_t GasToken::GetGasPerBlock(std::shared_ptr<persistence::StoreView> snapshot) const
    {
        auto key = GetStorageKey(PREFIX_GAS_PER_BLOCK, io::ByteVector{});
        auto value = GetStorageValue(snapshot, key);
        if (value.IsEmpty())
            return 5 * FACTOR; // Default: 5 GAS per block

        return *reinterpret_cast<const int64_t*>(value.Data());
    }

    void GasToken::SetGasPerBlock(std::shared_ptr<persistence::StoreView> snapshot, int64_t gasPerBlock)
    {
        auto key = GetStorageKey(PREFIX_GAS_PER_BLOCK, io::ByteVector{});
        io::ByteVector value(io::ByteSpan(reinterpret_cast<const uint8_t*>(&gasPerBlock), sizeof(int64_t)));
        PutStorageValue(snapshot, key, value);
    }

    std::shared_ptr<vm::StackItem> GasToken::OnSymbol(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        return vm::StackItem::Create("GAS");
    }

    std::shared_ptr<vm::StackItem> GasToken::OnDecimals(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        return vm::StackItem::Create(8);
    }

    std::shared_ptr<vm::StackItem> GasToken::OnTotalSupply(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        return vm::StackItem::Create(GetTotalSupply(engine.GetSnapshot()));
    }

    std::shared_ptr<vm::StackItem> GasToken::OnBalanceOf(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.empty())
            throw std::runtime_error("Invalid arguments");

        auto accountItem = args[0];
        auto accountBytes = accountItem->GetByteArray();

        if (accountBytes.Size() != 20)
            throw std::runtime_error("Invalid account");

        io::UInt160 account;
        std::memcpy(account.Data(), accountBytes.Data(), 20);

        return vm::StackItem::Create(GetBalance(engine.GetSnapshot(), account));
    }

    std::shared_ptr<vm::StackItem> GasToken::OnTransfer(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.size() < 3)
            throw std::runtime_error("Invalid arguments");

        auto fromItem = args[0];
        auto toItem = args[1];
        auto amountItem = args[2];

        auto fromBytes = fromItem->GetByteArray();
        auto toBytes = toItem->GetByteArray();
        auto amount = amountItem->GetInteger();

        if (fromBytes.Size() != 20 || toBytes.Size() != 20)
            throw std::runtime_error("Invalid account");

        io::UInt160 from;
        io::UInt160 to;
        std::memcpy(from.Data(), fromBytes.Data(), 20);
        std::memcpy(to.Data(), toBytes.Data(), 20);

        // Check if from account is the current script hash
        if (from != engine.GetCurrentScriptHash())
            throw std::runtime_error("Invalid from account");

        // Check if amount is valid
        if (amount <= 0)
            throw std::runtime_error("Invalid amount");

        // Get data if provided
        std::shared_ptr<vm::StackItem> data = vm::StackItem::Null();
        if (args.size() >= 4)
            data = args[3];

        // Transfer
        bool result = Transfer(engine, from, to, amount, data, true);

        return vm::StackItem::Create(result);
    }

    std::shared_ptr<vm::StackItem> GasToken::OnNEP17Payment(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        // GAS token doesn't accept NEP-17 payments
        throw std::runtime_error("Method not supported");
    }

    bool GasToken::OnPersist(ApplicationEngine& engine)
    {
        // Get the persisting block
        auto block = engine.GetPersistingBlock();
        if (!block)
            return false;

        // Process transaction fees
        int64_t totalNetworkFee = 0;
        auto transactions = block->GetTransactions();
        for (const auto& tx : transactions)
        {
            // Burn system fee and network fee from sender
            auto sender = tx->GetSender();
            int64_t totalFee = tx->GetSystemFee() + tx->GetNetworkFee();
            Burn(engine, sender, totalFee);

            // Add network fee to total
            totalNetworkFee += tx->GetNetworkFee();

            // Handle NotaryAssisted attribute
            auto notaryAssisted = tx->GetAttribute<NotaryAssisted>();
            if (notaryAssisted)
            {
                // Get the policy contract
                auto policyContract = engine.GetNativeContract(PolicyContract::ID);
                if (policyContract)
                {
                    // Get the attribute fee
                    auto attributeFeeResult = policyContract->Call(engine, "getAttributeFee", { vm::StackItem::Create(static_cast<int64_t>(notaryAssisted->GetType())) });
                    if (attributeFeeResult && attributeFeeResult->IsInteger())
                    {
                        int64_t attributeFee = attributeFeeResult->GetInteger();
                        totalNetworkFee -= (notaryAssisted->GetNKeys() + 1) * attributeFee;
                    }
                }
            }
        }

        return true;
    }

    bool GasToken::PostPersist(ApplicationEngine& engine)
    {
        // Get the persisting block
        auto block = engine.GetPersistingBlock();
        if (!block)
            return false;

        // Get the protocol settings
        auto settings = engine.GetProtocolSettings();
        if (!settings)
            return false;

        // Get the committee members count and validators count
        int committeeMembersCount = settings->GetCommitteeMembersCount();
        int validatorsCount = settings->GetValidatorsCount();

        if (committeeMembersCount <= 0 || validatorsCount <= 0)
            return false;

        // Calculate the index of the committee member to reward
        int index = static_cast<int>(block->GetIndex() % committeeMembersCount);

        // Get the gas per block
        int64_t gasPerBlock = GetGasPerBlock(engine.GetSnapshot());

        // Get the committee
        auto neoToken = engine.GetNativeContract(NeoToken::ID);
        if (!neoToken)
            return false;

        auto committeeResult = neoToken->Call(engine, "getCommittee", {});
        if (!committeeResult || !committeeResult->IsArray())
            return false;

        auto committee = committeeResult->GetArray();
        if (committee.size() <= index)
            return false;

        // Get the public key of the committee member
        auto pubkeyItem = committee[index];
        if (!pubkeyItem || !pubkeyItem->IsBuffer())
            return false;

        auto pubkey = pubkeyItem->GetByteArray();

        // Create a signature redeem script with the public key
        // TODO: Implement CreateSignatureRedeemScript
        io::UInt160 account;

        // Calculate the committee reward
        int64_t committeeReward = gasPerBlock * 10 / 100; // 10% of gas per block

        // Mint the committee reward
        Mint(engine, account, committeeReward, false);

        // Record the cumulative reward of the voters of committee
        // TODO: Implement voter rewards

        if (!validators.empty())
        {
            // Mint network fee to primary validator
            int primaryIndex = block->GetPrimaryIndex();
            if (primaryIndex >= 0 && primaryIndex < static_cast<int>(validators.size()))
            {
                auto pubKey = validators[primaryIndex];
                auto account = cryptography::Hash::Hash160(pubKey.ToArray().AsSpan());
                Mint(engine.GetSnapshot(), account, totalNetworkFee);
            }
        }

        // Get the gas per block
        int64_t gasPerBlock = GetGasPerBlock(engine.GetSnapshot());

        // Get the committee members
        auto committee = neoToken->GetCommittee(engine.GetSnapshot());
        if (committee.empty())
            return false;

        // Calculate gas per committee member
        int64_t gasPerMember = gasPerBlock / committee.size();
        if (gasPerMember <= 0)
            return false;

        // Store gas distribution for post persist
        auto key = GetStorageKey(PREFIX_GAS_DISTRIBUTION, io::ByteVector{});
        io::ByteVector value(io::ByteSpan(reinterpret_cast<const uint8_t*>(&gasPerMember), sizeof(int64_t)));
        PutStorageValue(engine.GetSnapshot(), key, value);

        return true;
    }

    bool GasToken::PostPersist(ApplicationEngine& engine)
    {
        // Get the persisting block
        auto block = engine.GetPersistingBlock();
        if (!block)
            return false;

        // Get the gas per member
        auto key = GetStorageKey(PREFIX_GAS_DISTRIBUTION, io::ByteVector{});
        auto value = GetStorageValue(engine.GetSnapshot(), key);
        if (value.IsEmpty())
            return false;

        int64_t gasPerMember = *reinterpret_cast<const int64_t*>(value.Data());
        if (gasPerMember <= 0)
            return false;

        // Get the committee members
        auto neoToken = NativeContract::GetContract<NeoToken>();
        auto committee = neoToken->GetCommittee(engine.GetSnapshot());
        if (committee.empty())
            return false;

        // Distribute gas to committee members
        for (const auto& member : committee)
        {
            // Convert public key to script hash
            io::UInt160 account = cryptography::Hash::Hash160(member.ToArray());

            // Mint gas for the committee member
            Mint(engine.GetSnapshot(), account, gasPerMember);
        }

        // Clean up gas distribution
        DeleteStorageValue(engine.GetSnapshot(), key);

        return true;
    }
}
