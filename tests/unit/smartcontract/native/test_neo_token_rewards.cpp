#include <gtest/gtest.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/trigger_type.h>
#include <neo/persistence/memory_store_view.h>
#include <neo/ledger/block.h>
#include <neo/core/fixed8.h>

using namespace neo::smartcontract::native;
using namespace neo::smartcontract;
using namespace neo::persistence;
using namespace neo::ledger;
using namespace neo::vm;
using namespace neo::core;

class NeoTokenRewardsTest : public ::testing::Test
{
protected:
    std::shared_ptr<MemoryStoreView> snapshot;
    std::shared_ptr<NeoToken> neoToken;
    std::shared_ptr<GasToken> gasToken;
    std::shared_ptr<ApplicationEngine> engine;
    
    void SetUp() override
    {
        snapshot = std::make_shared<MemoryStoreView>();
        neoToken = NeoToken::GetInstance();
        gasToken = GasToken::GetInstance();
        engine = std::make_shared<ApplicationEngine>(TriggerType::Application, nullptr, snapshot, nullptr, 0);
    }
};

// Test that reward ratios are correctly defined
TEST_F(NeoTokenRewardsTest, TestRewardRatioConstants)
{
    // Test that the reward ratios are correctly set
    EXPECT_EQ(NeoToken::NEO_HOLDER_REWARD_RATIO, 10); // 10% for NEO holders
    EXPECT_EQ(NeoToken::VOTER_REWARD_RATIO, 80);      // 80% for voters
    
    // Committee gets the remaining 10% (100 - 10 - 80 = 10)
    int committee_ratio = 100 - NeoToken::NEO_HOLDER_REWARD_RATIO - NeoToken::VOTER_REWARD_RATIO;
    EXPECT_EQ(committee_ratio, 10);
}

// Test GAS distribution calculation
TEST_F(NeoTokenRewardsTest, TestGasDistributionCalculation)
{
    // Simulate a block with system fees
    const int64_t total_system_fee = 10000000000; // 100 GAS in Fixed8
    
    // Calculate expected distributions
    int64_t neo_holder_reward = total_system_fee * NeoToken::NEO_HOLDER_REWARD_RATIO / 100;
    int64_t voter_reward = total_system_fee * NeoToken::VOTER_REWARD_RATIO / 100;
    int64_t committee_reward = total_system_fee - neo_holder_reward - voter_reward;
    
    // Verify calculations
    EXPECT_EQ(neo_holder_reward, 1000000000);  // 10 GAS (10%)
    EXPECT_EQ(voter_reward, 8000000000);       // 80 GAS (80%)
    EXPECT_EQ(committee_reward, 1000000000);   // 10 GAS (10%)
    
    // Total should equal original amount
    EXPECT_EQ(neo_holder_reward + voter_reward + committee_reward, total_system_fee);
}

// Test edge cases for reward distribution
TEST_F(NeoTokenRewardsTest, TestRewardDistributionEdgeCases)
{
    // Test with zero fees
    {
        const int64_t zero_fee = 0;
        int64_t neo_holder_reward = zero_fee * NeoToken::NEO_HOLDER_REWARD_RATIO / 100;
        int64_t voter_reward = zero_fee * NeoToken::VOTER_REWARD_RATIO / 100;
        
        EXPECT_EQ(neo_holder_reward, 0);
        EXPECT_EQ(voter_reward, 0);
    }
    
    // Test with minimum fee (1 unit)
    {
        const int64_t min_fee = 1;
        int64_t neo_holder_reward = min_fee * NeoToken::NEO_HOLDER_REWARD_RATIO / 100;
        int64_t voter_reward = min_fee * NeoToken::VOTER_REWARD_RATIO / 100;
        
        // Due to integer division, these should be 0
        EXPECT_EQ(neo_holder_reward, 0);
        EXPECT_EQ(voter_reward, 0);
    }
    
    // Test with fee that divides evenly
    {
        const int64_t even_fee = 100;
        int64_t neo_holder_reward = even_fee * NeoToken::NEO_HOLDER_REWARD_RATIO / 100;
        int64_t voter_reward = even_fee * NeoToken::VOTER_REWARD_RATIO / 100;
        
        EXPECT_EQ(neo_holder_reward, 10);
        EXPECT_EQ(voter_reward, 80);
    }
    
    // Test with large fees
    {
        const int64_t large_fee = INT64_MAX / 100; // Avoid overflow
        int64_t neo_holder_reward = large_fee * NeoToken::NEO_HOLDER_REWARD_RATIO / 100;
        int64_t voter_reward = large_fee * NeoToken::VOTER_REWARD_RATIO / 100;
        
        EXPECT_GT(neo_holder_reward, 0);
        EXPECT_GT(voter_reward, 0);
        EXPECT_GT(voter_reward, neo_holder_reward); // Voter reward should be 8x larger
    }
}

// Test that reward distribution maintains precision
TEST_F(NeoTokenRewardsTest, TestRewardPrecision)
{
    // Test various amounts to ensure no precision loss
    std::vector<int64_t> test_amounts = {
        99,         // Just below 100
        101,        // Just above 100
        999,        // Three digits
        1001,       // Just above 1000
        12345,      // Random amount
        100000000   // 1 GAS (Fixed8.One equivalent)
    };
    
    for (int64_t amount : test_amounts) {
        int64_t neo_holder = amount * NeoToken::NEO_HOLDER_REWARD_RATIO / 100;
        int64_t voter = amount * NeoToken::VOTER_REWARD_RATIO / 100;
        int64_t committee = amount - neo_holder - voter;
        
        // Verify no negative rewards
        EXPECT_GE(neo_holder, 0);
        EXPECT_GE(voter, 0);
        EXPECT_GE(committee, 0);
        
        // Verify total doesn't exceed original
        EXPECT_LE(neo_holder + voter + committee, amount);
        
        // Verify ratios are approximately correct (within 1% due to integer division)
        if (amount >= 100) {
            double neo_ratio = static_cast<double>(neo_holder) / amount * 100;
            double voter_ratio = static_cast<double>(voter) / amount * 100;
            
            EXPECT_NEAR(neo_ratio, NeoToken::NEO_HOLDER_REWARD_RATIO, 1.0);
            EXPECT_NEAR(voter_ratio, NeoToken::VOTER_REWARD_RATIO, 1.0);
        }
    }
}

// Test that total supply is correctly initialized
TEST_F(NeoTokenRewardsTest, TestTotalSupply)
{
    // NEO total supply should be 100,000,000
    auto total_supply_result = neoToken->Call(*engine, "totalSupply", {});
    
    ASSERT_TRUE(total_supply_result->IsInteger());
    EXPECT_EQ(total_supply_result->GetInteger(), 100000000);
}

// Test reward distribution proportions
TEST_F(NeoTokenRewardsTest, TestRewardProportions)
{
    // The sum of all reward ratios should be <= 100%
    int total_ratio = NeoToken::NEO_HOLDER_REWARD_RATIO + NeoToken::VOTER_REWARD_RATIO;
    EXPECT_LE(total_ratio, 100);
    
    // Committee gets the remainder
    int committee_ratio = 100 - total_ratio;
    EXPECT_EQ(committee_ratio, 10); // 10% for committee
    
    // Verify the 10:80:10 split (NEO holders : Voters : Committee)
    EXPECT_EQ(NeoToken::NEO_HOLDER_REWARD_RATIO, 10);
    EXPECT_EQ(NeoToken::VOTER_REWARD_RATIO, 80);
    EXPECT_EQ(committee_ratio, 10);
}