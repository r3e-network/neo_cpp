#include <gtest/gtest.h>

#include <neo/cryptography/crypto.h>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/io/byte_vector.h>
#include <neo/io/memory_stream.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/ledger/conflicts_attribute.h>
#include <neo/ledger/high_priority_attribute.h>
#include <neo/ledger/not_valid_before.h>
#include <neo/ledger/oracle_response.h>
#include <neo/network/p2p/payloads/oracle_response_code.h>
#include <neo/ledger/signer.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/transaction_attribute.h>
#include <neo/ledger/witness.h>
#include <neo/vm/script_builder.h>


#include <algorithm>
#include <array>
#include <memory>
#include <random>
#include <vector>

using namespace neo::ledger;
using namespace neo::cryptography;
using namespace neo::cryptography::ecc;
using namespace neo::io;
using namespace neo::vm;

namespace
{
UInt256 MakeRandomHash()
{
    std::array<uint8_t, UInt256::Size> data{};
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 255);
    for (auto& byte : data)
    {
        byte = static_cast<uint8_t>(dist(gen));
    }
    return UInt256(data);
}

constexpr uint32_t kCheckSigSysCall = 0x41627d5b;      // System.Crypto.CheckSig
constexpr uint32_t kCheckMultiSigSysCall = 0x0973c0b6;  // System.Crypto.CheckMultisig
}  // namespace

class TransactionCompleteTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        tx_ = std::make_unique<Transaction>();

        static const char* kSampleKeys[] = {
            "03b209fd4f53a7170ea4444e0cb0a6bb6a53c2bd016926989cf85f9b0fba17a70c",
            "02a7834be9b32e2981d157cb5bbd3acb42cfd11ea5c3b10224d7a44e98c5910f1b",
            "0214baf0ceea3a66f17e7e1e839ea25fd8bed6cd82e6bb6e68250189065f44ff01"};

        keys_.clear();
        for (const auto* hex : kSampleKeys)
        {
            keys_.push_back(ECPoint::FromHex(hex));
        }
    }

    static ByteVector BuildSingleSigVerificationScript(const ECPoint& publicKey)
    {
        ScriptBuilder sb;
        sb.EmitPush(publicKey);
        sb.EmitSysCall(kCheckSigSysCall);
        return sb.ToArray();
    }

    static ByteVector BuildMultiSigVerificationScript(const std::vector<ECPoint>& publicKeys, int m)
    {
        ScriptBuilder sb;
        sb.EmitPushNumber(m);
        for (const auto& key : publicKeys)
        {
            sb.EmitPush(key);
        }
        sb.EmitPushNumber(static_cast<int>(publicKeys.size()));
        sb.EmitSysCall(kCheckMultiSigSysCall);
        return sb.ToArray();
    }

    static Witness BuildWitness(const ECPoint& publicKey)
    {
        ByteVector signature(64);
        std::fill(signature.Data(), signature.Data() + signature.Size(), 0x01);

        ScriptBuilder invocationBuilder;
        invocationBuilder.EmitPush(signature);

        Witness witness;
        witness.SetInvocationScript(invocationBuilder.ToArray());
        witness.SetVerificationScript(BuildSingleSigVerificationScript(publicKey));
        return witness;
    }

    void AddSigner(const Signer& signer)
    {
        auto signers = tx_->GetSigners();
        signers.push_back(signer);
        tx_->SetSigners(signers);
    }

    void AddAttribute(const std::shared_ptr<TransactionAttribute>& attribute)
    {
        auto attributes = tx_->GetAttributes();
        attributes.push_back(attribute);
        tx_->SetAttributes(attributes);
    }

    void AddWitness(const Witness& witness)
    {
        auto witnesses = tx_->GetWitnesses();
        witnesses.push_back(witness);
        tx_->SetWitnesses(witnesses);
    }

    static UInt160 ComputeScriptHash(const ECPoint& publicKey)
    {
        ScriptBuilder sb;
        sb.EmitPush(publicKey);
        sb.EmitSysCall(kCheckSigSysCall);
        auto script = sb.ToArray();
        return Crypto::Hash160(script.AsSpan());
    }

    std::unique_ptr<Transaction> tx_;
    std::vector<ECPoint> keys_;
};

TEST_F(TransactionCompleteTest, Transaction_DefaultValues)
{
    EXPECT_EQ(tx_->GetVersion(), 0);
    EXPECT_EQ(tx_->GetNonce(), 0u);
    EXPECT_EQ(tx_->GetSystemFee(), 0);
    EXPECT_EQ(tx_->GetNetworkFee(), 0);
    EXPECT_EQ(tx_->GetValidUntilBlock(), 0u);
    EXPECT_TRUE(tx_->GetSigners().empty());
    EXPECT_TRUE(tx_->GetAttributes().empty());
    EXPECT_TRUE(tx_->GetWitnesses().empty());
    EXPECT_TRUE(tx_->GetScript().IsEmpty());
}

TEST_F(TransactionCompleteTest, Transaction_SettersPersistValues)
{
    tx_->SetVersion(0);
    tx_->SetNonce(123456u);
    tx_->SetSystemFee(1'000'000);
    tx_->SetNetworkFee(250'000);
    tx_->SetValidUntilBlock(5000u);
    auto script = ByteVector::FromString("test-script");
    tx_->SetScript(script);

    EXPECT_EQ(tx_->GetVersion(), 0);
    EXPECT_EQ(tx_->GetNonce(), 123456u);
    EXPECT_EQ(tx_->GetSystemFee(), 1'000'000);
    EXPECT_EQ(tx_->GetNetworkFee(), 250'000);
    EXPECT_EQ(tx_->GetValidUntilBlock(), 5000u);
    EXPECT_EQ(tx_->GetScript(), script);
    EXPECT_EQ(tx_->GetTotalFee(), 1'250'000);
}

TEST_F(TransactionCompleteTest, Transaction_HashChangesWhenMutated)
{
    tx_->SetNonce(42);
    tx_->SetSystemFee(2'000'000);
    tx_->SetNetworkFee(750'000);
    tx_->SetValidUntilBlock(12345);
    tx_->SetScript(ByteVector::FromString("execution-payload"));

    Signer signer;
    signer.SetAccount(ComputeScriptHash(keys_[0]));
    signer.SetScopes(WitnessScope::CalledByEntry);
    AddSigner(signer);

    auto hash1 = tx_->GetHash();
    auto hash2 = tx_->GetHash();
    EXPECT_EQ(hash1, hash2);

    tx_->SetNonce(43);
    auto hash3 = tx_->GetHash();
    EXPECT_NE(hash1, hash3);
}

TEST_F(TransactionCompleteTest, Transaction_SerializeRoundTrip)
{
    tx_->SetVersion(0);
    tx_->SetNonce(777);
    tx_->SetSystemFee(5'000'000);
    tx_->SetNetworkFee(1'500'000);
    tx_->SetValidUntilBlock(99999);
    tx_->SetScript(ByteVector::FromString("serialize-this"));

    Signer signer;
    signer.SetAccount(ComputeScriptHash(keys_[0]));
    signer.SetScopes(WitnessScope::CalledByEntry);
    AddSigner(signer);

    Witness witness = BuildWitness(keys_[0]);
    AddWitness(witness);

    Transaction deserialized(*tx_);

    EXPECT_EQ(deserialized.GetVersion(), tx_->GetVersion());
    EXPECT_EQ(deserialized.GetNonce(), tx_->GetNonce());
    EXPECT_EQ(deserialized.GetSystemFee(), tx_->GetSystemFee());
    EXPECT_EQ(deserialized.GetNetworkFee(), tx_->GetNetworkFee());
    EXPECT_EQ(deserialized.GetValidUntilBlock(), tx_->GetValidUntilBlock());
    EXPECT_EQ(deserialized.GetScript(), tx_->GetScript());
    EXPECT_EQ(deserialized.GetSigners().size(), tx_->GetSigners().size());
    EXPECT_EQ(deserialized.GetWitnesses().size(), tx_->GetWitnesses().size());
}

TEST_F(TransactionCompleteTest, Signer_CollectionManagement)
{
    Signer signerA;
    signerA.SetAccount(ComputeScriptHash(keys_[0]));
    signerA.SetScopes(WitnessScope::CalledByEntry);
    AddSigner(signerA);

    Signer signerB;
    signerB.SetAccount(ComputeScriptHash(keys_[1]));
    signerB.SetScopes(WitnessScope::CustomContracts);
    AddSigner(signerB);

    const auto& signers = tx_->GetSigners();
    ASSERT_EQ(signers.size(), 2u);
    EXPECT_NE(signers[0].GetAccount(), signers[1].GetAccount());
    EXPECT_EQ(signers[0].GetScopes(), WitnessScope::CalledByEntry);
    EXPECT_EQ(signers[1].GetScopes(), WitnessScope::CustomContracts);
}

TEST_F(TransactionCompleteTest, Witness_ComputesScriptHash)
{
    tx_->SetScript(ByteVector::FromString("payload"));
    auto hash = tx_->GetHash();

    Witness witness = BuildWitness(keys_[0]);
    auto expectedHash = ComputeScriptHash(keys_[0]);

    EXPECT_EQ(witness.GetScriptHash(), expectedHash);
    EXPECT_FALSE(witness.GetInvocationScript().IsEmpty());
    EXPECT_FALSE(witness.GetVerificationScript().IsEmpty());
}

TEST_F(TransactionCompleteTest, Witness_MultiSigScripts)
{
    std::vector<ECPoint> publicKeys = {
        keys_[0],
        keys_[1],
        keys_[2],
    };

    auto verificationScript = BuildMultiSigVerificationScript(publicKeys, 2);
    EXPECT_GT(verificationScript.Size(), 0u);

    ScriptBuilder invocationBuilder;
    std::vector<uint8_t> sig_a(64, 0xAA);
    std::vector<uint8_t> sig_b(64, 0xBB);
    invocationBuilder.EmitPush(ByteVector(sig_a));
    invocationBuilder.EmitPush(ByteVector(sig_b));

    Witness witness;
    witness.SetVerificationScript(verificationScript);
    witness.SetInvocationScript(invocationBuilder.ToArray());

    auto invocationScript = invocationBuilder.ToArray();

    EXPECT_EQ(witness.GetVerificationScript(), verificationScript);
    EXPECT_EQ(witness.GetInvocationScript(), invocationScript);
}

TEST_F(TransactionCompleteTest, Attributes_AttachedToTransaction)
{
    auto highPriority = std::make_shared<HighPriorityAttribute>();
    auto notValidBefore = std::make_shared<NotValidBefore>();
    notValidBefore->SetHeight(1234);

    auto conflicts = std::make_shared<ConflictsAttribute>();
    conflicts->SetHash(MakeRandomHash());

    auto oracle = std::make_shared<OracleResponse>();
    oracle->SetId(42);
    oracle->SetCode(OracleResponseCode::Success);
    oracle->SetResult(ByteVector::FromString("oracle-result"));

    AddAttribute(highPriority);
    AddAttribute(notValidBefore);
    AddAttribute(conflicts);
    AddAttribute(oracle);

    const auto& attributes = tx_->GetAttributes();
    ASSERT_EQ(attributes.size(), 4u);
    EXPECT_NE(std::dynamic_pointer_cast<HighPriorityAttribute>(attributes[0]), nullptr);
    EXPECT_NE(std::dynamic_pointer_cast<NotValidBefore>(attributes[1]), nullptr);
    EXPECT_NE(std::dynamic_pointer_cast<ConflictsAttribute>(attributes[2]), nullptr);
    EXPECT_NE(std::dynamic_pointer_cast<OracleResponse>(attributes[3]), nullptr);
}

TEST_F(TransactionCompleteTest, Transaction_GetScriptHashesForVerifyingMatchesSigners)
{
    for (const auto& key : keys_)
    {
        Signer signer;
        signer.SetAccount(ComputeScriptHash(key));
        signer.SetScopes(WitnessScope::CalledByEntry);
        AddSigner(signer);
    }

    auto hashes = tx_->GetScriptHashesForVerifying();
    ASSERT_EQ(hashes.size(), keys_.size());
    for (size_t i = 0; i < hashes.size(); ++i)
    {
        EXPECT_EQ(hashes[i], tx_->GetSigners()[i].GetAccount());
    }
}
