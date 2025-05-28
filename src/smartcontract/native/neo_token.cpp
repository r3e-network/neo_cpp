#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/neo_token_account.h>
#include <neo/smartcontract/native/neo_token_candidate.h>
#include <neo/smartcontract/native/neo_token_committee.h>
#include <neo/smartcontract/native/neo_token_gas.h>
#include <neo/smartcontract/native/neo_token_vote.h>
#include <neo/smartcontract/native/neo_token_transfer.h>
#include <neo/smartcontract/native/neo_token_persistence.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/storage_item.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/vm/stack_item.h>
#include <neo/cryptography/hash.h>
#include <sstream>
#include <algorithm>

namespace neo::smartcontract::native
{
    std::shared_ptr<NeoToken> NeoToken::GetInstance()
    {
        static std::shared_ptr<NeoToken> instance = std::make_shared<NeoToken>();
        return instance;
    }

    NeoToken::NeoToken()
        : NativeContract(NAME, ID)
    {
    }

    void NeoToken::Initialize()
    {
        RegisterMethod("totalSupply", CallFlags::ReadStates, std::function<std::shared_ptr<vm::StackItem>(smartcontract::ApplicationEngine&, const std::vector<std::shared_ptr<vm::StackItem>>&)>([this](smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args) {
            return NeoTokenTransfer::OnTotalSupply(*this, engine, args);
        }));
        RegisterMethod("symbol", CallFlags::ReadStates, std::function<std::shared_ptr<vm::StackItem>(smartcontract::ApplicationEngine&, const std::vector<std::shared_ptr<vm::StackItem>>&)>([](smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args) {
            return vm::StackItem::Create(SYMBOL);
        }));
        RegisterMethod("decimals", CallFlags::ReadStates, std::function<std::shared_ptr<vm::StackItem>(smartcontract::ApplicationEngine&, const std::vector<std::shared_ptr<vm::StackItem>>&)>([](smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args) {
            return vm::StackItem::Create(static_cast<int64_t>(DECIMALS));
        }));
        RegisterMethod("balanceOf", CallFlags::ReadStates, std::function<std::shared_ptr<vm::StackItem>(smartcontract::ApplicationEngine&, const std::vector<std::shared_ptr<vm::StackItem>>&)>([this](smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args) {
            return NeoTokenAccount::OnBalanceOf(*this, engine, args);
        }));
        RegisterMethod("transfer", CallFlags::States | CallFlags::AllowNotify, std::function<std::shared_ptr<vm::StackItem>(smartcontract::ApplicationEngine&, const std::vector<std::shared_ptr<vm::StackItem>>&)>([this](smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args) {
            return NeoTokenTransfer::OnTransfer(*this, engine, args);
        }));
        RegisterMethod("getValidators", CallFlags::ReadStates, std::function<std::shared_ptr<vm::StackItem>(smartcontract::ApplicationEngine&, const std::vector<std::shared_ptr<vm::StackItem>>&)>([this](smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args) {
            return NeoTokenCommittee::OnGetValidators(*this, engine, args);
        }));
        RegisterMethod("getCommittee", CallFlags::ReadStates, std::function<std::shared_ptr<vm::StackItem>(smartcontract::ApplicationEngine&, const std::vector<std::shared_ptr<vm::StackItem>>&)>([this](smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args) {
            return NeoTokenCommittee::OnGetCommittee(*this, engine, args);
        }));
        RegisterMethod("getNextBlockValidators", CallFlags::ReadStates, std::function<std::shared_ptr<vm::StackItem>(smartcontract::ApplicationEngine&, const std::vector<std::shared_ptr<vm::StackItem>>&)>([this](smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args) {
            return NeoTokenCommittee::OnGetNextBlockValidators(*this, engine, args);
        }));
        RegisterMethod("registerCandidate", CallFlags::States | CallFlags::AllowNotify, std::function<std::shared_ptr<vm::StackItem>(smartcontract::ApplicationEngine&, const std::vector<std::shared_ptr<vm::StackItem>>&)>([this](smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args) {
            return NeoTokenCandidate::OnRegisterCandidate(*this, engine, args);
        }));
        RegisterMethod("unregisterCandidate", CallFlags::States | CallFlags::AllowNotify, std::function<std::shared_ptr<vm::StackItem>(smartcontract::ApplicationEngine&, const std::vector<std::shared_ptr<vm::StackItem>>&)>([this](smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args) {
            return NeoTokenCandidate::OnUnregisterCandidate(*this, engine, args);
        }));
        RegisterMethod("vote", CallFlags::States | CallFlags::AllowNotify, std::function<std::shared_ptr<vm::StackItem>(smartcontract::ApplicationEngine&, const std::vector<std::shared_ptr<vm::StackItem>>&)>([this](smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args) {
            return NeoTokenVote::OnVote(*this, engine, args);
        }));
        RegisterMethod("getGasPerBlock", CallFlags::ReadStates, std::function<std::shared_ptr<vm::StackItem>(smartcontract::ApplicationEngine&, const std::vector<std::shared_ptr<vm::StackItem>>&)>([this](smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args) {
            return NeoTokenGas::OnGetGasPerBlock(*this, engine, args);
        }));
        RegisterMethod("setGasPerBlock", CallFlags::States, std::function<std::shared_ptr<vm::StackItem>(smartcontract::ApplicationEngine&, const std::vector<std::shared_ptr<vm::StackItem>>&)>([this](smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args) {
            return NeoTokenGas::OnSetGasPerBlock(*this, engine, args);
        }));
        RegisterMethod("getAccountState", CallFlags::ReadStates, std::function<std::shared_ptr<vm::StackItem>(smartcontract::ApplicationEngine&, const std::vector<std::shared_ptr<vm::StackItem>>&)>([this](smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args) {
            return NeoTokenAccount::OnGetAccountState(*this, engine, args);
        }));
        RegisterMethod("getCandidates", CallFlags::ReadStates, std::function<std::shared_ptr<vm::StackItem>(smartcontract::ApplicationEngine&, const std::vector<std::shared_ptr<vm::StackItem>>&)>([this](smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args) {
            return NeoTokenCandidate::OnGetCandidates(*this, engine, args);
        }));
        RegisterMethod("getCandidateVote", CallFlags::ReadStates, std::function<std::shared_ptr<vm::StackItem>(smartcontract::ApplicationEngine&, const std::vector<std::shared_ptr<vm::StackItem>>&)>([this](smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args) {
            return NeoTokenCandidate::OnGetCandidateVote(*this, engine, args);
        }));
        RegisterMethod("getRegisterPrice", CallFlags::ReadStates, std::function<std::shared_ptr<vm::StackItem>(smartcontract::ApplicationEngine&, const std::vector<std::shared_ptr<vm::StackItem>>&)>([this](smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args) {
            return NeoTokenCandidate::OnGetRegisterPrice(*this, engine, args);
        }));
        RegisterMethod("setRegisterPrice", CallFlags::States, std::function<std::shared_ptr<vm::StackItem>(smartcontract::ApplicationEngine&, const std::vector<std::shared_ptr<vm::StackItem>>&)>([this](smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args) {
            return NeoTokenCandidate::OnSetRegisterPrice(*this, engine, args);
        }));
        RegisterMethod("getUnclaimedGas", CallFlags::ReadStates, std::function<std::shared_ptr<vm::StackItem>(smartcontract::ApplicationEngine&, const std::vector<std::shared_ptr<vm::StackItem>>&)>([this](smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args) {
            return NeoTokenGas::OnGetUnclaimedGas(*this, engine, args);
        }));
    }

    io::Fixed8 NeoToken::GetTotalSupply(std::shared_ptr<persistence::DataCache> snapshot) const
    {
        return NeoTokenTransfer::GetTotalSupply(*this, snapshot);
    }

    io::Fixed8 NeoToken::GetBalance(std::shared_ptr<persistence::DataCache> snapshot, const io::UInt160& account) const
    {
        return NeoTokenAccount::GetBalance(*this, snapshot, account);
    }

    bool NeoToken::Transfer(std::shared_ptr<persistence::DataCache> snapshot, const io::UInt160& from, const io::UInt160& to, const io::Fixed8& amount)
    {
        return NeoTokenTransfer::Transfer(*this, snapshot, from, to, amount);
    }

    std::vector<cryptography::ecc::ECPoint> NeoToken::GetValidators(std::shared_ptr<persistence::DataCache> snapshot) const
    {
        return NeoTokenCommittee::GetValidators(*this, snapshot);
    }

    std::vector<cryptography::ecc::ECPoint> NeoToken::GetCommittee(std::shared_ptr<persistence::DataCache> snapshot) const
    {
        return NeoTokenCommittee::GetCommittee(*this, snapshot);
    }

    std::vector<cryptography::ecc::ECPoint> NeoToken::GetNextBlockValidators(std::shared_ptr<persistence::DataCache> snapshot, int32_t validatorsCount) const
    {
        return NeoTokenCommittee::GetNextBlockValidators(*this, snapshot, validatorsCount);
    }

    int64_t NeoToken::GetRegisterPrice(std::shared_ptr<persistence::DataCache> snapshot) const
    {
        return NeoTokenCandidate::GetRegisterPrice(*this, snapshot);
    }

    bool NeoToken::RegisterCandidate(std::shared_ptr<persistence::DataCache> snapshot, const cryptography::ecc::ECPoint& pubKey)
    {
        return NeoTokenCandidate::RegisterCandidate(*this, snapshot, pubKey);
    }

    bool NeoToken::UnregisterCandidate(std::shared_ptr<persistence::DataCache> snapshot, const cryptography::ecc::ECPoint& pubKey)
    {
        return NeoTokenCandidate::UnregisterCandidate(*this, snapshot, pubKey);
    }

    bool NeoToken::Vote(std::shared_ptr<persistence::DataCache> snapshot, const io::UInt160& account, const std::vector<cryptography::ecc::ECPoint>& pubKeys)
    {
        return NeoTokenVote::Vote(*this, snapshot, account, pubKeys);
    }



    std::shared_ptr<vm::StackItem> NeoToken::OnTotalSupply(neo::smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        return NeoTokenTransfer::OnTotalSupply(*this, engine, args);
    }

    std::shared_ptr<vm::StackItem> NeoToken::OnBalanceOf(neo::smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        return NeoTokenAccount::OnBalanceOf(*this, engine, args);
    }

    std::shared_ptr<vm::StackItem> NeoToken::OnTransfer(neo::smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        return NeoTokenTransfer::OnTransfer(*this, engine, args);
    }

    std::shared_ptr<vm::StackItem> NeoToken::OnGetValidators(neo::smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        return NeoTokenCommittee::OnGetValidators(*this, engine, args);
    }

    std::shared_ptr<vm::StackItem> NeoToken::OnGetCommittee(neo::smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        return NeoTokenCommittee::OnGetCommittee(*this, engine, args);
    }

    std::shared_ptr<vm::StackItem> NeoToken::OnGetNextBlockValidators(neo::smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        return NeoTokenCommittee::OnGetNextBlockValidators(*this, engine, args);
    }

    std::shared_ptr<vm::StackItem> NeoToken::OnRegisterCandidate(neo::smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        return NeoTokenCandidate::OnRegisterCandidate(*this, engine, args);
    }

    std::shared_ptr<vm::StackItem> NeoToken::OnUnregisterCandidate(neo::smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        return NeoTokenCandidate::OnUnregisterCandidate(*this, engine, args);
    }

    std::shared_ptr<vm::StackItem> NeoToken::OnVote(neo::smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        return NeoTokenVote::OnVote(*this, engine, args);
    }

    void NeoToken::AccountState::Serialize(io::BinaryWriter& writer) const
    {
        writer.Write(balance);
        writer.Write(balanceHeight);
        auto voteToBytes = voteTo.ToArray();
        writer.Write(io::ByteSpan(voteToBytes.Data(), voteToBytes.Size()));
        writer.Write(lastGasPerVote);
    }

    void NeoToken::AccountState::Deserialize(io::BinaryReader& reader)
    {
        balance = reader.ReadInt64();
        balanceHeight = reader.ReadUInt32();
        voteTo = reader.ReadSerializable<cryptography::ecc::ECPoint>();
        lastGasPerVote = reader.ReadInt64();
    }

    void NeoToken::CandidateState::Serialize(io::BinaryWriter& writer) const
    {
        writer.Write(registered);
        writer.Write(votes);
    }

    void NeoToken::CandidateState::Deserialize(io::BinaryReader& reader)
    {
        registered = reader.ReadBool();
        votes = reader.ReadInt64();
    }

    void NeoToken::CommitteeMember::Serialize(io::BinaryWriter& writer) const
    {
        auto publicKeyBytes = publicKey.ToArray();
        writer.Write(io::ByteSpan(publicKeyBytes.Data(), publicKeyBytes.Size()));
        writer.Write(votes);
    }

    void NeoToken::CommitteeMember::Deserialize(io::BinaryReader& reader)
    {
        publicKey = reader.ReadSerializable<cryptography::ecc::ECPoint>();
        votes = reader.ReadInt64();
    }

    int64_t NeoToken::GetGasPerBlock(std::shared_ptr<persistence::DataCache> snapshot) const
    {
        return NeoTokenGas::GetGasPerBlock(*this, snapshot);
    }

    void NeoToken::SetGasPerBlock(std::shared_ptr<persistence::DataCache> snapshot, int64_t gasPerBlock)
    {
        NeoTokenGas::SetGasPerBlock(*this, snapshot, gasPerBlock);
    }

    NeoToken::GasDistribution NeoToken::DistributeGas(neo::smartcontract::ApplicationEngine& engine, const io::UInt160& account, const AccountState& state)
    {
        return NeoTokenGas::DistributeGas(*this, engine, account, state);
    }

    int64_t NeoToken::GetUnclaimedGas(std::shared_ptr<persistence::DataCache> snapshot, const io::UInt160& account, uint32_t end) const
    {
        return NeoTokenGas::GetUnclaimedGas(*this, snapshot, account, end);
    }

    int64_t NeoToken::CalculateBonus(std::shared_ptr<persistence::DataCache> snapshot, const AccountState& state, uint32_t end) const
    {
        return NeoTokenGas::CalculateBonus(*this, snapshot, state, end);
    }

    int64_t NeoToken::CalculateNeoHolderReward(std::shared_ptr<persistence::DataCache> snapshot, int64_t value, uint32_t start, uint32_t end) const
    {
        return NeoTokenGas::CalculateNeoHolderReward(*this, snapshot, value, start, end);
    }

    std::vector<cryptography::ecc::ECPoint> NeoToken::ComputeCommitteeMembers(std::shared_ptr<persistence::DataCache> snapshot, int32_t committeeSize) const
    {
        return NeoTokenCommittee::ComputeCommitteeMembers(*this, snapshot, committeeSize);
    }

    bool NeoToken::ShouldRefreshCommittee(uint32_t blockIndex, int32_t committeeSize) const
    {
        return NeoTokenCommittee::ShouldRefreshCommittee(*this, blockIndex, committeeSize);
    }

    std::vector<NeoToken::CommitteeMember> NeoToken::GetCommitteeFromCache(std::shared_ptr<persistence::DataCache> snapshot) const
    {
        return NeoTokenCommittee::GetCommitteeFromCache(*this, snapshot);
    }

    bool NeoToken::InitializeContract(neo::smartcontract::ApplicationEngine& engine, uint32_t hardfork)
    {
        return NeoTokenPersistence::InitializeContract(*this, engine, hardfork);
    }

    bool NeoToken::OnPersist(neo::smartcontract::ApplicationEngine& engine)
    {
        return NeoTokenPersistence::OnPersist(*this, engine);
    }

    bool NeoToken::PostPersist(neo::smartcontract::ApplicationEngine& engine)
    {
        return NeoTokenPersistence::PostPersist(*this, engine);
    }

    std::shared_ptr<vm::StackItem> NeoToken::OnGetGasPerBlock(neo::smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        return NeoTokenGas::OnGetGasPerBlock(*this, engine, args);
    }

    std::shared_ptr<vm::StackItem> NeoToken::OnSetGasPerBlock(neo::smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        return NeoTokenGas::OnSetGasPerBlock(*this, engine, args);
    }

    NeoToken::AccountState NeoToken::GetAccountState(std::shared_ptr<persistence::DataCache> snapshot, const io::UInt160& account) const
    {
        return NeoTokenAccount::GetAccountState(*this, snapshot, account);
    }

    NeoToken::CandidateState NeoToken::GetCandidateState(std::shared_ptr<persistence::DataCache> snapshot, const cryptography::ecc::ECPoint& pubKey) const
    {
        return NeoTokenCandidate::GetCandidateState(*this, snapshot, pubKey);
    }

    std::vector<std::pair<cryptography::ecc::ECPoint, NeoToken::CandidateState>> NeoToken::GetCandidates(std::shared_ptr<persistence::DataCache> snapshot) const
    {
        return NeoTokenCandidate::GetCandidates(*this, snapshot);
    }

    int64_t NeoToken::GetCandidateVote(std::shared_ptr<persistence::DataCache> snapshot, const cryptography::ecc::ECPoint& pubKey) const
    {
        return NeoTokenCandidate::GetCandidateVote(*this, snapshot, pubKey);
    }

    void NeoToken::CheckCandidate(std::shared_ptr<persistence::DataCache> snapshot, const cryptography::ecc::ECPoint& pubKey, const CandidateState& state)
    {
        NeoTokenCandidate::CheckCandidate(*this, snapshot, pubKey, state);
    }

    std::shared_ptr<vm::StackItem> NeoToken::OnGetAccountState(neo::smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        return NeoTokenAccount::OnGetAccountState(*this, engine, args);
    }

    std::shared_ptr<vm::StackItem> NeoToken::OnGetCandidates(neo::smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        return NeoTokenCandidate::OnGetCandidates(*this, engine, args);
    }

    std::shared_ptr<vm::StackItem> NeoToken::OnGetCandidateVote(neo::smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        return NeoTokenCandidate::OnGetCandidateVote(*this, engine, args);
    }

    std::shared_ptr<vm::StackItem> NeoToken::OnGetRegisterPrice(neo::smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        return NeoTokenCandidate::OnGetRegisterPrice(*this, engine, args);
    }

    std::shared_ptr<vm::StackItem> NeoToken::OnSetRegisterPrice(neo::smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        return NeoTokenCandidate::OnSetRegisterPrice(*this, engine, args);
    }

    std::shared_ptr<vm::StackItem> NeoToken::OnGetUnclaimedGas(neo::smartcontract::ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        return NeoTokenGas::OnGetUnclaimedGas(*this, engine, args);
    }

    io::UInt160 NeoToken::GetCommitteeAddress(std::shared_ptr<persistence::DataCache> snapshot) const
    {
        return NeoTokenCommittee::GetCommitteeAddress(*this, snapshot);
    }

    io::ByteVector NeoToken::GetStoragePrefix() const
    {
        io::ByteVector prefix;
        prefix.Push(static_cast<uint8_t>(ID));
        return prefix;
    }
}
