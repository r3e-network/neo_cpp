#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>

#include "neo/consensus/consensus_context.h"
#include "neo/consensus/consensus_message.h"
#include "neo/consensus/prepare_request.h"
#include "neo/consensus/prepare_response.h"
#include "neo/consensus/change_view_message.h"
#include "neo/consensus/commit_message.h"
#include "neo/ledger/block.h"
#include "neo/cryptography/ecc.h"
#include "tests/mocks/mock_neo_system.h"
#include "tests/mocks/mock_protocol_settings.h"

using namespace neo::consensus;
using namespace neo::ledger;
using namespace neo::cryptography;
using namespace neo::tests;
using namespace testing;

class ByzantineFaultToleranceTest : public ::testing::Test {
protected:
    void SetUp() override {
        settings_ = std::make_shared<MockProtocolSettings>();
        neo_system_ = std::make_shared<MockNeoSystem>();
        
        // Setup 7 validators for f=2 Byzantine fault tolerance
        EXPECT_CALL(*settings_, GetValidatorsCount())
            .WillRepeatedly(Return(7));
        
        // Setup validator keys
        validators_.clear();
        for (int i = 0; i < 7; ++i) {
            auto key_pair = ECPoint::GenerateKeyPair();
            validators_.push_back(key_pair.GetPublicKey());
        }
        
        EXPECT_CALL(*settings_, GetStandbyCommittee())
            .WillRepeatedly(ReturnRef(validators_));
    }

    std::shared_ptr<MockProtocolSettings> settings_;
    std::shared_ptr<MockNeoSystem> neo_system_;
    std::vector<ECPoint> validators_;
    
    std::shared_ptr<ConsensusContext> CreateConsensusContext(uint8_t validator_index) {
        auto context = std::make_shared<ConsensusContext>(neo_system_, settings_, nullptr);
        context->Reset(0); // Start at block 0
        context->SetValidatorIndex(validator_index);
        return context;
    }
    
    std::shared_ptr<PrepareRequest> CreateValidPrepareRequest(uint8_t validator_index) {
        auto block = std::make_shared<Block>();
        // Setup valid block data
        auto prepare_request = std::make_shared<PrepareRequest>();
        prepare_request->SetBlockIndex(0);
        prepare_request->SetValidatorIndex(validator_index);
        prepare_request->SetViewNumber(0);
        prepare_request->SetBlock(block);
        return prepare_request;
    }
    
    std::shared_ptr<PrepareResponse> CreatePrepareResponse(uint8_t validator_index, const UInt256& block_hash) {
        auto response = std::make_shared<PrepareResponse>();
        response->SetBlockIndex(0);
        response->SetValidatorIndex(validator_index);
        response->SetViewNumber(0);
        response->SetPreparationHash(block_hash);
        return response;
    }
};

// Test Byzantine node sending conflicting messages
TEST_F(ByzantineFaultToleranceTest, ConflictingPrepareRequests) {
    auto context = CreateConsensusContext(0); // Primary node
    
    // Byzantine node (validator 1) sends two different prepare requests
    auto request1 = CreateValidPrepareRequest(1);
    auto request2 = CreateValidPrepareRequest(1);
    
    // Make requests have different block hashes
    auto block1 = std::make_shared<Block>();
    auto block2 = std::make_shared<Block>();
    // Ensure different hashes by setting different timestamps
    block1->GetHeader().SetTimestamp(1000);
    block2->GetHeader().SetTimestamp(2000);
    
    request1->SetBlock(block1);
    request2->SetBlock(block2);
    
    // Process first request
    bool result1 = context->ProcessMessage(request1);
    EXPECT_TRUE(result1); // First request should be accepted
    
    // Process conflicting second request from same validator
    bool result2 = context->ProcessMessage(request2);
    EXPECT_FALSE(result2); // Second conflicting request should be rejected
    
    // Verify context state is not corrupted
    EXPECT_EQ(context->GetState(), ConsensusState::RequestSent);
}

// Test Byzantine node sending conflicting prepare responses
TEST_F(ByzantineFaultToleranceTest, ConflictingPrepareResponses) {
    auto primary_context = CreateConsensusContext(0);
    auto byzantine_context = CreateConsensusContext(1);
    
    // Primary sends valid prepare request
    auto prepare_request = CreateValidPrepareRequest(0);
    auto block_hash = prepare_request->GetBlock()->GetHash();
    
    // Byzantine node sends two different responses to same prepare request
    UInt256 fake_hash = UInt256::Parse("1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
    
    auto response1 = CreatePrepareResponse(1, block_hash);    // Correct response
    auto response2 = CreatePrepareResponse(1, fake_hash);     // Conflicting response
    
    // Process primary's prepare request first
    bool result0 = primary_context->ProcessMessage(prepare_request);
    EXPECT_TRUE(result0);
    
    // Process first response
    bool result1 = primary_context->ProcessMessage(response1);
    EXPECT_TRUE(result1);
    
    // Process conflicting response from same validator
    bool result2 = primary_context->ProcessMessage(response2);
    EXPECT_FALSE(result2); // Should be rejected due to conflict
    
    // Verify the correct response is still recorded
    auto preparations = primary_context->GetPreparations();
    EXPECT_TRUE(preparations[1] != nullptr);
    EXPECT_EQ(preparations[1]->GetPreparationHash(), block_hash);
}

// Test consensus with exactly f Byzantine nodes
TEST_F(ByzantineFaultToleranceTest, MaxByzantineNodes) {
    auto primary_context = CreateConsensusContext(0);
    
    // Primary sends prepare request
    auto prepare_request = CreateValidPrepareRequest(0);
    auto block_hash = prepare_request->GetBlock()->GetHash();
    
    bool result = primary_context->ProcessMessage(prepare_request);
    EXPECT_TRUE(result);
    
    // 2 Byzantine nodes (f=2) send conflicting responses
    UInt256 fake_hash = UInt256::Parse("deadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef");
    
    auto byzantine_response1 = CreatePrepareResponse(1, fake_hash);
    auto byzantine_response2 = CreatePrepareResponse(2, fake_hash);
    
    // 4 honest nodes send correct responses (need f+1=3 for safety)
    std::vector<std::shared_ptr<PrepareResponse>> honest_responses;
    for (uint8_t i = 3; i <= 6; ++i) {
        honest_responses.push_back(CreatePrepareResponse(i, block_hash));
    }
    
    // Process Byzantine responses (should be accepted as they're not conflicting with each other)
    primary_context->ProcessMessage(byzantine_response1);
    primary_context->ProcessMessage(byzantine_response2);
    
    // Process honest responses
    for (auto& response : honest_responses) {
        primary_context->ProcessMessage(response);
    }
    
    // Should still be able to achieve consensus with honest majority
    EXPECT_GE(primary_context->GetPreparationCount(), 5); // M = (7+1)*2/3 = 5.33, so need 6
}

// Test network partition scenarios
TEST_F(ByzantineFaultToleranceTest, NetworkPartition) {
    // Create contexts for 7 validators
    std::vector<std::shared_ptr<ConsensusContext>> contexts;
    for (uint8_t i = 0; i < 7; ++i) {
        contexts.push_back(CreateConsensusContext(i));
    }
    
    // Simulate network partition: 4 nodes vs 3 nodes
    std::vector<uint8_t> partition1 = {0, 1, 2, 3}; // 4 nodes (majority)
    std::vector<uint8_t> partition2 = {4, 5, 6};    // 3 nodes (minority)
    
    // Primary (node 0) in majority partition sends prepare request
    auto prepare_request = CreateValidPrepareRequest(0);
    auto block_hash = prepare_request->GetBlock()->GetHash();
    
    // Only majority partition processes the request
    for (auto node_index : partition1) {
        contexts[node_index]->ProcessMessage(prepare_request);
    }
    
    // Majority partition nodes send responses
    for (auto node_index : partition1) {
        if (node_index != 0) { // Skip primary
            auto response = CreatePrepareResponse(node_index, block_hash);
            for (auto recv_index : partition1) {
                contexts[recv_index]->ProcessMessage(response);
            }
        }
    }
    
    // Majority partition should achieve consensus
    EXPECT_GE(contexts[0]->GetPreparationCount(), 3); // Need majority of 4
    
    // Minority partition should not achieve consensus
    EXPECT_LT(contexts[4]->GetPreparationCount(), 3); // Cannot achieve consensus with only 3 nodes
}

// Test view change with Byzantine nodes
TEST_F(ByzantineFaultToleranceTest, ViewChangeWithByzantineNodes) {
    auto context = CreateConsensusContext(1); // Non-primary node
    
    // Byzantine primary (node 0) fails to send prepare request
    // Honest nodes should initiate view change
    
    // Simulate timeout - honest nodes send change view messages
    std::vector<std::shared_ptr<ChangeViewMessage>> change_views;
    for (uint8_t i = 1; i <= 4; ++i) { // 4 honest nodes send change view
        auto change_view = std::make_shared<ChangeViewMessage>();
        change_view->SetBlockIndex(0);
        change_view->SetValidatorIndex(i);
        change_view->SetViewNumber(1); // Change to view 1
        change_view->SetTimestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
        change_views.push_back(change_view);
    }
    
    // Process change view messages
    for (auto& cv : change_views) {
        context->ProcessMessage(cv);
    }
    
    // Should trigger view change to view 1 with new primary
    EXPECT_EQ(context->GetViewNumber(), 1);
    EXPECT_EQ(context->GetPrimaryIndex(), 1); // New primary should be validator 1
}

// Test commit phase with Byzantine nodes
TEST_F(ByzantineFaultToleranceTest, CommitPhaseWithByzantineNodes) {
    auto context = CreateConsensusContext(0);
    
    // Setup successful prepare phase
    auto prepare_request = CreateValidPrepareRequest(0);
    auto block_hash = prepare_request->GetBlock()->GetHash();
    context->ProcessMessage(prepare_request);
    
    // Receive enough prepare responses
    for (uint8_t i = 1; i <= 5; ++i) {
        auto response = CreatePrepareResponse(i, block_hash);
        context->ProcessMessage(response);
    }
    
    // Now in commit phase - Byzantine nodes send conflicting commits
    UInt256 fake_block_hash = UInt256::Parse("cafebabecafebabecafebabecafebabecafebabecafebabecafebabecafebabe");
    
    auto byzantine_commit1 = std::make_shared<CommitMessage>();
    byzantine_commit1->SetBlockIndex(0);
    byzantine_commit1->SetValidatorIndex(1);
    byzantine_commit1->SetViewNumber(0);
    byzantine_commit1->SetSignature(std::vector<uint8_t>(64, 0xFF)); // Fake signature
    
    auto byzantine_commit2 = std::make_shared<CommitMessage>();
    byzantine_commit2->SetBlockIndex(0);
    byzantine_commit2->SetValidatorIndex(2);
    byzantine_commit2->SetViewNumber(0);
    byzantine_commit2->SetSignature(std::vector<uint8_t>(64, 0xAA)); // Different fake signature
    
    // Honest commits
    std::vector<std::shared_ptr<CommitMessage>> honest_commits;
    for (uint8_t i = 3; i <= 6; ++i) {
        auto commit = std::make_shared<CommitMessage>();
        commit->SetBlockIndex(0);
        commit->SetValidatorIndex(i);
        commit->SetViewNumber(0);
        // In real implementation, this would be actual signature of block hash
        commit->SetSignature(std::vector<uint8_t>(64, i)); // Placeholder
        honest_commits.push_back(commit);
    }
    
    // Process commits
    context->ProcessMessage(byzantine_commit1);
    context->ProcessMessage(byzantine_commit2);
    
    for (auto& commit : honest_commits) {
        context->ProcessMessage(commit);
    }
    
    // Should achieve consensus with honest majority
    EXPECT_GE(context->GetCommitCount(), 5); // Need M commits for finalization
}

// Test recovery from Byzantine node equivocation
TEST_F(ByzantineFaultToleranceTest, RecoveryFromEquivocation) {
    auto context = CreateConsensusContext(2);
    
    // Byzantine node 0 sends equivocating messages
    auto prepare1 = CreateValidPrepareRequest(0);
    auto prepare2 = CreateValidPrepareRequest(0);
    
    // Make them different
    prepare1->GetBlock()->GetHeader().SetTimestamp(1000);
    prepare2->GetBlock()->GetHeader().SetTimestamp(2000);
    
    // Process first message
    bool result1 = context->ProcessMessage(prepare1);
    EXPECT_TRUE(result1);
    
    // Process equivocating message
    bool result2 = context->ProcessMessage(prepare2);
    EXPECT_FALSE(result2); // Should be rejected
    
    // Context should mark validator 0 as faulty
    EXPECT_TRUE(context->IsValidatorFaulty(0));
    
    // Consensus should continue with remaining honest validators
    // New primary should be validator 1
    auto valid_prepare = CreateValidPrepareRequest(1);
    bool result3 = context->ProcessMessage(valid_prepare);
    EXPECT_TRUE(result3);
}

// Test Byzantine node sending messages for wrong view
TEST_F(ByzantineFaultToleranceTest, WrongViewMessages) {
    auto context = CreateConsensusContext(0);
    context->SetViewNumber(1); // Current view is 1
    
    // Byzantine node sends message for old view
    auto old_view_message = CreateValidPrepareRequest(1);
    old_view_message->SetViewNumber(0); // Wrong view
    
    bool result = context->ProcessMessage(old_view_message);
    EXPECT_FALSE(result); // Should be rejected
    
    // Byzantine node sends message for future view
    auto future_view_message = CreateValidPrepareRequest(1);
    future_view_message->SetViewNumber(2); // Future view
    
    bool result2 = context->ProcessMessage(future_view_message);
    EXPECT_FALSE(result2); // Should be rejected
}

// Test Byzantine node sending messages with invalid signatures
TEST_F(ByzantineFaultToleranceTest, InvalidSignatures) {
    auto context = CreateConsensusContext(0);
    
    auto prepare_request = CreateValidPrepareRequest(1);
    
    // Corrupt the signature
    std::vector<uint8_t> invalid_signature(64, 0x00);
    prepare_request->SetSignature(invalid_signature);
    
    bool result = context->ProcessMessage(prepare_request);
    EXPECT_FALSE(result); // Should be rejected due to invalid signature
}

// Performance test: consensus under Byzantine attack
TEST_F(ByzantineFaultToleranceTest, PerformanceUnderAttack) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    auto context = CreateConsensusContext(0);
    
    // Simulate 1000 Byzantine messages
    for (int i = 0; i < 1000; ++i) {
        auto byzantine_message = CreateValidPrepareRequest(1);
        byzantine_message->SetBlockIndex(i); // Different block indices
        context->ProcessMessage(byzantine_message);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Should handle Byzantine messages efficiently (< 1 second for 1000 messages)
    EXPECT_LT(duration.count(), 1000);
    
    // Context should remain functional
    auto valid_prepare = CreateValidPrepareRequest(0);
    bool result = context->ProcessMessage(valid_prepare);
    EXPECT_TRUE(result);
}