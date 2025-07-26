#include <neo/smartcontract/native/neo_token_gas.h>
#include <neo/smartcontract/native/neo_token_account.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/storage_item.h>
#include <neo/vm/stack_item.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <sstream>
#include <stdexcept>
#include <iostream>

namespace neo::smartcontract::native
{
    int64_t NeoTokenGas::GetGasPerBlock(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot)
    {
        persistence::StorageKey key = token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::GasPerBlock));
        auto item = snapshot->TryGet(key);
        if (!item)
            return 5 * 100000000; // 5 GAS

        std::istringstream stream(std::string(reinterpret_cast<const char*>(item->GetValue().Data()), item->GetValue().Size()));
        io::BinaryReader reader(stream);
        return reader.ReadInt64();
    }

    void NeoTokenGas::SetGasPerBlock(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot, int64_t gasPerBlock)
    {
        if (gasPerBlock < 0 || gasPerBlock > 10 * 100000000) // 0-10 GAS
            throw std::runtime_error("Gas per block out of range");

        std::ostringstream stream;
        io::BinaryWriter writer(stream);
        writer.Write(gasPerBlock);
        std::string data = stream.str();

        persistence::StorageKey key = token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::GasPerBlock));
        persistence::StorageItem item(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size())));
        snapshot->Add(key, item);
    }

    int64_t NeoTokenGas::GetUnclaimedGas(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot, const io::UInt160& account, uint32_t end)
    {
        auto state = NeoTokenAccount::GetAccountState(token, snapshot, account);
        return CalculateBonus(token, snapshot, state, end);
    }

    NeoToken::GasDistribution NeoTokenGas::DistributeGas(const NeoToken& token, ApplicationEngine& engine, const io::UInt160& account, const NeoToken::AccountState& state)
    {
        // Get the current block
        auto block = engine.GetPersistingBlock();
        if (!block)
            return NeoToken::GasDistribution{account, 0};

        // Calculate unclaimed gas
        int64_t gas = CalculateBonus(token, engine.GetSnapshot(), state, block->GetIndex());
        if (gas <= 0)
            return NeoToken::GasDistribution{account, 0};

        // Mint GAS to account
        auto gasToken = GasToken::GetInstance();
        gasToken->Mint(engine, account, gas, true);

        return NeoToken::GasDistribution{account, gas};
    }

    int64_t NeoTokenGas::CalculateBonus(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot, const NeoToken::AccountState& state, uint32_t end)
    {
        if (state.balance <= 0 || state.balanceHeight >= end)
            return 0;

        // Calculate NEO holder reward
        int64_t neoHolderReward = CalculateNeoHolderReward(token, snapshot, state.balance, state.balanceHeight, end);

        // Calculate voter reward
        int64_t voterReward = 0;
        if (!state.voteTo.IsInfinity())
        {
            // Get the current gas per vote
            persistence::StorageKey key = token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::VoterReward), state.voteTo.ToArray());
            auto item = snapshot->TryGet(key);
            if (item)
            {
                std::istringstream stream(std::string(reinterpret_cast<const char*>(item->GetValue().Data()), item->GetValue().Size()));
                io::BinaryReader reader(stream);
                int64_t gasPerVote = reader.ReadInt64();

                // Calculate voter reward
                voterReward = (gasPerVote - state.lastGasPerVote) * state.balance;
            }
        }

        return neoHolderReward + voterReward;
    }

    int64_t NeoTokenGas::CalculateNeoHolderReward(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot, int64_t value, uint32_t start, uint32_t end)
    {
        if (start >= end)
            return 0;

        // Get the gas per block
        int64_t gasPerBlock = GetGasPerBlock(token, snapshot);

        // Calculate the reward
        int64_t amount = 0;
        uint32_t ustart = start;
        uint32_t uend = end;

        // Calculate the reward for each block
        for (uint32_t i = ustart; i < uend; i++)
        {
            amount += gasPerBlock * (100 - NeoToken::COMMITTEE_REWARD_RATIO) / 100;
        }

        // Calculate the reward for the account
        return amount * value / NeoToken::TOTAL_AMOUNT;
    }

    std::shared_ptr<vm::StackItem> NeoTokenGas::OnGetGasPerBlock(const NeoToken& token, ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        return vm::StackItem::Create(GetGasPerBlock(token, engine.GetSnapshot()));
    }

    std::shared_ptr<vm::StackItem> NeoTokenGas::OnSetGasPerBlock(const NeoToken& token, ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.size() < 1)
            throw std::runtime_error("Invalid number of arguments");

        auto gasPerBlockItem = args[0];
        int64_t gasPerBlock = gasPerBlockItem->GetInteger();

        // Complete committee authorization implementation
        try
        {
            // Get current committee members from NEO token contract
            auto committee = GetCommitteeFromNeoContract(engine.GetSnapshot());
            if (committee.empty()) {
                throw std::runtime_error("No committee members found");
            }
            
            // Calculate committee multi-signature address
            auto committee_address = CalculateCommitteeAddress(committee);
            
            // Get the calling script hash
            auto calling_script_hash = engine.GetCallingScriptHash();
            
            // Verify committee authorization through multiple methods
            bool authorized = false;
            
            // Method 1: Check if called by committee multi-sig contract
            if (calling_script_hash.has_value() && calling_script_hash.value() == committee_address) {
                authorized = true;
            }
            
            // Method 2: Check witness verification for committee
            if (!authorized && engine.CheckWitness(committee_address)) {
                authorized = true;
            }
            
            // Method 3: Check if any committee member has authorized this
            if (!authorized) {
                for (const auto& member : committee) {
                    auto member_script_hash = GetScriptHashFromPublicKey(member);
                    if (engine.CheckWitness(member_script_hash)) {
                        authorized = true;
                        break;
                    }
                }
            }
            
            if (!authorized) {
                throw std::runtime_error("Committee authorization required for NEO gas operations");
            }
            
            // Committee authorization successful
            std::cout << "Committee authorization verified for NEO gas operations" << std::endl;
        }
        catch (const std::exception& e)
        {
            // Committee authorization failed - MUST deny access for security
            throw std::runtime_error(std::string("Committee authorization failed for NEO gas operations: ") + e.what());
        }

        try
        {
            SetGasPerBlock(token, engine.GetSnapshot(), gasPerBlock);
            return vm::StackItem::Create(true);
        }
        catch (const std::exception&)
        {
            return vm::StackItem::Create(false);
        }
    }

    std::shared_ptr<vm::StackItem> NeoTokenGas::OnGetUnclaimedGas(const NeoToken& token, ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.size() < 2)
            throw std::runtime_error("Invalid number of arguments");

        auto accountItem = args[0];
        auto endItem = args[1];

        io::UInt160 account;
        auto accountBytes = accountItem->GetByteArray();
        if (accountBytes.Size() != 20)
            throw std::runtime_error("Invalid account");

        std::memcpy(account.Data(), accountBytes.Data(), 20);

        uint32_t end = static_cast<uint32_t>(endItem->GetInteger());

        auto gas = GetUnclaimedGas(token, engine.GetSnapshot(), account, end);

        return vm::StackItem::Create(gas);
    }
} 