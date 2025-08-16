/**
 * @file test_consensus_integration.cpp
 * @brief Consensus integration test suite
 */

#include <gtest/gtest.h>
#include <neo/consensus/consensus_context.h>
#include <neo/consensus/consensus_message.h>
#include <neo/ledger/blockchain.h>
#include <thread>
#include <chrono>

namespace neo::consensus::tests {

class ConsensusIntegrationTest : public ::testing::Test {
protected:
    std::unique_ptr<ConsensusContext> context;
    std::unique_ptr<ledger::Blockchain> blockchain;
    
    void SetUp() override {
        blockchain = std::make_unique<ledger::Blockchain>();
        context = std::make_unique<ConsensusContext>(blockchain.get());
    }
    
    void TearDown() override {
        context.reset();
        blockchain.reset();
    }
};

TEST_F(ConsensusIntegrationTest, InitializeConsensus) {
    EXPECT_FALSE(context->IsRunning());
    context->Start();
    EXPECT_TRUE(context->IsRunning());
    context->Stop();
    EXPECT_FALSE(context->IsRunning());
}

TEST_F(ConsensusIntegrationTest, ViewChange) {
    context->Start();
    
    auto initialView = context->ViewNumber;
    context->ChangeView(ConsensusMessageType::ChangeView);
    
    EXPECT_GT(context->ViewNumber, initialView);
    
    context->Stop();
}

TEST_F(ConsensusIntegrationTest, PrepareRequest) {
    context->Start();
    
    PrepareRequest request;
    request.Version = 0;
    request.PrevHash = blockchain->GetCurrentBlockHash();
    request.BlockIndex = blockchain->GetHeight() + 1;
    request.ViewNumber = context->ViewNumber;
    request.Timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    request.NextConsensus = io::UInt160::Zero();
    
    auto result = context->OnPrepareRequest(request);
    EXPECT_TRUE(result.Processed);
    
    context->Stop();
}

TEST_F(ConsensusIntegrationTest, PrepareResponse) {
    context->Start();
    
    // First send prepare request
    PrepareRequest request;
    request.Version = 0;
    request.BlockIndex = blockchain->GetHeight() + 1;
    request.ViewNumber = context->ViewNumber;
    context->OnPrepareRequest(request);
    
    // Then send prepare response
    PrepareResponse response;
    response.ViewNumber = context->ViewNumber;
    response.BlockHash = request.GetHash();
    
    auto result = context->OnPrepareResponse(response);
    EXPECT_TRUE(result.Processed);
    
    context->Stop();
}

TEST_F(ConsensusIntegrationTest, CommitMessage) {
    context->Start();
    
    Commit commit;
    commit.ViewNumber = context->ViewNumber;
    commit.BlockIndex = blockchain->GetHeight() + 1;
    
    auto result = context->OnCommit(commit);
    EXPECT_TRUE(result.Processed);
    
    context->Stop();
}

TEST_F(ConsensusIntegrationTest, MultipleValidators) {
    const int validatorCount = 4;
    std::vector<std::unique_ptr<ConsensusContext>> validators;
    
    for (int i = 0; i < validatorCount; ++i) {
        validators.push_back(std::make_unique<ConsensusContext>(blockchain.get()));
        validators[i]->Start();
    }
    
    // Simulate consensus round
    for (auto& validator : validators) {
        EXPECT_TRUE(validator->IsRunning());
    }
    
    // Stop all validators
    for (auto& validator : validators) {
        validator->Stop();
        EXPECT_FALSE(validator->IsRunning());
    }
}

TEST_F(ConsensusIntegrationTest, ConsensusTimeout) {
    context->Start();
    context->SetTimeout(std::chrono::milliseconds(100));
    
    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    
    // View should have changed after timeout
    auto initialView = context->ViewNumber;
    context->OnTimeout();
    EXPECT_GT(context->ViewNumber, initialView);
    
    context->Stop();
}

TEST_F(ConsensusIntegrationTest, RecoveryMessage) {
    context->Start();
    
    RecoveryMessage recovery;
    recovery.ViewNumber = context->ViewNumber;
    recovery.BlockIndex = blockchain->GetHeight() + 1;
    
    auto result = context->OnRecoveryMessage(recovery);
    EXPECT_TRUE(result.Processed);
    
    context->Stop();
}

TEST_F(ConsensusIntegrationTest, InvalidViewNumber) {
    context->Start();
    
    PrepareRequest request;
    request.ViewNumber = context->ViewNumber + 100; // Invalid view
    request.BlockIndex = blockchain->GetHeight() + 1;
    
    auto result = context->OnPrepareRequest(request);
    EXPECT_FALSE(result.Processed);
    EXPECT_TRUE(result.ShouldChangeView);
    
    context->Stop();
}

TEST_F(ConsensusIntegrationTest, ConsensusStateTransitions) {
    context->Start();
    
    EXPECT_EQ(context->GetState(), ConsensusState::Initial);
    
    // Send prepare request
    PrepareRequest request;
    request.BlockIndex = blockchain->GetHeight() + 1;
    request.ViewNumber = context->ViewNumber;
    context->OnPrepareRequest(request);
    
    EXPECT_EQ(context->GetState(), ConsensusState::RequestSent);
    
    // Simulate receiving enough responses
    for (int i = 0; i < 3; ++i) {
        PrepareResponse response;
        response.ViewNumber = context->ViewNumber;
        context->OnPrepareResponse(response);
    }
    
    EXPECT_EQ(context->GetState(), ConsensusState::ResponseSent);
    
    context->Stop();
}

} // namespace neo::consensus::tests