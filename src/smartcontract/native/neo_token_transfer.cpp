#include <neo/smartcontract/native/neo_token_transfer.h>
#include <neo/smartcontract/native/neo_token_account.h>
#include <neo/smartcontract/native/neo_token_candidate.h>
#include <neo/smartcontract/native/neo_token_gas.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/vm/stack_item.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <sstream>
#include <stdexcept>

namespace neo::smartcontract::native
{
    io::Fixed8 NeoTokenTransfer::GetTotalSupply(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot)
    {
        persistence::StorageKey key = token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::TotalSupply));
        auto item = snapshot->TryGet(key);
        if (!item)
            return io::Fixed8(static_cast<int64_t>(0));

        std::istringstream stream(std::string(reinterpret_cast<const char*>(item->GetValue().Data()), item->GetValue().Size()));
        io::BinaryReader reader(stream);
        return io::Fixed8(reader.ReadInt64());
    }

    bool NeoTokenTransfer::Transfer(const NeoToken& token, ApplicationEngine& engine, std::shared_ptr<persistence::DataCache> snapshot, const io::UInt160& from, const io::UInt160& to, const io::Fixed8& amount)
    {
        // Check if the amount is valid
        if (amount.Value() <= 0)
            return false;

        // Check if the from account is the null account (for minting)
        if (from != io::UInt160())
        {
            // Get the from account state
            auto fromState = NeoTokenAccount::GetAccountState(token, snapshot, from);

            // Check if the from account has enough balance
            if (fromState.balance < amount.Value())
                return false;

            // Update the from account state
            fromState.balance -= amount.Value();

            // Save the from account state
            std::ostringstream fromStream;
            io::BinaryWriter fromWriter(fromStream);
            fromState.Serialize(fromWriter);
            std::string fromData = fromStream.str();

            persistence::StorageKey fromKey = token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::Account), io::ByteVector(io::ByteSpan(from.Data(), io::UInt160::Size)));
            persistence::StorageItem fromItem(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(fromData.data()), fromData.size())));
            snapshot->Add(fromKey, fromItem);

            // Update the candidate votes if the account is voting
            if (!fromState.voteTo.IsInfinity())
            {
                auto candidateState = NeoTokenCandidate::GetCandidateState(token, snapshot, fromState.voteTo);
                if (candidateState.registered)
                {
                    candidateState.votes -= amount.Value();

                    // Save the candidate state
                    std::ostringstream candidateStream;
                    io::BinaryWriter candidateWriter(candidateStream);
                    candidateState.Serialize(candidateWriter);
                    std::string candidateData = candidateStream.str();

                    persistence::StorageKey candidateKey = token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::Candidate), fromState.voteTo.ToArray());
                    persistence::StorageItem candidateItem(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(candidateData.data()), candidateData.size())));
                    snapshot->Add(candidateKey, candidateItem);
                }
            }
        }

        // Check if the to account is the null account (for burning)
        if (to != io::UInt160())
        {
            // Get the to account state
            auto toState = NeoTokenAccount::GetAccountState(token, snapshot, to);

            // Update the to account state
            toState.balance += amount.Value();

            // If this is the first time the account is receiving NEO, set the balance height
            if (toState.balance == 0)
            {
                // Get the current block height from ApplicationEngine
                uint32_t currentHeight = engine.GetCurrentBlockHeight();
                toState.balanceHeight = currentHeight;
            }

            // Save the to account state
            std::ostringstream toStream;
            io::BinaryWriter toWriter(toStream);
            toState.Serialize(toWriter);
            std::string toData = toStream.str();

            persistence::StorageKey toKey = token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::Account), io::ByteVector(io::ByteSpan(to.Data(), io::UInt160::Size)));
            persistence::StorageItem toItem(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(toData.data()), toData.size())));
            snapshot->Add(toKey, toItem);

            // Update the candidate votes if the account is voting
            if (!toState.voteTo.IsInfinity())
            {
                auto candidateState = NeoTokenCandidate::GetCandidateState(token, snapshot, toState.voteTo);
                if (candidateState.registered)
                {
                    candidateState.votes += amount.Value();

                    // Save the candidate state
                    std::ostringstream candidateStream;
                    io::BinaryWriter candidateWriter(candidateStream);
                    candidateState.Serialize(candidateWriter);
                    std::string candidateData = candidateStream.str();

                    persistence::StorageKey candidateKey = token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::Candidate), toState.voteTo.ToArray());
                    persistence::StorageItem candidateItem(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(candidateData.data()), candidateData.size())));
                    snapshot->Add(candidateKey, candidateItem);
                }
            }
        }

        return true;
    }

    std::shared_ptr<vm::StackItem> NeoTokenTransfer::OnTotalSupply(const NeoToken& token, ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        auto totalSupply = GetTotalSupply(token, engine.GetSnapshot());
        return vm::StackItem::Create(totalSupply.Value());
    }

    std::shared_ptr<vm::StackItem> NeoTokenTransfer::OnTransfer(const NeoToken& token, ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.size() != 3)
            throw std::runtime_error("The argument count is incorrect");

        auto fromItem = args[0];
        auto toItem = args[1];
        auto amountItem = args[2];

        io::UInt160 from;
        if (!fromItem->IsNull())
        {
            auto fromBytes = fromItem->GetByteArray();
            if (fromBytes.Size() != 20)
                throw std::runtime_error("The argument 'from' is invalid");

            std::memcpy(from.Data(), fromBytes.Data(), 20);
        }

        io::UInt160 to;
        if (!toItem->IsNull())
        {
            auto toBytes = toItem->GetByteArray();
            if (toBytes.Size() != 20)
                throw std::runtime_error("The argument 'to' is invalid");

            std::memcpy(to.Data(), toBytes.Data(), 20);
        }

        io::Fixed8 amount(amountItem->GetInteger());

        // Check if the amount is valid
        if (amount.Value() <= 0)
            throw std::runtime_error("The amount must be a positive number");

        // Check witness
        if (from != io::UInt160() && !engine.CheckWitness(from))
            throw std::runtime_error("No authorization");

        // Distribute GAS before transfer
        if (from != io::UInt160())
        {
            auto fromState = NeoTokenAccount::GetAccountState(token, engine.GetSnapshot(), from);
            NeoTokenGas::DistributeGas(token, engine, from, fromState);
        }

        bool result = Transfer(token, engine, engine.GetSnapshot(), from, to, amount);

        // Notify transfer event
        std::vector<std::shared_ptr<vm::StackItem>> state = {
            from == io::UInt160() ? vm::StackItem::Null() : vm::StackItem::Create(from),
            to == io::UInt160() ? vm::StackItem::Null() : vm::StackItem::Create(to),
            vm::StackItem::Create(amount.Value())
        };

        engine.Notify(token.GetScriptHash(), "Transfer", state);

        return vm::StackItem::Create(result);
    }
}
