#include <gtest/gtest.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/persistence/memory_store_view.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/cryptography/hash.h>
#include <sstream>

using namespace neo::smartcontract::native;
using namespace neo::persistence;
using namespace neo::io;
using namespace neo::vm;
using namespace neo::cryptography;

class NeoTokenTest : public ::testing::Test
{
protected:
    std::shared_ptr<MemoryStoreView> snapshot;
    std::shared_ptr<NeoToken> neoToken;
    std::shared_ptr<ApplicationEngine> engine;
    
    void SetUp() override
    {
        snapshot = std::make_shared<MemoryStoreView>();
        neoToken = NeoToken::GetInstance();
        engine = std::make_shared<ApplicationEngine>(TriggerType::Application, nullptr, snapshot, 0, false);
    }
};

TEST_F(NeoTokenTest, TestSymbol)
{
    // Call the symbol method
    auto result = neoToken->Call(*engine, "symbol", {});
    
    // Check the result
    ASSERT_TRUE(result->IsString());
    ASSERT_EQ(result->GetString(), "NEO");
}

TEST_F(NeoTokenTest, TestDecimals)
{
    // Call the decimals method
    auto result = neoToken->Call(*engine, "decimals", {});
    
    // Check the result
    ASSERT_TRUE(result->IsInteger());
    ASSERT_EQ(result->GetInteger(), 0);
}

TEST_F(NeoTokenTest, TestTotalSupply)
{
    // Initialize the contract
    neoToken->InitializeContract(*engine, 0);
    
    // Call the totalSupply method
    auto result = neoToken->Call(*engine, "totalSupply", {});
    
    // Check the result
    ASSERT_TRUE(result->IsInteger());
    ASSERT_EQ(result->GetInteger(), NeoToken::TOTAL_AMOUNT);
}

TEST_F(NeoTokenTest, TestGetCommittee)
{
    // Initialize the contract
    neoToken->InitializeContract(*engine, 0);
    
    // Call the getCommittee method
    auto result = neoToken->Call(*engine, "getCommittee", {});
    
    // Check the result
    ASSERT_TRUE(result->IsArray());
    auto committee = result->GetArray();
    ASSERT_GT(committee.size(), 0);
    
    // Check that each committee member is a public key
    for (const auto& member : committee)
    {
        ASSERT_TRUE(member->IsBuffer());
        auto pubKeyBytes = member->GetByteArray();
        ASSERT_TRUE(pubKeyBytes.Size() == 33);
    }
}

TEST_F(NeoTokenTest, TestGetNextBlockValidators)
{
    // Initialize the contract
    neoToken->InitializeContract(*engine, 0);
    
    // Call the getNextBlockValidators method
    auto result = neoToken->Call(*engine, "getNextBlockValidators", {});
    
    // Check the result
    ASSERT_TRUE(result->IsArray());
    auto validators = result->GetArray();
    ASSERT_GT(validators.size(), 0);
    
    // Check that each validator is a public key
    for (const auto& validator : validators)
    {
        ASSERT_TRUE(validator->IsBuffer());
        auto pubKeyBytes = validator->GetByteArray();
        ASSERT_TRUE(pubKeyBytes.Size() == 33);
    }
}

TEST_F(NeoTokenTest, TestRegisterCandidate)
{
    // Initialize the contract
    neoToken->InitializeContract(*engine, 0);
    
    // Create a key pair
    ByteVector privateKey = ByteVector::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    auto publicKey = ecc::Secp256r1::GeneratePublicKey(privateKey.AsSpan());
    
    // Create the account
    UInt160 account = Hash::Hash160(publicKey.ToArray().AsSpan());
    
    // Set the current script hash to the account
    engine->SetCurrentScriptHash(account);
    
    // Call the registerCandidate method
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(publicKey.ToArray()));
    auto result = neoToken->Call(*engine, "registerCandidate", args);
    
    // Check the result
    ASSERT_TRUE(result->IsBoolean());
    ASSERT_TRUE(result->GetBoolean());
    
    // Call the getCandidates method
    auto candidatesResult = neoToken->Call(*engine, "getCandidates", {});
    
    // Check the result
    ASSERT_TRUE(candidatesResult->IsArray());
    auto candidates = candidatesResult->GetArray();
    ASSERT_EQ(candidates.size(), 1);
    
    // Check the candidate
    auto candidate = candidates[0];
    ASSERT_TRUE(candidate->IsStruct());
    auto candidateStruct = candidate->GetStruct();
    ASSERT_EQ(candidateStruct->Count(), 2);
    ASSERT_TRUE(candidateStruct->Get(0)->IsBuffer());
    ASSERT_EQ(candidateStruct->Get(0)->GetByteArray(), publicKey.ToArray());
    ASSERT_TRUE(candidateStruct->Get(1)->IsInteger());
    ASSERT_EQ(candidateStruct->Get(1)->GetInteger(), 0);
}

TEST_F(NeoTokenTest, TestVote)
{
    // Initialize the contract
    neoToken->InitializeContract(*engine, 0);
    
    // Create a key pair
    ByteVector privateKey = ByteVector::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    auto publicKey = ecc::Secp256r1::GeneratePublicKey(privateKey.AsSpan());
    
    // Create the account
    UInt160 account = Hash::Hash160(publicKey.ToArray().AsSpan());
    
    // Set the current script hash to the account
    engine->SetCurrentScriptHash(account);
    
    // Register the candidate
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(publicKey.ToArray()));
    neoToken->Call(*engine, "registerCandidate", args);
    
    // Transfer some NEO to the account
    UInt160 committeeAddress = neoToken->GetCommitteeAddress(snapshot);
    neoToken->Transfer(snapshot, committeeAddress, account, Fixed8(100));
    
    // Vote for the candidate
    args.clear();
    args.push_back(StackItem::Create(account));
    args.push_back(StackItem::CreateArray({ StackItem::Create(publicKey.ToArray()) }));
    auto result = neoToken->Call(*engine, "vote", args);
    
    // Check the result
    ASSERT_TRUE(result->IsBoolean());
    ASSERT_TRUE(result->GetBoolean());
    
    // Call the getCandidateVote method
    args.clear();
    args.push_back(StackItem::Create(publicKey.ToArray()));
    auto voteResult = neoToken->Call(*engine, "getCandidateVote", args);
    
    // Check the result
    ASSERT_TRUE(voteResult->IsInteger());
    ASSERT_EQ(voteResult->GetInteger(), 100);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
