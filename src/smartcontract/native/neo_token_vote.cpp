#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/neo_token_account.h>
#include <neo/smartcontract/native/neo_token_candidate.h>
#include <neo/smartcontract/native/neo_token_vote.h>
#include <neo/vm/stack_item.h>
#include <sstream>
#include <stdexcept>

namespace neo::smartcontract::native
{
bool NeoTokenVote::Vote(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot,
                        const io::UInt160& account, const std::vector<cryptography::ecc::ECPoint>& pubKeys)
{
    // Get the account state
    auto state = NeoTokenAccount::GetAccountState(token, snapshot, account);

    // Check if the account has NEO
    if (state.balance <= 0)
        return false;

    // Check if the account is already voting
    if (!state.voteTo.IsInfinity())
    {
        // Remove votes from the old candidate
        auto oldCandidateState = NeoTokenCandidate::GetCandidateState(token, snapshot, state.voteTo);
        if (oldCandidateState.registered)
        {
            oldCandidateState.votes -= state.balance;

            // Save the old candidate state
            std::ostringstream stream;
            io::BinaryWriter writer(stream);
            oldCandidateState.Serialize(writer);
            std::string data = stream.str();

            persistence::StorageKey key = token.CreateStorageKey(
                static_cast<uint8_t>(NeoToken::StoragePrefix::Candidate), state.voteTo.ToArray());
            persistence::StorageItem item(
                io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size())));
            snapshot->Add(key, item);
        }
    }

    // Update the account state
    if (pubKeys.empty())
    {
        // Clear vote
        state.voteTo = cryptography::ecc::ECPoint();
        state.lastGasPerVote = 0;
    }
    else
    {
        // Check if the candidate is registered
        auto candidateState = NeoTokenCandidate::GetCandidateState(token, snapshot, pubKeys[0]);
        if (!candidateState.registered)
            return false;

        // Add votes to the new candidate
        candidateState.votes += state.balance;

        // Save the new candidate state
        std::ostringstream stream;
        io::BinaryWriter writer(stream);
        candidateState.Serialize(writer);
        std::string data = stream.str();

        persistence::StorageKey key =
            token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::Candidate), pubKeys[0].ToArray());
        persistence::StorageItem item(
            io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size())));
        snapshot->Add(key, item);

        // Update the account state
        state.voteTo = pubKeys[0];

        // Get the current gas per vote
        persistence::StorageKey gasKey =
            token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::VoterReward), pubKeys[0].ToArray());
        auto gasItem = snapshot->TryGet(gasKey);
        if (gasItem)
        {
            std::istringstream gasStream(
                std::string(reinterpret_cast<const char*>(gasItem->GetValue().Data()), gasItem->GetValue().Size()));
            io::BinaryReader gasReader(gasStream);
            state.lastGasPerVote = gasReader.ReadInt64();
        }
        else
        {
            state.lastGasPerVote = 0;
        }
    }

    // Save the account state
    std::ostringstream stream;
    io::BinaryWriter writer(stream);
    state.Serialize(writer);
    std::string data = stream.str();

    persistence::StorageKey key =
        token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::Account),
                               io::ByteVector(io::ByteSpan(account.Data(), io::UInt160::Size)));
    persistence::StorageItem item(
        io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size())));
    snapshot->Add(key, item);

    return true;
}

std::shared_ptr<vm::StackItem> NeoTokenVote::OnVote(const NeoToken& token, ApplicationEngine& engine,
                                                    const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.size() < 2)
        throw std::runtime_error("Invalid number of arguments");

    auto accountItem = args[0];
    auto pubKeysItem = args[1];

    io::UInt160 account;
    auto accountBytes = accountItem->GetByteArray();
    if (accountBytes.Size() != 20)
        throw std::runtime_error("Invalid account");

    std::memcpy(account.Data(), accountBytes.Data(), 20);

    // Check witness
    if (!engine.CheckWitness(account))
        throw std::runtime_error("No authorization");

    std::vector<cryptography::ecc::ECPoint> pubKeys;
    auto pubKeysArray = pubKeysItem->GetArray();
    for (const auto& pubKeyItem : pubKeysArray)
    {
        auto pubKeyBytes = pubKeyItem->GetByteArray();
        cryptography::ecc::ECPoint pubKey;
        try
        {
            pubKey = cryptography::ecc::ECPoint(pubKeyBytes.ToHexString());
        }
        catch (const std::exception&)
        {
            throw std::runtime_error("Invalid public key");
        }

        pubKeys.push_back(pubKey);
    }

    bool result = Vote(token, engine.GetSnapshot(), account, pubKeys);
    return vm::StackItem::Create(result);
}

std::shared_ptr<vm::StackItem> NeoTokenVote::OnUnVote(const NeoToken& token, ApplicationEngine& engine,
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

    // Check witness
    if (!engine.CheckWitness(account))
        throw std::runtime_error("No authorization");

    // UnVote is equivalent to voting with empty public keys
    std::vector<cryptography::ecc::ECPoint> emptyPubKeys;
    bool result = Vote(token, engine.GetSnapshot(), account, emptyPubKeys);
    return vm::StackItem::Create(result);
}
}  // namespace neo::smartcontract::native
