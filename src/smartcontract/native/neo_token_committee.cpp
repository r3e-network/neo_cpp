#include <neo/smartcontract/native/neo_token_committee.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/script_builder.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/cryptography/hash.h>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iostream>

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
        // Implement validator selection based on voting
        try
        {
            // Get all committee members first
            auto committee = GetCommittee(token, snapshot);
            
            // Get the number of validators from protocol settings
            int validatorsCount = 7; // Default NEO validator count
            
            // If we have fewer committee members than needed validators, return all committee
            if (committee.size() <= static_cast<size_t>(validatorsCount))
                return committee;
            
            // Sort committee members by vote count (descending order)
            // This requires getting vote counts for each committee member
            std::vector<std::pair<cryptography::ecc::ECPoint, int64_t>> committeWithVotes;
            
            for (const auto& member : committee)
            {
                // Get vote count for this committee member
                auto candidateState = NeoTokenCandidate::GetCandidateState(token, snapshot, member);
                committeWithVotes.push_back(std::make_pair(member, candidateState.votes));
            }
            
            // Sort by vote count (highest first)
            std::sort(committeWithVotes.begin(), committeWithVotes.end(),
                [](const auto& a, const auto& b) {
                    return a.second > b.second;
                });
            
            // Take the top validators
            std::vector<cryptography::ecc::ECPoint> validators;
            validators.reserve(validatorsCount);
            
            for (int i = 0; i < validatorsCount && i < static_cast<int>(committeWithVotes.size()); i++)
            {
                validators.push_back(committeWithVotes[i].first);
            }
            
            return validators;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error selecting validators: " << e.what() << std::endl;
            // Fallback to committee if validator selection fails
            return GetCommittee(token, snapshot);
        }
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
        // Implement committee computation based on voting
        try
        {
            // Get all registered candidates with their vote counts
            auto candidates = NeoTokenCandidate::GetCandidates(token, snapshot);
            
            // Get the number of committee members from protocol settings
            int committeeMembersCount = 21; // Default NEO committee size
            
            // If we have fewer candidates than needed committee members, return all candidates
            if (candidates.size() <= static_cast<size_t>(committeeMembersCount))
            {
                std::vector<cryptography::ecc::ECPoint> committee;
                committee.reserve(candidates.size());
                
                for (const auto& [pubkey, state] : candidates)
                {
                    committee.push_back(pubkey);
                }
                
                return committee;
            }
            
            // Sort candidates by vote count (descending order)
            std::sort(candidates.begin(), candidates.end(),
                [](const auto& a, const auto& b) {
                    return a.second.votes > b.second.votes;
                });
            
            // Take the top committee members
            std::vector<cryptography::ecc::ECPoint> committee;
            committee.reserve(committeeMembersCount);
            
            for (int i = 0; i < committeeMembersCount && i < static_cast<int>(candidates.size()); i++)
            {
                committee.push_back(candidates[i].first);
            }
            
            return committee;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error computing committee: " << e.what() << std::endl;
            // Return empty committee on error
            return std::vector<cryptography::ecc::ECPoint>();
        }
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

        // Implement proper multi-signature script creation
        try
        {
            // Create multi-signature script for committee members
            // This follows the C# Contract.CreateMultiSigRedeemScript pattern
            
            // Calculate m (required signatures) - typically majority
            int m = (static_cast<int>(committee.size()) / 2) + 1;
            int n = static_cast<int>(committee.size());
            
            // Build multi-sig script: m PUSH(pubkey1) PUSH(pubkey2) ... PUSH(pubkeyn) n CHECKMULTISIG
            vm::ScriptBuilder builder;
            
            // Push m (required signatures count)
            builder.EmitPush(m);
            
            // Push all public keys in order
            for (const auto& pubkey : committee)
            {
                auto pubkeyBytes = pubkey.ToBytes();
                builder.EmitPush(pubkeyBytes.AsSpan());
            }
            
            // Push n (total public keys count)
            builder.EmitPush(n);
            
            // Emit CHECKMULTISIG opcode
            builder.EmitSysCall("System.Crypto.CheckMultisig");
            
            // Calculate script hash
            auto script = builder.ToArray();
            return cryptography::Hash::Hash160(script.AsSpan());
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error creating committee multi-sig script: " << e.what() << std::endl;
            return io::UInt160(); // Return zero hash on error
        }
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
