#include <gtest/gtest.h>

#include <neo/consensus/consensus_payload_helper.h>
#include <neo/consensus/prepare_request.h>
#include <neo/io/uint160.h>

using namespace neo;
using namespace neo::consensus;
using namespace neo::network::p2p::payloads;

TEST(ConsensusPayloadHelperTest, CreatePayloadWrapsMessage)
{
    auto request = std::make_shared<PrepareRequestMessage>();
    request->SetBlockIndex(10);
    request->SetViewNumber(2);
    request->SetValidatorIndex(1);
    request->SetNonce(42);
    request->SetTimestamp(std::chrono::system_clock::time_point{std::chrono::milliseconds{123456}});

    io::UInt160 sender;
    auto payload =
        ConsensusPayloadHelper::CreatePayload(request, sender, /*valid start*/ 10, /*valid end*/ 12);

    ASSERT_NE(payload, nullptr);
    EXPECT_EQ(payload->GetCategory(), ConsensusPayloadHelper::CONSENSUS_CATEGORY);
    EXPECT_EQ(payload->GetValidBlockStart(), 10u);
    EXPECT_EQ(payload->GetValidBlockEnd(), 12u);
    EXPECT_EQ(payload->GetSender(), sender);
}

TEST(ConsensusPayloadHelperTest, RoundTripExtractsMessage)
{
    auto request = std::make_shared<PrepareRequestMessage>();
    request->SetBlockIndex(25);
    request->SetViewNumber(3);
    request->SetValidatorIndex(4);
    request->SetNonce(99);
    request->SetTimestamp(std::chrono::system_clock::time_point{std::chrono::milliseconds{7890}});

    io::UInt160 sender;
    auto payload = ConsensusPayloadHelper::CreatePayload(request, sender, 25, 30);
    ASSERT_NE(payload, nullptr);

    auto message = ConsensusPayloadHelper::GetMessage(*payload);
    ASSERT_NE(message, nullptr);
    EXPECT_EQ(message->GetType(), ConsensusMessageType::PrepareRequest);
    EXPECT_EQ(message->GetBlockIndex(), 25u);
    EXPECT_EQ(message->GetViewNumber(), 3u);
    EXPECT_EQ(message->GetValidatorIndex(), 4u);
}

TEST(ConsensusPayloadHelperTest, IsConsensusPayload)
{
    auto request = std::make_shared<PrepareRequestMessage>();
    io::UInt160 sender;
    auto payload = ConsensusPayloadHelper::CreatePayload(request, sender, 0, 1);
    ASSERT_NE(payload, nullptr);
    EXPECT_TRUE(ConsensusPayloadHelper::IsConsensusPayload(*payload));

    ExtensiblePayload other;
    other.SetCategory("other");
    EXPECT_FALSE(ConsensusPayloadHelper::IsConsensusPayload(other));
}
