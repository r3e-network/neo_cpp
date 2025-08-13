/**
 * @file neo_token_persistence.cpp
 * @brief NEO governance token contract
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/cryptography/hash.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/neo_token_committee.h>
#include <neo/smartcontract/native/neo_token_gas.h>
#include <neo/smartcontract/native/neo_token_persistence.h>
#include <neo/smartcontract/native/neo_token_transfer.h>
#include <neo/vm/stack_item.h>

#include <sstream>
#include <stdexcept>
#include <vector>

namespace neo::smartcontract::native
{
bool NeoTokenPersistence::InitializeContract(const NeoToken& token, ApplicationEngine& engine, uint32_t hardfork)
{
    if (hardfork == 0)
    {
        // Create a cached committee from standby validators
        auto settings = engine.GetProtocolSettings();
        if (!settings) return false;

        auto standbyCommittee = settings->GetStandbyCommittee();
        // Use manual size check instead of empty() method
        bool hasCommittee = false;
        try
        {
            auto firstMember = standbyCommittee[0];  // Try to access first element
            hasCommittee = true;
        }
        catch (...)
        {
            return false;
        }

        // Store committee - use manual iteration
        std::vector<NeoToken::CommitteeMember> committee;
        size_t committeeCount = 0;

        // Try to iterate through standby committee manually
        for (size_t i = 0; i < 21; i++)  // Max 21 committee members
        {
            try
            {
                auto pubKey = standbyCommittee[i];
                NeoToken::CommitteeMember member;
                member.publicKey = pubKey;
                member.votes = 0;
                committee.push_back(member);
                committeeCount++;
            }
            catch (...)
            {
                break;  // End of committee
            }
        }

        if (committeeCount == 0) return false;

        std::ostringstream stream;
        io::BinaryWriter writer(stream);
        writer.WriteVarInt(committeeCount);
        for (size_t i = 0; i < committeeCount; i++)
        {
            committee[i].Serialize(writer);
        }
        std::string data = stream.str();

        persistence::StorageKey committeeKey =
            token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::Committee));
        persistence::StorageItem committeeItem(
            io::ByteVector(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
        engine.GetSnapshot()->Add(committeeKey, committeeItem);

        // Initialize voters count
        persistence::StorageKey votersCountKey =
            token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::VotersCount));
        persistence::StorageItem votersCountItem(io::ByteVector{});
        engine.GetSnapshot()->Add(votersCountKey, votersCountItem);

        // Initialize gas per block
        int64_t gasPerBlock = 5 * 100000000;  // 5 GAS
        std::ostringstream gasStream;
        io::BinaryWriter gasWriter(gasStream);
        gasWriter.Write(gasPerBlock);
        std::string gasData = gasStream.str();

        persistence::StorageKey gasKey =
            token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::GasPerBlock));
        persistence::StorageItem gasItem(
            io::ByteVector(reinterpret_cast<const uint8_t*>(gasData.data()), gasData.size()));
        engine.GetSnapshot()->Add(gasKey, gasItem);

        // Initialize register price
        int64_t registerPrice = 1000LL * 100000000LL;  // 1000 GAS
        std::ostringstream priceStream;
        io::BinaryWriter priceWriter(priceStream);
        priceWriter.Write(registerPrice);
        std::string priceData = priceStream.str();

        persistence::StorageKey priceKey =
            token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::RegisterPrice));
        persistence::StorageItem priceItem(
            io::ByteVector(reinterpret_cast<const uint8_t*>(priceData.data()), priceData.size()));
        engine.GetSnapshot()->Add(priceKey, priceItem);

        // Mint initial NEO to BFT address
        io::UInt160 bftAddress = NeoTokenCommittee::GetCommitteeAddress(token, engine.GetSnapshot());
        io::Fixed8 totalAmount(NeoToken::TOTAL_AMOUNT);
        bool result =
            NeoTokenTransfer::Transfer(token, engine, engine.GetSnapshot(), io::UInt160(), bftAddress, totalAmount);

        // Notify transfer event - use manual vector construction
        std::vector<std::shared_ptr<vm::StackItem>> state;
        state.push_back(vm::StackItem::Null());
        state.push_back(vm::StackItem::Create(bftAddress));
        state.push_back(vm::StackItem::Create(totalAmount.Value()));

        engine.Notify(token.GetScriptHash(), "Transfer", state);
    }

    return true;
}

bool NeoTokenPersistence::OnPersist(const NeoToken& token, ApplicationEngine& engine)
{
    // Get the persisting block
    auto block = engine.GetPersistingBlock();
    if (!block) return false;

    // Initialize contract if needed
    auto key = token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::Committee));
    auto item = engine.GetSnapshot()->TryGet(key);
    if (!item)
    {
        InitializeContract(token, engine, 0);
    }

    // Check if committee should be refreshed
    if (NeoTokenCommittee::ShouldRefreshCommittee(token, block->GetIndex(), 7))  // Default to 7 committee members
    {
        // Compute new committee
        auto committee = NeoTokenCommittee::ComputeCommitteeMembers(token, engine.GetSnapshot(), 7);

        // Store committee - use manual iteration
        std::ostringstream stream;
        io::BinaryWriter writer(stream);

        // Count committee members manually
        size_t committeeCount = 0;
        for (size_t i = 0; i < 21; i++)  // Max 21 committee members
        {
            try
            {
                auto member = committee[i];
                committeeCount++;
            }
            catch (...)
            {
                break;
            }
        }

        writer.WriteVarInt(committeeCount);
        for (size_t i = 0; i < committeeCount; i++)
        {
            committee[i].Serialize(writer);
        }
        std::string data = stream.str();

        persistence::StorageKey key = token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::Committee));
        persistence::StorageItem item(io::ByteVector(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
        engine.GetSnapshot()->Add(key, item);
    }

    return true;
}

bool NeoTokenPersistence::PostPersist(const NeoToken& token, ApplicationEngine& engine)
{
    // Get the persisting block
    auto block = engine.GetPersistingBlock();
    if (!block) return false;

    // Distribute GAS for committee
    int committeeSize = 7;    // Default to 7 committee members
    int validatorsCount = 7;  // Default to 7 validators
    int index = block->GetIndex() % committeeSize;
    int64_t gasPerBlock = NeoTokenGas::GetGasPerBlock(token, engine.GetSnapshot());
    auto committee = NeoTokenCommittee::GetCommitteeFromCache(token, engine.GetSnapshot());

    // Check committee manually
    bool hasCommittee = false;
    try
    {
        auto member = committee[index];
        hasCommittee = true;
    }
    catch (...)
    {
        return false;
    }

    auto pubKey = committee[index].publicKey;
    auto account = neo::cryptography::Hash::Hash160(pubKey.ToArray().AsSpan());

    // Mint GAS to committee member
    auto gasToken = GasToken::GetInstance();
    gasToken->Mint(engine.GetSnapshot(), account, gasPerBlock * NeoToken::COMMITTEE_REWARD_RATIO / 100);

    return true;
}
}  // namespace neo::smartcontract::native
