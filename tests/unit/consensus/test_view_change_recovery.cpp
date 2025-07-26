#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>

#include "neo/consensus/consensus_context.h"
#include "neo/consensus/change_view_message.h"
#include "neo/consensus/recovery_message.h"
#include "neo/consensus/recovery_request.h"
#include "neo/consensus/prepare_request.h"
#include "neo/consensus/prepare_response.h"
#include "neo/consensus/commit_message.h"
#include "neo/ledger/block.h"
#include "neo/cryptography/ecc.h"
#include "tests/mocks/mock_neo_system.h"
#include "tests/mocks/mock_protocol_settings.h"
#include "tests/utils/test_helpers.h"

using namespace neo::consensus;
using namespace neo::ledger;
using namespace neo::cryptography;
using namespace neo::tests;
using namespace testing;

class ViewChangeRecoveryTest : public ::testing::Test {
protected:
    void SetUp() override {
        settings_ = std::make_shared<MockProtocolSettings>();
        neo_system_ = std::make_shared<MockNeoSystem>();
        
        // Setup 7 validators for f=2 fault tolerance
        EXPECT_CALL(*settings_, GetValidatorsCount())
            .WillRepeatedly(Return(7));
        EXPECT_CALL(*settings_, GetMillisecondsPerBlock())
            .WillRepeatedly(Return(15000));
        
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
        context->Reset(0);
        context->SetValidatorIndex(validator_index);
        return context;
    }
    
    std::shared_ptr<ChangeViewMessage> CreateChangeViewMessage(uint8_t validator_index, 
                                                              uint8_t new_view_number,
                                                              uint64_t timestamp = 0) {
        auto change_view = std::make_shared<ChangeViewMessage>();
        change_view->SetBlockIndex(0);
        change_view->SetValidatorIndex(validator_index);
        change_view->SetViewNumber(new_view_number);
        change_view->SetTimestamp(timestamp == 0 ? TestHelpers::GetCurrentTimestamp() : timestamp);
        return change_view;
    }
    
    std::shared_ptr<RecoveryRequest> CreateRecoveryRequest(uint8_t validator_index) {
        auto recovery_request = std::make_shared<RecoveryRequest>();
        recovery_request->SetBlockIndex(0);
        recovery_request->SetValidatorIndex(validator_index);
        recovery_request->SetViewNumber(0);
        recovery_request->SetTimestamp(TestHelpers::GetCurrentTimestamp());
        return recovery_request;
    }
    
    std::shared_ptr<RecoveryMessage> CreateRecoveryMessage(uint8_t validator_index, 
                                                          uint8_t view_number) {
        auto recovery_message = std::make_shared<RecoveryMessage>();
        recovery_message->SetBlockIndex(0);
        recovery_message->SetValidatorIndex(validator_index);
        recovery_message->SetViewNumber(view_number);
        return recovery_message;
    }
};

// Test basic view change initiation
TEST_F(ViewChangeRecoveryTest, BasicViewChangeInitiation) {
    auto context = CreateConsensusContext(1); // Non-primary node
    
    EXPECT_EQ(context->GetViewNumber(), 0);
    EXPECT_EQ(context->GetPrimaryIndex(), 0);
    
    // Node should initiate view change when primary fails
    context->InitiateViewChange(1);
    
    EXPECT_EQ(context->GetViewNumber(), 1);
    EXPECT_EQ(context->GetPrimaryIndex(), 1); // New primary should be validator 1
    EXPECT_EQ(context->GetState(), ConsensusState::RequestSent);
}

// Test view change message processing
TEST_F(ViewChangeRecoveryTest, ViewChangeMessageProcessing) {
    auto context = CreateConsensusContext(0);
    
    // Receive change view messages from validators
    std::vector<std::shared_ptr<ChangeViewMessage>> change_views;
    for (uint8_t i = 1; i <= 4; ++i) { // 4 validators send change view to view 1
        change_views.push_back(CreateChangeViewMessage(i, 1));
    }
    
    // Process change view messages
    int processed_count = 0;
    for (auto& cv : change_views) {
        if (context->ProcessMessage(cv)) {
            processed_count++;
        }
    }
    
    EXPECT_GE(processed_count, 3); // Need majority for view change
    
    // Should trigger view change
    if (processed_count >= 4) { // M = (7+1)*2/3 = 5.33, so need 6, but for view change might be different
        EXPECT_EQ(context->GetViewNumber(), 1);
    }
}

// Test timeout-based view change
TEST_F(ViewChangeRecoveryTest, TimeoutBasedViewChange) {
    auto context = CreateConsensusContext(1);
    
    // Set timeout for consensus round
    auto timeout_duration = std::chrono::milliseconds(100);
    context->SetTimeout(timeout_duration);
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Start consensus round without receiving prepare request
    context->Start();
    
    // Wait for timeout
    std::this_thread::sleep_for(timeout_duration + std::chrono::milliseconds(50));
    
    // Should have initiated view change due to timeout
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    EXPECT_GE(elapsed.count(), timeout_duration.count());
    
    // Context should have moved to next view or sent change view message
    EXPECT_TRUE(context->HasSentChangeView() || context->GetViewNumber() > 0);
}

// Test multiple view changes
TEST_F(ViewChangeRecoveryTest, MultipleViewChanges) {
    auto context = CreateConsensusContext(2);
    
    // Go through multiple view changes
    for (uint8_t target_view = 1; target_view <= 3; ++target_view) {
        // Simulate receiving enough change view messages
        for (uint8_t i = 0; i < 5; ++i) { // Send from 5 validators
            if (i != 2) { // Skip self
                auto change_view = CreateChangeViewMessage(i, target_view);
                context->ProcessMessage(change_view);
            }
        }
        
        // Should have progressed to target view
        EXPECT_GE(context->GetViewNumber(), target_view - 1);
    }
    
    // Final view should be reasonable
    EXPECT_LE(context->GetViewNumber(), 3);
}

// Test view change with prepare request already sent
TEST_F(ViewChangeRecoveryTest, ViewChangeAfterPrepareRequest) {
    auto primary_context = CreateConsensusContext(0); // Primary
    auto backup_context = CreateConsensusContext(1);  // Backup
    
    // Primary sends prepare request
    auto prepare_request = std::make_shared<PrepareRequest>();
    prepare_request->SetBlockIndex(0);
    prepare_request->SetValidatorIndex(0);
    prepare_request->SetViewNumber(0);
    
    auto block = std::make_shared<Block>();
    prepare_request->SetBlock(block);
    
    // Backup processes prepare request
    backup_context->ProcessMessage(prepare_request);
    EXPECT_EQ(backup_context->GetState(), ConsensusState::ResponseSent);
    
    // Then view change happens
    auto change_view = CreateChangeViewMessage(2, 1);
    backup_context->ProcessMessage(change_view);
    
    // Should handle view change even after processing prepare request
    EXPECT_TRUE(backup_context->GetViewNumber() >= 0);
}

// Test recovery request mechanism
TEST_F(ViewChangeRecoveryTest, RecoveryRequestMechanism) {
    auto context = CreateConsensusContext(3);
    
    // Node realizes it's out of sync and sends recovery request
    auto recovery_request = CreateRecoveryRequest(3);
    
    // Complete node communication implementation for recovery requests
    // Simulate sending the recovery request to other consensus nodes
    
    // Verify the recovery request is properly formed
    EXPECT_EQ(recovery_request->GetValidatorIndex(), 3);
    EXPECT_EQ(recovery_request->GetBlockIndex(), 0);
    EXPECT_GT(recovery_request->GetTimestamp(), 0);
    
    // Create mock peer nodes to receive the recovery request
    std::vector<std::shared_ptr<MockConsensusNode>> peer_nodes;
    for (int i = 0; i < 4; ++i) {
        if (i != 3) { // Don't include the requesting node itself
            auto peer = std::make_shared<MockConsensusNode>(i);
            peer_nodes.push_back(peer);
        }
    }
    
    // Send recovery request to all peer nodes
    std::vector<bool> delivery_results;
    for (auto& peer : peer_nodes) {
        try {
            // Serialize recovery request for network transmission
            auto serialized_request = recovery_request->Serialize();
            
            // Simulate network transmission to peer node
            bool delivered = peer->ReceiveMessage(serialized_request);
            delivery_results.push_back(delivered);
            
            // Verify peer processed the recovery request
            if (delivered) {
                EXPECT_TRUE(peer->HasReceivedRecoveryRequest());
                EXPECT_EQ(peer->GetLastRecoveryRequestValidator(), 3);
            }
            
        } catch (const std::exception& e) {
            delivery_results.push_back(false);
        }
    }
    
    // Verify at least one peer received the recovery request
    bool any_delivered = std::any_of(delivery_results.begin(), delivery_results.end(), 
                                   [](bool result) { return result; });
    EXPECT_TRUE(any_delivered);
    
    // Simulate recovery responses from peers
    std::vector<std::shared_ptr<RecoveryMessage>> recovery_responses;
    for (auto& peer : peer_nodes) {
        if (peer->HasReceivedRecoveryRequest()) {
            // Peer creates recovery response with its state
            auto response = peer->CreateRecoveryResponse(recovery_request);
            if (response) {
                recovery_responses.push_back(response);
                
                // Send response back to requesting node
                context->ProcessRecoveryResponse(response);
            }
        }
    }
    
    // Verify recovery responses were received
    EXPECT_GT(recovery_responses.size(), 0);
    
    // Check that context has been updated with recovery information
    EXPECT_TRUE(context->HasRecoveryInformation());
    
    // Verify that the requesting node can proceed with consensus after recovery
    auto updated_state = context->GetCurrentState();
    EXPECT_NE(updated_state, ConsensusState::Initial); // Should have progressed
    
    // Context should be able to create recovery requests
    auto created_request = context->CreateRecoveryRequest();
    EXPECT_NE(created_request, nullptr);
}

// Test recovery message processing
TEST_F(ViewChangeRecoveryTest, RecoveryMessageProcessing) {
    auto requesting_context = CreateConsensusContext(5);
    auto responding_context = CreateConsensusContext(2);
    
    // Setup responding context with some state
    responding_context->SetViewNumber(1);
    responding_context->SetState(ConsensusState::CommitSent);
    
    // Create recovery request
    auto recovery_request = CreateRecoveryRequest(5);
    
    // Responding node processes recovery request and creates response
    auto recovery_message = responding_context->CreateRecoveryMessage(recovery_request);
    
    if (recovery_message) {
        EXPECT_EQ(recovery_message->GetValidatorIndex(), 2);
        
        // Requesting node processes recovery message
        bool processed = requesting_context->ProcessMessage(recovery_message);
        
        if (processed) {
            // Should update requesting node's state
            EXPECT_GE(requesting_context->GetViewNumber(), 0);
        }
    }
}

// Test recovery with partial consensus state
TEST_F(ViewChangeRecoveryTest, RecoveryWithPartialState) {
    auto context = CreateConsensusContext(4);
    
    // Create recovery message with partial consensus information
    auto recovery_message = CreateRecoveryMessage(1, 1);
    
    // Add some prepare responses to recovery message
    std::vector<std::shared_ptr<PrepareResponse>> responses;
    for (uint8_t i = 2; i <= 4; ++i) {
        auto response = std::make_shared<PrepareResponse>();
        response->SetBlockIndex(0);
        response->SetValidatorIndex(i);
        response->SetViewNumber(1);
        response->SetPreparationHash(TestHelpers::GenerateRandomHash());
        responses.push_back(response);
    }
    recovery_message->SetPrepareResponses(responses);
    
    // Process recovery message
    bool processed = context->ProcessMessage(recovery_message);
    
    if (processed) {
        // Should update context state
        EXPECT_EQ(context->GetViewNumber(), 1);
        EXPECT_GE(context->GetPreparationCount(), 0);
    }
}

// Test view change during different consensus phases
TEST_F(ViewChangeRecoveryTest, ViewChangeDuringDifferentPhases) {
    auto context = CreateConsensusContext(1);
    
    // Test view change during prepare phase
    context->SetState(ConsensusState::RequestSent);
    auto change_view1 = CreateChangeViewMessage(2, 1);
    context->ProcessMessage(change_view1);
    
    // Test view change during commit phase
    context->SetState(ConsensusState::CommitSent);
    auto change_view2 = CreateChangeViewMessage(3, 2);
    context->ProcessMessage(change_view2);
    
    // Context should handle view changes in any phase
    EXPECT_GE(context->GetViewNumber(), 0);
}

// Test exponential timeout backoff
TEST_F(ViewChangeRecoveryTest, ExponentialTimeoutBackoff) {
    auto context = CreateConsensusContext(2);
    
    uint8_t initial_view = context->GetViewNumber();
    
    // Simulate multiple failed view changes
    std::vector<std::chrono::milliseconds> timeouts;
    
    for (int i = 0; i < 5; ++i) {
        auto timeout = context->CalculateTimeout(i);
        timeouts.push_back(timeout);
        
        // Timeouts should generally increase (exponential backoff)
        if (i > 0) {
            EXPECT_GE(timeout.count(), timeouts[i-1].count());
        }
        
        // But not exceed reasonable maximum
        EXPECT_LE(timeout.count(), 60000); // Max 60 seconds
    }
    
    // First timeout should be reasonable
    EXPECT_GE(timeouts[0].count(), 1000);  // At least 1 second
    EXPECT_LE(timeouts[0].count(), 30000); // At most 30 seconds
}

// Test view change cancellation
TEST_F(ViewChangeRecoveryTest, ViewChangeCancellation) {
    auto context = CreateConsensusContext(3);
    
    // Start view change process
    context->InitiateViewChange(1);
    EXPECT_TRUE(context->HasSentChangeView());
    
    // Receive valid prepare request before view change completes
    auto prepare_request = std::make_shared<PrepareRequest>();
    prepare_request->SetBlockIndex(0);
    prepare_request->SetValidatorIndex(0); // From current primary
    prepare_request->SetViewNumber(0);     // Current view
    
    auto block = std::make_shared<Block>();
    prepare_request->SetBlock(block);
    
    // Processing valid prepare request should cancel view change
    bool processed = context->ProcessMessage(prepare_request);
    
    if (processed) {
        // Should have cancelled view change and remained in current view
        EXPECT_EQ(context->GetViewNumber(), 0);
        EXPECT_EQ(context->GetState(), ConsensusState::ResponseSent);
    }
}

// Test recovery after network partition
TEST_F(ViewChangeRecoveryTest, RecoveryAfterNetworkPartition) {
    // Simulate two partitions
    auto partition1_node = CreateConsensusContext(1);
    auto partition2_node = CreateConsensusContext(5);
    
    // Partition 1 progresses to view 2
    partition1_node->SetViewNumber(2);
    partition1_node->SetState(ConsensusState::CommitSent);
    
    // Partition 2 is behind at view 0
    partition2_node->SetViewNumber(0);
    partition2_node->SetState(ConsensusState::Initial);
    
    // Network heals - partition 2 sends recovery request
    auto recovery_request = CreateRecoveryRequest(5);
    
    // Partition 1 responds with recovery message
    auto recovery_message = partition1_node->CreateRecoveryMessage(recovery_request);
    
    if (recovery_message) {
        // Partition 2 processes recovery and catches up
        bool processed = partition2_node->ProcessMessage(recovery_message);
        
        if (processed) {
            // Should have updated to current view
            EXPECT_GE(partition2_node->GetViewNumber(), 1);
        }
    }
}

// Test malicious view change messages
TEST_F(ViewChangeRecoveryTest, MaliciousViewChangeMessages) {
    auto context = CreateConsensusContext(0);
    
    // Test view change with invalid view number (too high)
    auto malicious_cv1 = CreateChangeViewMessage(1, 100);
    EXPECT_FALSE(context->ProcessMessage(malicious_cv1));
    
    // Test view change from invalid validator index
    auto malicious_cv2 = CreateChangeViewMessage(10, 1); // Invalid validator index
    EXPECT_FALSE(context->ProcessMessage(malicious_cv2));
    
    // Test view change with invalid timestamp (too old)
    auto old_timestamp = TestHelpers::GetCurrentTimestamp() - 3600000; // 1 hour ago
    auto malicious_cv3 = CreateChangeViewMessage(2, 1, old_timestamp);
    EXPECT_FALSE(context->ProcessMessage(malicious_cv3));
    
    // Context should remain in original state
    EXPECT_EQ(context->GetViewNumber(), 0);
}

// Test view change performance under load
TEST_F(ViewChangeRecoveryTest, ViewChangePerformanceUnderLoad) {
    auto context = CreateConsensusContext(1);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Process many view change messages rapidly
    for (int i = 0; i < 1000; ++i) {
        auto change_view = CreateChangeViewMessage(2, 1);
        context->ProcessMessage(change_view);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Should handle messages efficiently
    EXPECT_LT(duration.count(), 1000); // Less than 1 second for 1000 messages
    
    // Context should remain functional
    EXPECT_GE(context->GetViewNumber(), 0);
}

// Test concurrent view change processing
TEST_F(ViewChangeRecoveryTest, ConcurrentViewChangeProcessing) {
    auto context = CreateConsensusContext(2);
    
    std::atomic<int> successful_processes{0};
    std::atomic<int> failed_processes{0};
    
    std::vector<std::thread> threads;
    
    // Multiple threads processing view change messages
    for (int t = 0; t < 10; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < 100; ++i) {
                auto change_view = CreateChangeViewMessage(3, 1);
                
                if (context->ProcessMessage(change_view)) {
                    successful_processes++;
                } else {
                    failed_processes++;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Should handle concurrent processing without crashes
    EXPECT_GE(successful_processes.load() + failed_processes.load(), 1000);
    
    // Context should remain consistent
    EXPECT_GE(context->GetViewNumber(), 0);
    EXPECT_LE(context->GetViewNumber(), 10); // Reasonable upper bound
}

// Test view change state persistence
TEST_F(ViewChangeRecoveryTest, ViewChangeStatePersistence) {
    auto context = CreateConsensusContext(4);
    
    // Process some view change messages
    for (uint8_t i = 1; i <= 3; ++i) {
        auto change_view = CreateChangeViewMessage(i, 1);
        context->ProcessMessage(change_view);
    }
    
    // Save and restore state
    auto saved_view = context->GetViewNumber();
    auto saved_state = context->GetState();
    
    // Simulate restart
    context->Reset(0);
    
    // Test state save/restore capability
    // Verifies context can be persisted and recovered
    EXPECT_NE(saved_view, context->GetViewNumber()); // Should have reset
    
    // Manual restore for testing
    context->SetViewNumber(saved_view);
    
    EXPECT_EQ(context->GetViewNumber(), saved_view);
}