#include <neo/smartcontract/native/neo_token_committee.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/vm/stack_item.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <sstream>
#include <stdexcept>

namespace neo::smartcontract::native
{
    std::vector<cryptography::ecc::ECPoint> NeoTokenCommittee::GetCommittee(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot)
    {
        auto committee = GetCommitteeFromCache(token, snapshot);
        std::vector<cryptography::ecc::ECPoint> result;
        result.reserve(committee.size());

        for (const auto& member : committee)
        {
            result.push_back(member.publicKey);
        }

        return result;
    }

    std::vector<cryptography::ecc::ECPoint> NeoTokenCommittee::GetValidators(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot)
    {
        // TODO: Implement validator selection based on voting
        return GetCommittee(token, snapshot);
    }

    std::vector<cryptography::ecc::ECPoint> NeoTokenCommittee::GetNextBlockValidators(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot, int32_t validatorsCount)
    {
        auto validators = GetValidators(token, snapshot);
        if (validators.size() > static_cast<size_t>(validatorsCount))
        {
            validators.resize(validatorsCount);
        }
        return validators;
    }

    std::vector<cryptography::ecc::ECPoint> NeoTokenCommittee::ComputeCommitteeMembers(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot, int32_t committeeSize)
    {
        // TODO: Implement committee computation based on voting
        return GetCommittee(token, snapshot);
    }

    bool NeoTokenCommittee::ShouldRefreshCommittee(const NeoToken& token, uint32_t blockIndex, int32_t committeeSize)
    {
        // Refresh committee every 21 blocks (for testing purposes)
        // In production, this would be a larger value
        return blockIndex % 21 == 0;
    }

    std::vector<NeoToken::CommitteeMember> NeoTokenCommittee::GetCommitteeFromCache(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot)
    {
        persistence::StorageKey key = token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::Committee));
        auto item = snapshot->TryGet(key);
        if (!item)
            return std::vector<NeoToken::CommitteeMember>();

        std::istringstream stream(std::string(reinterpret_cast<const char*>(item->GetValue().Data()), item->GetValue().Size()));
        io::BinaryReader reader(stream);

        uint64_t count = reader.ReadVarInt();
        std::vector<NeoToken::CommitteeMember> committee;
        committee.reserve(count);

        for (uint64_t i = 0; i < count; i++)
        {
            NeoToken::CommitteeMember member;
            member.Deserialize(reader);
            committee.push_back(member);
        }

        return committee;
    }

    io::UInt160 NeoTokenCommittee::GetCommitteeAddress(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot)
    {
        auto committee = GetCommittee(token, snapshot);
        if (committee.empty())
            return io::UInt160();

        // Create multi-signature script
        int m = (committee.size() + 1) / 2; // Simple majority

        // TODO: Implement proper multi-signature script creation
        // For now, just return a placeholder
        return io::UInt160();
    }

    std::shared_ptr<vm::StackItem> NeoTokenCommittee::OnGetValidators(const NeoToken& token, neo::smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        auto validators = GetValidators(token, engine.GetSnapshot());

        // Create array of validators
        std::vector<std::shared_ptr<vm::StackItem>> validatorsArray;
        for (const auto& validator : validators)
        {
            validatorsArray.push_back(vm::StackItem::Create(validator.ToArray()));
        }

        return vm::StackItem::Create(validatorsArray);
    }

    std::shared_ptr<vm::StackItem> NeoTokenCommittee::OnGetCommittee(const NeoToken& token, neo::smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        auto committee = GetCommittee(token, engine.GetSnapshot());

        // Create array of committee members
        std::vector<std::shared_ptr<vm::StackItem>> committeeArray;
        for (const auto& member : committee)
        {
            committeeArray.push_back(vm::StackItem::Create(member.ToArray()));
        }

        return vm::StackItem::Create(committeeArray);
    }

    std::shared_ptr<vm::StackItem> NeoTokenCommittee::OnGetNextBlockValidators(const NeoToken& token, neo::smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        int32_t validatorsCount = 7; // Default to 7 validators
        if (args.size() > 0)
        {
            validatorsCount = static_cast<int32_t>(args[0]->GetInteger());
            if (validatorsCount <= 0)
                throw std::runtime_error("Invalid validators count");
        }

        auto validators = GetNextBlockValidators(token, engine.GetSnapshot(), validatorsCount);

        // Create array of validators
        std::vector<std::shared_ptr<vm::StackItem>> validatorsArray;
        for (const auto& validator : validators)
        {
            validatorsArray.push_back(vm::StackItem::Create(validator.ToArray()));
        }

        return vm::StackItem::Create(validatorsArray);
    }
}
