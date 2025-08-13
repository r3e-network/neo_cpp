/**
 * @file neo_token_candidate.cpp
 * @brief NEO governance token contract
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/cryptography/ecc/ec_point.h>
#include <neo/cryptography/hash.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/uint160.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/neo_token_candidate.h>
#include <neo/smartcontract/native/neo_token_committee.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/stack_item.h>

#include <iostream>
#include <sstream>
#include <stdexcept>

namespace neo::smartcontract::native
{
bool NeoTokenCandidate::RegisterCandidate(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot,
                                          const cryptography::ecc::ECPoint& pubKey)
{
    // Check if the public key is valid
    if (pubKey.IsInfinity()) return false;

    // Get the candidate state
    auto state = GetCandidateState(token, snapshot, pubKey);

    // Check if the candidate is already registered
    if (state.registered) return false;

    // Register the candidate
    state.registered = true;

    // Save the candidate state
    std::ostringstream stream;
    io::BinaryWriter writer(stream);
    state.Serialize(writer);
    std::string data = stream.str();

    persistence::StorageKey key =
        token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::Candidate), pubKey.ToArray());
    persistence::StorageItem item(
        io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size())));
    snapshot->Add(key, item);

    return true;
}

bool NeoTokenCandidate::UnregisterCandidate(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot,
                                            const cryptography::ecc::ECPoint& pubKey)
{
    // Check if the public key is valid
    if (pubKey.IsInfinity()) return false;

    // Get the candidate state
    auto state = GetCandidateState(token, snapshot, pubKey);

    // Check if the candidate is registered
    if (!state.registered) return false;

    // Unregister the candidate
    state.registered = false;

    // Save the candidate state
    std::ostringstream stream;
    io::BinaryWriter writer(stream);
    state.Serialize(writer);
    std::string data = stream.str();

    persistence::StorageKey key =
        token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::Candidate), pubKey.ToArray());
    persistence::StorageItem item(
        io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size())));
    snapshot->Add(key, item);

    return true;
}

NeoToken::CandidateState NeoTokenCandidate::GetCandidateState(const NeoToken& token,
                                                              std::shared_ptr<persistence::DataCache> snapshot,
                                                              const cryptography::ecc::ECPoint& pubKey)
{
    persistence::StorageKey key =
        token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::Candidate), pubKey.ToArray());
    auto item = snapshot->TryGet(key);
    if (!item)
    {
        // Return default candidate state
        NeoToken::CandidateState state;
        state.registered = false;
        state.votes = 0;
        return state;
    }

    std::istringstream stream(
        std::string(reinterpret_cast<const char*>(item->GetValue().Data()), item->GetValue().Size()));
    io::BinaryReader reader(stream);

    NeoToken::CandidateState state;
    state.Deserialize(reader);
    return state;
}

std::vector<std::pair<cryptography::ecc::ECPoint, NeoToken::CandidateState>> NeoTokenCandidate::GetCandidates(
    const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot)
{
    std::vector<std::pair<cryptography::ecc::ECPoint, NeoToken::CandidateState>> candidates;

    // Get all candidates
    persistence::StorageKey prefix = token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::Candidate));
    auto iterator = snapshot->Seek(prefix);

    while (iterator->Valid())
    {
        auto key = iterator->Key();
        auto value = iterator->Value();

        // Convert keys to byte arrays for comparison
        auto keyBytes = key.ToArray();
        auto prefixBytes = prefix.ToArray();

        // Skip if key doesn't start with the prefix
        if (keyBytes.Size() <= prefixBytes.Size() ||
            std::memcmp(keyBytes.Data(), prefixBytes.Data(), prefixBytes.Size()) != 0)
            break;

        // Extract public key from key (skip the prefix part)
        io::ByteVector pubKeyBytes(keyBytes.AsSpan().subspan(prefixBytes.Size()));
        cryptography::ecc::ECPoint pubKey(pubKeyBytes.ToHexString());

        // Deserialize candidate state
        auto valueBytes = value.GetValue();
        std::istringstream stream(std::string(reinterpret_cast<const char*>(valueBytes.Data()), valueBytes.Size()));
        io::BinaryReader reader(stream);

        NeoToken::CandidateState state;
        state.Deserialize(reader);

        // Only add registered candidates
        if (state.registered) candidates.push_back(std::make_pair(pubKey, state));

        iterator->Next();
    }

    return candidates;
}

int64_t NeoTokenCandidate::GetCandidateVote(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot,
                                            const cryptography::ecc::ECPoint& pubKey)
{
    auto state = GetCandidateState(token, snapshot, pubKey);
    return state.registered ? state.votes : -1;
}

void NeoTokenCandidate::CheckCandidate(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot,
                                       const cryptography::ecc::ECPoint& pubKey, const NeoToken::CandidateState& state)
{
    // Implement candidate check matching C# CheckCandidate implementation
    // Remove candidate from storage if not registered and has no votes
    if (!state.registered && state.votes == 0)
    {
        // Remove the candidate from storage
        auto key = token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::Candidate), pubKey.ToArray());
        snapshot->Delete(key);
    }
}

std::shared_ptr<vm::StackItem> NeoTokenCandidate::OnRegisterCandidate(
    const NeoToken& token, ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.size() < 1) throw std::runtime_error("Invalid number of arguments");

    auto pubKeyItem = args[0];

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

    // Check witness
    // Implement proper ECPoint to script hash conversion matching C# Contract.CreateSignatureRedeemScript
    try
    {
        // Create signature redeem script: PUSH(pubkey) + CHECKSIG
        vm::ScriptBuilder scriptBuilder;
        auto pubkeyBytes = pubKey.ToBytes();
        scriptBuilder.EmitPush(pubkeyBytes.AsSpan());
        scriptBuilder.EmitSysCall("System.Crypto.CheckSig");

        // Calculate script hash (Hash160 of the script)
        auto script = scriptBuilder.ToArray();
        io::UInt160 scriptHash = cryptography::Hash::Hash160(script.AsSpan());

        // Check witness for this script hash
        if (!engine.CheckWitness(scriptHash))
        {
            throw std::runtime_error("CheckWitness failed for candidate registration");
        }
    }
    catch (...)
    {
        throw std::runtime_error("Failed to verify witness for candidate registration");
    }

    bool result = RegisterCandidate(token, engine.GetSnapshot(), pubKey);
    return vm::StackItem::Create(result);
}

std::shared_ptr<vm::StackItem> NeoTokenCandidate::OnUnregisterCandidate(
    const NeoToken& token, ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.size() < 1) throw std::runtime_error("Invalid number of arguments");

    auto pubKeyItem = args[0];

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

    // Check witness
    // Implement proper ECPoint to script hash conversion matching C# Contract.CreateSignatureRedeemScript
    try
    {
        // Create signature redeem script: PUSH(pubkey) + CHECKSIG
        vm::ScriptBuilder scriptBuilder;
        auto pubkeyBytes = pubKey.ToBytes();
        scriptBuilder.EmitPush(pubkeyBytes.AsSpan());
        scriptBuilder.EmitSysCall("System.Crypto.CheckSig");

        // Calculate script hash (Hash160 of the script)
        auto script = scriptBuilder.ToArray();
        io::UInt160 scriptHash = cryptography::Hash::Hash160(script.AsSpan());

        // Check witness for this script hash
        if (!engine.CheckWitness(scriptHash))
        {
            throw std::runtime_error("CheckWitness failed for candidate unregistration");
        }
    }
    catch (...)
    {
        throw std::runtime_error("Failed to verify witness for candidate unregistration");
    }

    bool result = UnregisterCandidate(token, engine.GetSnapshot(), pubKey);
    return vm::StackItem::Create(result);
}

std::shared_ptr<vm::StackItem> NeoTokenCandidate::OnGetCandidates(
    const NeoToken& token, ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
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

std::shared_ptr<vm::StackItem> NeoTokenCandidate::OnGetCandidateVote(
    const NeoToken& token, ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.size() < 1) throw std::runtime_error("Invalid number of arguments");

    auto pubKeyItem = args[0];

    auto pubKeyBytes = pubKeyItem->GetByteArray();
    cryptography::ecc::ECPoint pubKey = cryptography::ecc::ECPoint(pubKeyBytes.ToHexString());

    auto vote = GetCandidateVote(token, engine.GetSnapshot(), pubKey);

    return vm::StackItem::Create(vote);
}

int64_t NeoTokenCandidate::GetRegisterPrice(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot)
{
    // Implement register price logic matching C# GetRegisterPrice implementation
    auto key = token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::RegisterPrice));
    auto item = snapshot->TryGet(key);
    if (item)
    {
        // Read the register price from storage
        std::istringstream stream(
            std::string(reinterpret_cast<const char*>(item->GetValue().Data()), item->GetValue().Size()));
        io::BinaryReader reader(stream);
        int64_t price = reader.ReadInt64();
        return price;
    }
    else
    {
        // Default register price: 1000 GAS (in datoshi units)
        return 1000LL * 100000000LL;  // 1000 GAS
    }
}

std::shared_ptr<vm::StackItem> NeoTokenCandidate::OnGetRegisterPrice(
    const NeoToken& token, ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    return vm::StackItem::Create(GetRegisterPrice(token, engine.GetSnapshot()));
}

std::shared_ptr<vm::StackItem> NeoTokenCandidate::OnSetRegisterPrice(
    const NeoToken& token, ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.size() < 1) throw std::runtime_error("Invalid number of arguments");

    auto priceItem = args[0];
    int64_t price = priceItem->GetInteger();

    // Check if caller is committee using proper committee address verification
    try
    {
        // Get committee address from NEO token contract
        io::UInt160 committeeAddress = NeoTokenCommittee::GetCommitteeAddress(token, engine.GetSnapshot());

        // Check if the committee address has witnessed the current transaction
        if (!engine.CheckWitnessInternal(committeeAddress))
        {
            throw std::runtime_error("Committee authorization required for setting register price");
        }
    }
    catch (const std::exception& e)
    {
        // Committee authorization failed - MUST deny access for security
        throw std::runtime_error(std::string("Committee authorization failed for setting register price: ") + e.what());
    }

    // Validate register price
    if (price <= 0)
    {
        throw std::invalid_argument("Register price must be positive");
    }

    // Store the register price
    auto key = token.CreateStorageKey(static_cast<uint8_t>(NeoToken::StoragePrefix::RegisterPrice));

    std::ostringstream stream;
    io::BinaryWriter writer(stream);
    writer.Write(price);
    std::string data = stream.str();

    persistence::StorageItem item(io::ByteVector(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    engine.GetSnapshot()->Add(key, item);

    return vm::StackItem::Create(true);
}
}  // namespace neo::smartcontract::native