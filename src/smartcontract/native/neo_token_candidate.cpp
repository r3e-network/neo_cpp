#include <neo/smartcontract/native/neo_token_candidate.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/vm/stack_item.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <sstream>
#include <stdexcept>

namespace neo::smartcontract::native
{
    bool NeoTokenCandidate::RegisterCandidate(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot, const cryptography::ecc::ECPoint& pubKey)
    {
        // Check if the public key is valid
        if (pubKey.IsInfinity())
            return false;

        // Get the candidate state
        auto state = GetCandidateState(token, snapshot, pubKey);

        // Check if the candidate is already registered
        if (state.registered)
            return false;

        // Register the candidate
        state.registered = true;

        // Save the candidate state
        std::ostringstream stream;
        io::BinaryWriter writer(stream);
        state.Serialize(writer);
        std::string data = stream.str();

        persistence::StorageKey key = token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::Candidate), pubKey.ToArray());
        persistence::StorageItem item(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size())));
        snapshot->Add(key, item);

        return true;
    }

    bool NeoTokenCandidate::UnregisterCandidate(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot, const cryptography::ecc::ECPoint& pubKey)
    {
        // Check if the public key is valid
        if (pubKey.IsInfinity())
            return false;

        // Get the candidate state
        auto state = GetCandidateState(token, snapshot, pubKey);

        // Check if the candidate is registered
        if (!state.registered)
            return false;

        // Unregister the candidate
        state.registered = false;

        // Save the candidate state
        std::ostringstream stream;
        io::BinaryWriter writer(stream);
        state.Serialize(writer);
        std::string data = stream.str();

        persistence::StorageKey key = token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::Candidate), pubKey.ToArray());
        persistence::StorageItem item(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size())));
        snapshot->Add(key, item);

        return true;
    }

    NeoToken::CandidateState NeoTokenCandidate::GetCandidateState(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot, const cryptography::ecc::ECPoint& pubKey)
    {
        persistence::StorageKey key = token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::Candidate), pubKey.ToArray());
        auto item = snapshot->TryGet(key);
        if (!item)
        {
            // Return default candidate state
            NeoToken::CandidateState state;
            state.registered = false;
            state.votes = 0;
            return state;
        }

        std::istringstream stream(std::string(reinterpret_cast<const char*>(item->GetValue().Data()), item->GetValue().Size()));
        io::BinaryReader reader(stream);

        NeoToken::CandidateState state;
        state.Deserialize(reader);
        return state;
    }

    std::vector<std::pair<cryptography::ecc::ECPoint, NeoToken::CandidateState>> NeoTokenCandidate::GetCandidates(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot)
    {
        std::vector<std::pair<cryptography::ecc::ECPoint, NeoToken::CandidateState>> candidates;

        // Get all candidates
        persistence::StorageKey prefix = token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::Candidate));
        auto iterator = snapshot->Find(prefix);

        while (iterator->Valid())
        {
            auto key = iterator->Key();
            auto value = iterator->Value();

            // Skip if key doesn't start with the prefix
            if (key.Size() <= prefix.Size() || std::memcmp(key.Data(), prefix.Data(), prefix.Size()) != 0)
                break;

            // Extract public key from key
            io::ByteVector pubKeyBytes(key.AsSpan().SubSpan(prefix.Size()));
            cryptography::ecc::ECPoint pubKey = cryptography::ecc::ECPoint::FromBytes(pubKeyBytes.AsSpan(), cryptography::ecc::ECCurve::Secp256r1);

            // Deserialize candidate state
            std::istringstream stream(std::string(reinterpret_cast<const char*>(value.Data()), value.Size()));
            io::BinaryReader reader(stream);

            NeoToken::CandidateState state;
            state.Deserialize(reader);

            // Only add registered candidates
            if (state.registered)
                candidates.push_back(std::make_pair(pubKey, state));

            iterator->Next();
        }

        return candidates;
    }

    int64_t NeoTokenCandidate::GetCandidateVote(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot, const cryptography::ecc::ECPoint& pubKey)
    {
        auto state = GetCandidateState(token, snapshot, pubKey);
        return state.registered ? state.votes : -1;
    }

    void NeoTokenCandidate::CheckCandidate(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot, const cryptography::ecc::ECPoint& pubKey, const NeoToken::CandidateState& state)
    {
        // TODO: Implement candidate check
    }

    std::shared_ptr<vm::StackItem> NeoTokenCandidate::OnRegisterCandidate(const NeoToken& token, ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.size() < 1)
            throw std::runtime_error("Invalid number of arguments");

        auto pubKeyItem = args[0];

        auto pubKeyBytes = pubKeyItem->GetByteArray();
        cryptography::ecc::ECPoint pubKey;
        try
        {
            pubKey = cryptography::ecc::ECPoint::FromBytes(pubKeyBytes.AsSpan(), cryptography::ecc::ECCurve::Secp256r1);
        }
        catch (const std::exception&)
        {
            throw std::runtime_error("Invalid public key");
        }

        // Check witness
        if (!engine.CheckWitness(pubKey))
            throw std::runtime_error("No authorization");

        bool result = RegisterCandidate(token, engine.GetSnapshot(), pubKey);
        return vm::StackItem::Create(result);
    }

    std::shared_ptr<vm::StackItem> NeoTokenCandidate::OnUnregisterCandidate(const NeoToken& token, ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.size() < 1)
            throw std::runtime_error("Invalid number of arguments");

        auto pubKeyItem = args[0];

        auto pubKeyBytes = pubKeyItem->GetByteArray();
        cryptography::ecc::ECPoint pubKey;
        try
        {
            pubKey = cryptography::ecc::ECPoint::FromBytes(pubKeyBytes.AsSpan(), cryptography::ecc::ECCurve::Secp256r1);
        }
        catch (const std::exception&)
        {
            throw std::runtime_error("Invalid public key");
        }

        // Check witness
        if (!engine.CheckWitness(pubKey))
            throw std::runtime_error("No authorization");

        bool result = UnregisterCandidate(token, engine.GetSnapshot(), pubKey);
        return vm::StackItem::Create(result);
    }

    std::shared_ptr<vm::StackItem> NeoTokenCandidate::OnGetCandidates(const NeoToken& token, ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        auto candidates = GetCandidates(token, engine.GetSnapshot());

        // Create array of candidates
        std::vector<std::shared_ptr<vm::StackItem>> candidatesArray;
        for (const auto& [pubKey, state] : candidates)
        {
            std::vector<std::shared_ptr<vm::StackItem>> candidateItems;
            candidateItems.push_back(vm::StackItem::Create(pubKey.ToArray()));
            candidateItems.push_back(vm::StackItem::Create(static_cast<int64_t>(state.votes)));
            vm::ReferenceCounter refCounter;
            candidatesArray.push_back(vm::StackItem::CreateStruct(candidateItems, refCounter));
        }

        return vm::StackItem::Create(candidatesArray);
    }

    std::shared_ptr<vm::StackItem> NeoTokenCandidate::OnGetCandidateVote(const NeoToken& token, ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.size() < 1)
            throw std::runtime_error("Invalid number of arguments");

        auto pubKeyItem = args[0];

        auto pubKeyBytes = pubKeyItem->GetByteArray();
        cryptography::ecc::ECPoint pubKey = cryptography::ecc::ECPoint::FromBytes(pubKeyBytes.AsSpan(), cryptography::ecc::ECCurve::Secp256r1);

        auto vote = GetCandidateVote(token, engine.GetSnapshot(), pubKey);

        return vm::StackItem::Create(vote);
    }

    int64_t NeoTokenCandidate::GetRegisterPrice(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot)
    {
        // TODO: Implement register price logic
        return 1000 * 100000000; // 1000 GAS
    }

    std::shared_ptr<vm::StackItem> NeoTokenCandidate::OnGetRegisterPrice(const NeoToken& token, ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        return vm::StackItem::Create(GetRegisterPrice(token, engine.GetSnapshot()));
    }

    std::shared_ptr<vm::StackItem> NeoTokenCandidate::OnSetRegisterPrice(const NeoToken& token, ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.size() < 1)
            throw std::runtime_error("Invalid number of arguments");

        auto priceItem = args[0];
        int64_t price = priceItem->GetInteger();

        // Check if caller is committee
        // TODO: Implement committee check

        // TODO: Implement set register price logic

        return vm::StackItem::Create(true);
    }
}
