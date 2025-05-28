#include <neo/smartcontract/native/neo_token_persistence.h>
#include <neo/smartcontract/native/neo_token_committee.h>
#include <neo/smartcontract/native/neo_token_gas.h>
#include <neo/smartcontract/native/neo_token_transfer.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/vm/stack_item.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <sstream>
#include <stdexcept>

namespace neo::smartcontract::native
{
    bool NeoTokenPersistence::InitializeContract(const NeoToken& token, ApplicationEngine& engine, uint32_t hardfork)
    {
        if (hardfork == 0)
        {
            // Create a cached committee from standby validators
            auto settings = engine.GetProtocolSettings();
            if (!settings)
                return false;

            auto standbyCommittee = settings->GetStandbyCommittee();
            if (standbyCommittee.empty())
                return false;

            // Store committee
            std::vector<NeoToken::CommitteeMember> committee;
            for (const auto& pubKey : standbyCommittee)
            {
                NeoToken::CommitteeMember member;
                member.publicKey = pubKey;
                member.votes = 0;
                committee.push_back(member);
            }

            std::ostringstream stream;
            io::BinaryWriter writer(stream);
            writer.WriteVarInt(committee.size());
            for (const auto& member : committee)
            {
                writer.Write(member);
            }
            std::string data = stream.str();

            persistence::StorageKey committeeKey = token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::Committee));
            persistence::StorageItem committeeItem(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size())));
            engine.GetSnapshot()->Add(committeeKey, committeeItem);

            // Initialize voters count
            persistence::StorageKey votersCountKey = token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::VotersCount));
            persistence::StorageItem votersCountItem(io::ByteVector{});
            engine.GetSnapshot()->Add(votersCountKey, votersCountItem);

            // Initialize gas per block
            int64_t gasPerBlock = 5 * 100000000; // 5 GAS
            std::ostringstream gasStream;
            io::BinaryWriter gasWriter(gasStream);
            gasWriter.Write(gasPerBlock);
            std::string gasData = gasStream.str();

            persistence::StorageKey gasKey = token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::GasPerBlock));
            persistence::StorageItem gasItem(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(gasData.data()), gasData.size())));
            engine.GetSnapshot()->Add(gasKey, gasItem);

            // Initialize register price
            int64_t registerPrice = 1000 * 100000000; // 1000 GAS
            std::ostringstream priceStream;
            io::BinaryWriter priceWriter(priceStream);
            priceWriter.Write(registerPrice);
            std::string priceData = priceStream.str();

            persistence::StorageKey priceKey = token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::RegisterPrice));
            persistence::StorageItem priceItem(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(priceData.data()), priceData.size())));
            engine.GetSnapshot()->Add(priceKey, priceItem);

            // Mint initial NEO to BFT address
            io::UInt160 bftAddress = NeoTokenCommittee::GetCommitteeAddress(token, engine.GetSnapshot());
            io::Fixed8 totalAmount(NeoToken::TOTAL_AMOUNT);
            NeoTokenTransfer::Transfer(token, engine.GetSnapshot(), io::UInt160(), bftAddress, totalAmount);

            // Notify transfer event
            std::vector<std::shared_ptr<vm::StackItem>> state = {
                vm::StackItem::Null(),
                vm::StackItem::Create(bftAddress),
                vm::StackItem::Create(totalAmount.GetValue())
            };

            engine.Notify(token.GetScriptHash(), "Transfer", state);
        }

        return true;
    }

    bool NeoTokenPersistence::OnPersist(const NeoToken& token, ApplicationEngine& engine)
    {
        // Get the persisting block
        auto block = engine.GetPersistingBlock();
        if (!block)
            return false;

        // Initialize contract if needed
        auto key = token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::Committee));
        auto item = engine.GetSnapshot()->TryGet(key);
        if (!item)
        {
            InitializeContract(token, engine, 0);
        }

        // Check if committee should be refreshed
        if (NeoTokenCommittee::ShouldRefreshCommittee(token, block->GetIndex(), 7)) // Default to 7 committee members
        {
            // Compute new committee
            auto committee = NeoTokenCommittee::ComputeCommitteeMembers(token, engine.GetSnapshot(), 7);

            // Store committee
            std::ostringstream stream;
            io::BinaryWriter writer(stream);
            writer.WriteVarInt(committee.size());
            for (const auto& member : committee)
            {
                writer.Write(member);
            }
            std::string data = stream.str();

            persistence::StorageKey key = token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::Committee));
            persistence::StorageItem item(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size())));
            engine.GetSnapshot()->Add(key, item);
        }

        return true;
    }

    bool NeoTokenPersistence::PostPersist(const NeoToken& token, ApplicationEngine& engine)
    {
        // Get the persisting block
        auto block = engine.GetPersistingBlock();
        if (!block)
            return false;

        // Distribute GAS for committee
        int committeeSize = 7; // Default to 7 committee members
        int validatorsCount = 7; // Default to 7 validators
        int index = block->GetIndex() % committeeSize;
        int64_t gasPerBlock = NeoTokenGas::GetGasPerBlock(token, engine.GetSnapshot());
        auto committee = NeoTokenCommittee::GetCommitteeFromCache(token, engine.GetSnapshot());

        if (committee.empty() || index >= static_cast<int>(committee.size()))
            return false;

        auto pubKey = committee[index].publicKey;
        auto account = cryptography::Hash::Hash160(pubKey.ToArray().AsSpan());

        // Mint GAS to committee member
        auto gasToken = GasToken::GetInstance();
        gasToken->Mint(engine.GetSnapshot(), account, gasPerBlock * NeoToken::COMMITTEE_REWARD_RATIO / 100);

        return true;
    }
}
