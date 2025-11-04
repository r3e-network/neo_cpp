/**
 * @file test_consensus_complete.cpp
 * @brief Complete consensus tests for dBFT 2.0 protocol
 * Must match Neo C# consensus implementation exactly
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
#include <neo/network/p2p/payloads/block.h>
#include <neo/io/byte_vector.h>
#include <memory>
#include <vector>

using namespace neo::consensus;
using namespace neo::io;
using namespace neo::network::p2p::payloads;

class ConsensusCompleteTest : public ::testing::Test {
protected:
    std::unique_ptr<ConsensusContext> context_;
    std::unique_ptr<ConsensusService> service_;
    std::vector<ECPoint> validators_;
    
    void SetUp() override {
        // Initialize with 7 validators (common test scenario)
        for (int i = 0; i < 7; ++i) {
            ECPoint validator;
            validator.Fill(0x02 + (i % 2));
            validator.Data()[1] = i;
            validators_.push_back(validator);
        }
        
        context_ = std::make_unique<ConsensusContext>(validators_);
        service_ = std::make_unique<ConsensusService>(context_.get());
    }
    
    ConsensusMessage CreateMessage(ConsensusMessageType type, uint8_t viewNumber) {
        ConsensusMessage msg;
        msg.Type = type;
        msg.ViewNumber = viewNumber;
        return msg;
    }
    
    int GetF() const {
        return (validators_.size() - 1) / 3;
    }
    
    int GetM() const {
        return validators_.size() - GetF();
    }
};

// ============================================================================
// Message Type Tests
// ============================================================================

TEST_F(ConsensusCompleteTest, MessageTypes_Values) {
    EXPECT_EQ(static_cast<uint8_t>(ConsensusMessageType::ChangeView), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(ConsensusMessageType::PrepareRequest), 0x20);
    EXPECT_EQ(static_cast<uint8_t>(ConsensusMessageType::PrepareResponse), 0x21);
    EXPECT_EQ(static_cast<uint8_t>(ConsensusMessageType::Commit), 0x30);
    EXPECT_EQ(static_cast<uint8_t>(ConsensusMessageType::RecoveryRequest), 0x40);
    EXPECT_EQ(static_cast<uint8_t>(ConsensusMessageType::RecoveryMessage), 0x41);
}

TEST_F(ConsensusCompleteTest, MessageTypes_Serialization) {
    for (auto type : {ConsensusMessageType::ChangeView, 
                      ConsensusMessageType::PrepareRequest,
                      ConsensusMessageType::PrepareResponse,
                      ConsensusMessageType::Commit}) {
        auto msg = CreateMessage(type, 0);
        
        ByteVector serialized = msg.Serialize();
        ConsensusMessage deserialized;
        deserialized.Deserialize(serialized);
        
        EXPECT_EQ(deserialized.Type, type);
    }
}

// ============================================================================
// Consensus Context Tests
// ============================================================================

TEST_F(ConsensusCompleteTest, Context_Initialization) {
    EXPECT_EQ(context_->ViewNumber, 0);
    EXPECT_EQ(context_->MyIndex, -1); // Not a validator by default
    EXPECT_FALSE(context_->IsPrimary());
    EXPECT_FALSE(context_->IsBackup());
    EXPECT_EQ(context_->Validators.size(), 7);
}

TEST_F(ConsensusCompleteTest, Context_Reset) {
    context_->ViewNumber = 5;
    context_->Reset(0);
    
    EXPECT_EQ(context_->ViewNumber, 0);
    EXPECT_TRUE(context_->PreparationPayloads.empty());
    EXPECT_TRUE(context_->CommitPayloads.empty());
    EXPECT_TRUE(context_->ChangeViewPayloads.empty());
}

TEST_F(ConsensusCompleteTest, Context_GetPrimaryIndex) {
    // Primary index = (block_index - view_number) mod validator_count
    uint32_t blockIndex = 100;
    
    for (uint8_t view = 0; view < 7; ++view) {
        context_->ViewNumber = view;
        int primary = context_->GetPrimaryIndex(blockIndex);
        
        EXPECT_EQ(primary, (blockIndex - view) % validators_.size());
    }
}

TEST_F(ConsensusCompleteTest, Context_IsPrimary) {
    context_->MyIndex = 0;
    context_->ViewNumber = 0;
    
    EXPECT_TRUE(context_->IsPrimary());
    
    context_->ViewNumber = 1;
    EXPECT_FALSE(context_->IsPrimary());
}

TEST_F(ConsensusCompleteTest, Context_IsBackup) {
    context_->MyIndex = 1;
    context_->ViewNumber = 0;
    
    EXPECT_TRUE(context_->IsBackup());
    
    context_->MyIndex = 0;
    EXPECT_FALSE(context_->IsBackup());
}

TEST_F(ConsensusCompleteTest, Context_CountCommitted) {
    // Add some commit payloads
    for (int i = 0; i < 5; ++i) {
        context_->CommitPayloads[i] = std::make_shared<Commit>();
    }
    
    EXPECT_EQ(context_->CountCommitted(), 5);
}

TEST_F(ConsensusCompleteTest, Context_CountFailed) {
    // Add change view payloads
    for (int i = 0; i < 3; ++i) {
        context_->ChangeViewPayloads[i] = std::make_shared<ChangeView>();
    }
    
    EXPECT_EQ(context_->CountFailed(), 3);
}

TEST_F(ConsensusCompleteTest, Context_MoreThanFNodesCommitted) {
    int f = GetF();
    
    // Add exactly F commits - should not be enough
    for (int i = 0; i < f; ++i) {
        context_->CommitPayloads[i] = std::make_shared<Commit>();
    }
    EXPECT_FALSE(context_->MoreThanFNodesCommitted());
    
    // Add one more - should be enough
    context_->CommitPayloads[f] = std::make_shared<Commit>();
    EXPECT_TRUE(context_->MoreThanFNodesCommitted());
}

TEST_F(ConsensusCompleteTest, Context_CreateBlock) {
    // Setup context for block creation
    context_->Timestamp = 1000000;
    context_->Nonce = 12345;
    context_->NextConsensus = UInt160::Parse("0x1234567890abcdef1234567890abcdef12345678");
    
    auto block = context_->CreateBlock();
    
    EXPECT_EQ(block->Timestamp, context_->Timestamp);
    EXPECT_EQ(block->Nonce, context_->Nonce);
    EXPECT_EQ(block->NextConsensus, context_->NextConsensus);
}

// ============================================================================
// ChangeView Tests
// ============================================================================

TEST_F(ConsensusCompleteTest, ChangeView_Creation) {
    ChangeView cv;
    cv.ViewNumber = 1;
    cv.Reason = ChangeViewReason::Timeout;
    cv.Timestamp = 1000000;
    
    EXPECT_EQ(cv.ViewNumber, 1);
    EXPECT_EQ(cv.Reason, ChangeViewReason::Timeout);
    EXPECT_EQ(cv.Timestamp, 1000000);
}

TEST_F(ConsensusCompleteTest, ChangeView_Serialization) {
    ChangeView original;
    original.ViewNumber = 2;
    original.Reason = ChangeViewReason::ChangeAgreement;
    original.Timestamp = 2000000;
    
    auto serialized = original.Serialize();
    
    ChangeView deserialized;
    deserialized.Deserialize(serialized);
    
    EXPECT_EQ(deserialized.ViewNumber, original.ViewNumber);
    EXPECT_EQ(deserialized.Reason, original.Reason);
    EXPECT_EQ(deserialized.Timestamp, original.Timestamp);
}

TEST_F(ConsensusCompleteTest, ChangeView_Reasons) {
    EXPECT_EQ(static_cast<uint8_t>(ChangeViewReason::Timeout), 0x00);
    EXPECT_EQ(static_cast<uint8_t>(ChangeViewReason::ChangeAgreement), 0x01);
    EXPECT_EQ(static_cast<uint8_t>(ChangeViewReason::TxNotFound), 0x02);
    EXPECT_EQ(static_cast<uint8_t>(ChangeViewReason::TxRejectedByPolicy), 0x03);
    EXPECT_EQ(static_cast<uint8_t>(ChangeViewReason::TxInvalid), 0x04);
    EXPECT_EQ(static_cast<uint8_t>(ChangeViewReason::BlockRejectedByPolicy), 0x05);
}

TEST_F(ConsensusCompleteTest, ChangeView_GetDelay) {
    // Delay doubles with each view change
    EXPECT_EQ(ChangeView::GetDelay(0), 1 << 0);
    EXPECT_EQ(ChangeView::GetDelay(1), 1 << 1);
    EXPECT_EQ(ChangeView::GetDelay(2), 1 << 2);
    EXPECT_EQ(ChangeView::GetDelay(3), 1 << 3);
    EXPECT_EQ(ChangeView::GetDelay(4), 1 << 4);
}

// ============================================================================
// PrepareRequest Tests
// ============================================================================

TEST_F(ConsensusCompleteTest, PrepareRequest_Creation) {
    PrepareRequest req;
    req.Version = 0;
    req.PrevHash = UInt256::Parse("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
    req.Timestamp = 1000000;
    req.Nonce = 12345;
    
    EXPECT_EQ(req.Version, 0);
    EXPECT_EQ(req.Timestamp, 1000000);
    EXPECT_EQ(req.Nonce, 12345);
}

TEST_F(ConsensusCompleteTest, PrepareRequest_AddTransaction) {
    PrepareRequest req;
    
    for (int i = 0; i < 10; ++i) {
        UInt256 txHash;
        txHash.Fill(i);
        req.TransactionHashes.push_back(txHash);
    }
    
    EXPECT_EQ(req.TransactionHashes.size(), 10);
}

TEST_F(ConsensusCompleteTest, PrepareRequest_Serialization) {
    PrepareRequest original;
    original.Version = 0;
    original.Timestamp = 3000000;
    original.Nonce = 54321;
    
    UInt256 txHash;
    txHash.Fill(0xAB);
    original.TransactionHashes.push_back(txHash);
    
    auto serialized = original.Serialize();
    
    PrepareRequest deserialized;
    deserialized.Deserialize(serialized);
    
    EXPECT_EQ(deserialized.Version, original.Version);
    EXPECT_EQ(deserialized.Timestamp, original.Timestamp);
    EXPECT_EQ(deserialized.Nonce, original.Nonce);
    EXPECT_EQ(deserialized.TransactionHashes.size(), 1);
    EXPECT_EQ(deserialized.TransactionHashes[0], txHash);
}

TEST_F(ConsensusCompleteTest, PrepareRequest_Validation) {
    PrepareRequest req;
    req.Version = 0;
    req.Timestamp = 0; // Invalid timestamp
    
    EXPECT_FALSE(req.Verify(context_.get()));
    
    req.Timestamp = 1000000;
    EXPECT_TRUE(req.Verify(context_.get()));
}

// ============================================================================
// PrepareResponse Tests
// ============================================================================

TEST_F(ConsensusCompleteTest, PrepareResponse_Creation) {
    PrepareResponse resp;
    resp.PreparationHash = UInt256::Parse("0xabcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890");
    
    EXPECT_NE(resp.PreparationHash, UInt256::Zero());
}

TEST_F(ConsensusCompleteTest, PrepareResponse_Serialization) {
    PrepareResponse original;
    original.PreparationHash.Fill(0xCD);
    
    auto serialized = original.Serialize();
    
    PrepareResponse deserialized;
    deserialized.Deserialize(serialized);
    
    EXPECT_EQ(deserialized.PreparationHash, original.PreparationHash);
}

// ============================================================================
// Commit Tests
// ============================================================================

TEST_F(ConsensusCompleteTest, Commit_Creation) {
    Commit commit;
    commit.Signature.Fill(0xEF);
    
    EXPECT_FALSE(commit.Signature.IsZero());
}

TEST_F(ConsensusCompleteTest, Commit_Serialization) {
    Commit original;
    original.Signature.Fill(0x12);
    
    auto serialized = original.Serialize();
    
    Commit deserialized;
    deserialized.Deserialize(serialized);
    
    EXPECT_EQ(deserialized.Signature, original.Signature);
}

TEST_F(ConsensusCompleteTest, Commit_Verification) {
    Commit commit;
    // Would need proper signature for real verification
    commit.Signature.Fill(0x34);
    
    // Basic verification (would need real crypto in production)
    EXPECT_FALSE(commit.Signature.IsZero());
}

// ============================================================================
// Recovery Tests
// ============================================================================

TEST_F(ConsensusCompleteTest, RecoveryRequest_Creation) {
    RecoveryRequest req;
    req.ViewNumber = 3;
    req.Timestamp = 4000000;
    
    EXPECT_EQ(req.ViewNumber, 3);
    EXPECT_EQ(req.Timestamp, 4000000);
}

TEST_F(ConsensusCompleteTest, RecoveryMessage_Creation) {
    RecoveryMessage msg;
    msg.ViewNumber = 2;
    
    // Add prepare request
    msg.PrepareRequest = std::make_shared<PrepareRequest>();
    msg.PrepareRequest->Timestamp = 5000000;
    
    // Add prepare responses
    for (int i = 0; i < 3; ++i) {
        auto resp = std::make_shared<PrepareResponse>();
        resp->PreparationHash.Fill(i);
        msg.PrepareResponses[i] = resp;
    }
    
    EXPECT_EQ(msg.ViewNumber, 2);
    EXPECT_NE(msg.PrepareRequest, nullptr);
    EXPECT_EQ(msg.PrepareResponses.size(), 3);
}

TEST_F(ConsensusCompleteTest, RecoveryMessage_AddChangeViews) {
    RecoveryMessage msg;
    
    for (int i = 0; i < 4; ++i) {
        auto cv = std::make_shared<ChangeView>();
        cv->ViewNumber = i;
        cv->Reason = ChangeViewReason::Timeout;
        msg.ChangeViewMessages[i] = cv;
    }
    
    EXPECT_EQ(msg.ChangeViewMessages.size(), 4);
}

TEST_F(ConsensusCompleteTest, RecoveryMessage_AddCommits) {
    RecoveryMessage msg;
    
    for (int i = 0; i < 5; ++i) {
        auto commit = std::make_shared<Commit>();
        commit->Signature.Fill(i);
        msg.CommitMessages[i] = commit;
    }
    
    EXPECT_EQ(msg.CommitMessages.size(), 5);
}

// ============================================================================
// Byzantine Fault Tests
// ============================================================================

TEST_F(ConsensusCompleteTest, Byzantine_FaultTolerance) {
    // With 7 nodes, f = 2, so we can tolerate 2 Byzantine nodes
    int f = GetF();
    EXPECT_EQ(f, 2);
    
    // Need at least 2f+1 = 5 nodes for consensus
    int minHonest = 2 * f + 1;
    EXPECT_EQ(minHonest, 5);
    
    // Maximum Byzantine nodes
    EXPECT_LT(f, validators_.size() / 2);
}

TEST_F(ConsensusCompleteTest, Byzantine_InvalidPrepareRequest) {
    PrepareRequest req;
    req.Version = 255; // Invalid version
    req.Timestamp = 1000000;
    
    EXPECT_FALSE(req.Verify(context_.get()));
}

TEST_F(ConsensusCompleteTest, Byzantine_DuplicateMessages) {
    // Add same prepare response twice from same node
    context_->PreparationPayloads[0] = std::make_shared<PrepareResponse>();
    context_->PreparationPayloads[0] = std::make_shared<PrepareResponse>();
    
    // Should only count as one
    int count = 0;
    for (const auto& [idx, payload] : context_->PreparationPayloads) {
        if (payload) count++;
    }
    EXPECT_EQ(count, 1);
}

TEST_F(ConsensusCompleteTest, Byzantine_ConflictingMessages) {
    // Node sends prepare response for different hashes
    PrepareResponse resp1;
    resp1.PreparationHash.Fill(0x01);
    
    PrepareResponse resp2;
    resp2.PreparationHash.Fill(0x02);
    
    // Should detect conflict
    context_->PreparationPayloads[0] = std::make_shared<PrepareResponse>(resp1);
    
    // Attempting to add conflicting message should be rejected
    bool canAdd = context_->CanAddPayload(0, std::make_shared<PrepareResponse>(resp2));
    EXPECT_FALSE(canAdd);
}

// ============================================================================
// View Change Tests
// ============================================================================

TEST_F(ConsensusCompleteTest, ViewChange_Timeout) {
    // Initial view
    EXPECT_EQ(context_->ViewNumber, 0);
    
    // Simulate timeout - add F+1 change view messages
    int f = GetF();
    for (int i = 0; i <= f; ++i) {
        auto cv = std::make_shared<ChangeView>();
        cv->ViewNumber = 1;
        cv->Reason = ChangeViewReason::Timeout;
        context_->ChangeViewPayloads[i] = cv;
    }
    
    // Should trigger view change
    EXPECT_TRUE(context_->ShouldChangeView());
}

TEST_F(ConsensusCompleteTest, ViewChange_MultipleViews) {
    // Test view changes through multiple views
    for (uint8_t targetView = 1; targetView <= 5; ++targetView) {
        context_->Reset(targetView);
        EXPECT_EQ(context_->ViewNumber, targetView);
        
        // Primary changes with view
        int primary = context_->GetPrimaryIndex(100);
        EXPECT_EQ(primary, (100 - targetView) % validators_.size());
    }
}

// ============================================================================
// Block Finalization Tests
// ============================================================================

TEST_F(ConsensusCompleteTest, BlockFinalization_RequiredSignatures) {
    // Need M signatures to finalize block
    int m = GetM();
    EXPECT_EQ(m, validators_.size() - GetF());
    
    // With 7 validators, need 5 signatures
    EXPECT_EQ(m, 5);
}

TEST_F(ConsensusCompleteTest, BlockFinalization_CreateBlockWithSignatures) {
    // Add M commit messages
    int m = GetM();
    for (int i = 0; i < m; ++i) {
        auto commit = std::make_shared<Commit>();
        commit->Signature.Fill(i);
        context_->CommitPayloads[i] = commit;
    }
    
    // Should have enough signatures to create block
    EXPECT_GE(context_->CountCommitted(), m);
    
    auto block = context_->CreateBlock();
    EXPECT_NE(block, nullptr);
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(ConsensusCompleteTest, Performance_MessageProcessing) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Process 1000 messages
    for (int i = 0; i < 1000; ++i) {
        auto msg = CreateMessage(ConsensusMessageType::PrepareResponse, 0);
        // Process message (simplified)
        service_->ProcessMessage(msg);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should process 1000 messages in under 100ms
    EXPECT_LT(duration.count(), 100);
}

TEST_F(ConsensusCompleteTest, Performance_BlockCreation) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Create 100 blocks
    for (int i = 0; i < 100; ++i) {
        auto block = context_->CreateBlock();
        EXPECT_NE(block, nullptr);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should create 100 blocks in under 100ms
    EXPECT_LT(duration.count(), 100);
}
