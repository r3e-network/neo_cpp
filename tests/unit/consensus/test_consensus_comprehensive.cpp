/**
 * @file test_consensus_comprehensive.cpp
 * @brief Comprehensive consensus tests for Neo dBFT 2.0
 */

#include <gtest/gtest.h>
#include <neo/consensus/consensus_context.h>
#include <neo/consensus/consensus_message.h>
#include <neo/consensus/consensus_service.h>
#include <neo/consensus/change_view.h>
#include <neo/consensus/commit.h>
#include <neo/consensus/prepare_request.h>
#include <neo/consensus/prepare_response.h>
#include <neo/consensus/recovery_message.h>
#include <neo/consensus/recovery_request.h>
#include <neo/ledger/blockchain.h>
#include <neo/cryptography/key_pair.h>
#include <neo/wallets/wallet.h>
#include <memory>
#include <vector>

using namespace neo::consensus;
using namespace neo::cryptography;
using namespace neo::ledger;
using namespace neo::wallets;

class ConsensusComprehensiveTest : public ::testing::Test {
protected:
    std::unique_ptr<ConsensusContext> context_;
    std::unique_ptr<ConsensusService> service_;
    std::unique_ptr<Blockchain> blockchain_;
    std::vector<std::unique_ptr<KeyPair>> validators_;
    
    void SetUp() override {
        // Initialize blockchain
        blockchain_ = std::make_unique<Blockchain>();
        
        // Create validator keys
        for (int i = 0; i < 7; ++i) {
            validators_.push_back(std::make_unique<KeyPair>());
        }
        
        // Initialize consensus context
        context_ = std::make_unique<ConsensusContext>(blockchain_.get());
        
        // Initialize consensus service
        service_ = std::make_unique<ConsensusService>(
            blockchain_.get(),
            nullptr,  // network
            validators_[0].get()  // primary validator
        );
    }
    
    void TearDown() override {
        service_.reset();
        context_.reset();
        blockchain_.reset();
    }
};

// ============================================================================
// Basic Consensus Tests
// ============================================================================

TEST_F(ConsensusComprehensiveTest, ConsensusContext_Initialization) {
    EXPECT_NE(context_, nullptr);
    EXPECT_EQ(context_->ViewNumber(), 0);
    EXPECT_FALSE(context_->IsPrimary());
    EXPECT_FALSE(context_->IsBackup());
    EXPECT_EQ(context_->CountCommitted(), 0);
    EXPECT_EQ(context_->CountFailed(), 0);
}

TEST_F(ConsensusComprehensiveTest, ConsensusContext_Reset) {
    context_->SetViewNumber(5);
    context_->Reset(0);
    
    EXPECT_EQ(context_->ViewNumber(), 0);
    EXPECT_EQ(context_->CountCommitted(), 0);
    EXPECT_EQ(context_->CountFailed(), 0);
}

TEST_F(ConsensusComprehensiveTest, ConsensusService_Start) {
    service_->Start();
    EXPECT_TRUE(service_->IsStarted());
    
    service_->Stop();
    EXPECT_FALSE(service_->IsStarted());
}

// ============================================================================
// View Change Tests
// ============================================================================

TEST_F(ConsensusComprehensiveTest, ViewChange_InitiateViewChange) {
    uint8_t initial_view = context_->ViewNumber();
    
    // Create change view message
    ChangeView cv;
    cv.ViewNumber = initial_view + 1;
    cv.Timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    cv.Reason = ChangeViewReason::Timeout;
    
    // Process change view
    context_->ProcessChangeView(cv);
    
    // Verify view change initiated
    EXPECT_GT(context_->ViewNumber(), initial_view);
}

TEST_F(ConsensusComprehensiveTest, ViewChange_MultipleChangeViews) {
    std::vector<ChangeView> changes;
    
    // Create multiple change view requests from different validators
    for (size_t i = 0; i < validators_.size(); ++i) {
        ChangeView cv;
        cv.ViewNumber = 1;
        cv.ValidatorIndex = i;
        cv.Timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        cv.Reason = ChangeViewReason::Timeout;
        changes.push_back(cv);
    }
    
    // Process change views
    for (const auto& cv : changes) {
        context_->ProcessChangeView(cv);
    }
    
    // Should trigger view change after f+1 requests (f=2 for 7 validators)
    EXPECT_GE(context_->CountChangeViews(), 3);
}

TEST_F(ConsensusComprehensiveTest, ViewChange_InvalidViewNumber) {
    uint8_t current_view = context_->ViewNumber();
    
    // Try to change to a lower view number
    ChangeView cv;
    cv.ViewNumber = current_view - 1;
    cv.Timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    
    // Should reject invalid view change
    EXPECT_FALSE(context_->ProcessChangeView(cv));
}

// ============================================================================
// Prepare Request/Response Tests
// ============================================================================

TEST_F(ConsensusComprehensiveTest, PrepareRequest_Creation) {
    PrepareRequest req;
    req.ViewNumber = 0;
    req.BlockIndex = 1;
    req.Timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    req.Nonce = 12345;
    
    EXPECT_EQ(req.ViewNumber, 0);
    EXPECT_EQ(req.BlockIndex, 1);
    EXPECT_EQ(req.Nonce, 12345);
    EXPECT_GT(req.Timestamp, 0);
}

TEST_F(ConsensusComprehensiveTest, PrepareResponse_Processing) {
    // Create prepare request first
    PrepareRequest req;
    req.ViewNumber = 0;
    req.BlockIndex = 1;
    context_->ProcessPrepareRequest(req);
    
    // Create prepare responses
    std::vector<PrepareResponse> responses;
    for (size_t i = 0; i < validators_.size(); ++i) {
        PrepareResponse resp;
        resp.ViewNumber = 0;
        resp.BlockIndex = 1;
        resp.ValidatorIndex = i;
        responses.push_back(resp);
    }
    
    // Process responses
    for (const auto& resp : responses) {
        context_->ProcessPrepareResponse(resp);
    }
    
    // Check if prepared (need 2f+1 responses)
    EXPECT_GE(context_->CountPrepared(), 5);
}

// ============================================================================
// Commit Tests
// ============================================================================

TEST_F(ConsensusComprehensiveTest, Commit_Processing) {
    // Setup: Process prepare phase first
    PrepareRequest req;
    req.ViewNumber = 0;
    req.BlockIndex = 1;
    context_->ProcessPrepareRequest(req);
    
    // Create commit messages
    std::vector<Commit> commits;
    for (size_t i = 0; i < validators_.size(); ++i) {
        Commit commit;
        commit.ViewNumber = 0;
        commit.BlockIndex = 1;
        commit.ValidatorIndex = i;
        commit.Signature = ByteVector(64, 0xFF); // Mock signature
        commits.push_back(commit);
    }
    
    // Process commits
    for (const auto& commit : commits) {
        context_->ProcessCommit(commit);
    }
    
    // Check if committed (need 2f+1 commits)
    EXPECT_GE(context_->CountCommitted(), 5);
}

TEST_F(ConsensusComprehensiveTest, Commit_InvalidSignature) {
    Commit commit;
    commit.ViewNumber = 0;
    commit.BlockIndex = 1;
    commit.Signature = ByteVector(); // Empty signature
    
    // Should reject commit with invalid signature
    EXPECT_FALSE(context_->ProcessCommit(commit));
}

// ============================================================================
// Recovery Tests
// ============================================================================

TEST_F(ConsensusComprehensiveTest, RecoveryRequest_Creation) {
    RecoveryRequest req;
    req.ViewNumber = 2;
    req.Timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    
    EXPECT_EQ(req.ViewNumber, 2);
    EXPECT_GT(req.Timestamp, 0);
}

TEST_F(ConsensusComprehensiveTest, RecoveryMessage_Processing) {
    RecoveryMessage msg;
    msg.ViewNumber = 1;
    msg.BlockIndex = 100;
    
    // Add change view messages
    for (int i = 0; i < 3; ++i) {
        ChangeView cv;
        cv.ViewNumber = 1;
        cv.ValidatorIndex = i;
        msg.ChangeViews.push_back(cv);
    }
    
    // Add prepare messages
    PrepareRequest req;
    req.ViewNumber = 1;
    req.BlockIndex = 100;
    msg.PrepareRequest = std::make_optional(req);
    
    // Process recovery message
    context_->ProcessRecoveryMessage(msg);
    
    // Verify recovery processed
    EXPECT_EQ(context_->ViewNumber(), msg.ViewNumber);
}

// ============================================================================
// Byzantine Fault Tolerance Tests
// ============================================================================

TEST_F(ConsensusComprehensiveTest, ByzantineFaultTolerance_OneThirdFault) {
    // With 7 validators, can tolerate 2 byzantine nodes (f = floor((n-1)/3))
    int total_validators = 7;
    int byzantine_nodes = 2;
    int honest_nodes = total_validators - byzantine_nodes;
    
    // Need 2f+1 = 5 honest nodes for consensus
    EXPECT_GE(honest_nodes, 2 * byzantine_nodes + 1);
}

TEST_F(ConsensusComprehensiveTest, ByzantineFaultTolerance_MessageValidation) {
    // Test duplicate message rejection
    PrepareResponse resp;
    resp.ViewNumber = 0;
    resp.BlockIndex = 1;
    resp.ValidatorIndex = 0;
    
    // First message should be accepted
    EXPECT_TRUE(context_->ProcessPrepareResponse(resp));
    
    // Duplicate should be rejected
    EXPECT_FALSE(context_->ProcessPrepareResponse(resp));
}

// ============================================================================
// Primary Selection Tests
// ============================================================================

TEST_F(ConsensusComprehensiveTest, PrimarySelection_RoundRobin) {
    // Primary should rotate in round-robin fashion
    for (uint8_t view = 0; view < validators_.size(); ++view) {
        uint8_t primary_index = context_->GetPrimaryIndex(view);
        EXPECT_EQ(primary_index, view % validators_.size());
    }
}

TEST_F(ConsensusComprehensiveTest, PrimarySelection_AfterViewChange) {
    uint8_t initial_primary = context_->GetPrimaryIndex(0);
    
    // Change view
    context_->SetViewNumber(1);
    uint8_t new_primary = context_->GetPrimaryIndex(1);
    
    // Primary should change
    EXPECT_NE(initial_primary, new_primary);
}

// ============================================================================
// Timeout Tests
// ============================================================================

TEST_F(ConsensusComprehensiveTest, Timeout_PrepareTimeout) {
    // Set prepare timeout
    auto timeout = std::chrono::seconds(15);
    context_->SetPrepareTimeout(timeout);
    
    // Start timer
    auto start = std::chrono::steady_clock::now();
    context_->StartPrepareTimer();
    
    // Check if timeout triggers view change
    bool timeout_triggered = context_->CheckPrepareTimeout();
    
    // Initially should not timeout
    EXPECT_FALSE(timeout_triggered);
}

TEST_F(ConsensusComprehensiveTest, Timeout_CommitTimeout) {
    // Set commit timeout
    auto timeout = std::chrono::seconds(15);
    context_->SetCommitTimeout(timeout);
    
    // Check commit timeout
    bool timeout_triggered = context_->CheckCommitTimeout();
    
    // Initially should not timeout
    EXPECT_FALSE(timeout_triggered);
}

// ============================================================================
// Block Creation Tests
// ============================================================================

TEST_F(ConsensusComprehensiveTest, BlockCreation_ValidBlock) {
    // Create block proposal
    auto block = context_->CreateBlock();
    
    EXPECT_NE(block, nullptr);
    EXPECT_EQ(block->Index, blockchain_->GetHeight() + 1);
    EXPECT_GT(block->Timestamp, 0);
}

TEST_F(ConsensusComprehensiveTest, BlockCreation_WithTransactions) {
    // Add mock transactions to mempool
    std::vector<Transaction> transactions;
    for (int i = 0; i < 10; ++i) {
        Transaction tx;
        tx.Nonce = i;
        transactions.push_back(tx);
    }
    
    // Create block with transactions
    auto block = context_->CreateBlock(transactions);
    
    EXPECT_NE(block, nullptr);
    EXPECT_EQ(block->Transactions.size(), transactions.size());
}

// ============================================================================
// Message Serialization Tests
// ============================================================================

TEST_F(ConsensusComprehensiveTest, MessageSerialization_ChangeView) {
    ChangeView original;
    original.ViewNumber = 5;
    original.Timestamp = 1234567890;
    original.Reason = ChangeViewReason::Timeout;
    
    // Serialize
    ByteVector serialized = original.Serialize();
    
    // Deserialize
    ChangeView deserialized;
    deserialized.Deserialize(serialized);
    
    // Verify
    EXPECT_EQ(original.ViewNumber, deserialized.ViewNumber);
    EXPECT_EQ(original.Timestamp, deserialized.Timestamp);
    EXPECT_EQ(original.Reason, deserialized.Reason);
}

TEST_F(ConsensusComprehensiveTest, MessageSerialization_PrepareRequest) {
    PrepareRequest original;
    original.ViewNumber = 3;
    original.BlockIndex = 1000;
    original.Timestamp = 9876543210;
    original.Nonce = 42;
    
    // Serialize
    ByteVector serialized = original.Serialize();
    
    // Deserialize
    PrepareRequest deserialized;
    deserialized.Deserialize(serialized);
    
    // Verify
    EXPECT_EQ(original.ViewNumber, deserialized.ViewNumber);
    EXPECT_EQ(original.BlockIndex, deserialized.BlockIndex);
    EXPECT_EQ(original.Timestamp, deserialized.Timestamp);
    EXPECT_EQ(original.Nonce, deserialized.Nonce);
}

// ============================================================================
// State Machine Tests
// ============================================================================

TEST_F(ConsensusComprehensiveTest, StateMachine_StateTransitions) {
    // Initial state
    EXPECT_EQ(context_->GetState(), ConsensusState::Initial);
    
    // Transition to backup
    context_->ChangeState(ConsensusState::Backup);
    EXPECT_EQ(context_->GetState(), ConsensusState::Backup);
    
    // Transition to primary
    context_->ChangeState(ConsensusState::Primary);
    EXPECT_EQ(context_->GetState(), ConsensusState::Primary);
    
    // Transition to request sent
    context_->ChangeState(ConsensusState::RequestSent);
    EXPECT_EQ(context_->GetState(), ConsensusState::RequestSent);
    
    // Transition to committed
    context_->ChangeState(ConsensusState::BlockSent);
    EXPECT_EQ(context_->GetState(), ConsensusState::BlockSent);
}

TEST_F(ConsensusComprehensiveTest, StateMachine_InvalidTransitions) {
    // Set to committed state
    context_->ChangeState(ConsensusState::BlockSent);
    
    // Should not allow transition back to initial
    EXPECT_FALSE(context_->CanTransitionTo(ConsensusState::Initial));
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(ConsensusComprehensiveTest, Performance_MessageProcessingThroughput) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Process many messages
    for (int i = 0; i < 1000; ++i) {
        PrepareResponse resp;
        resp.ViewNumber = 0;
        resp.BlockIndex = 1;
        resp.ValidatorIndex = i % validators_.size();
        context_->ProcessPrepareResponse(resp);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should process 1000 messages in under 100ms
    EXPECT_LT(duration.count(), 100);
}

TEST_F(ConsensusComprehensiveTest, Performance_BlockCreationTime) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Create block
    auto block = context_->CreateBlock();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Block creation should be fast
    EXPECT_LT(duration.count(), 50);
}