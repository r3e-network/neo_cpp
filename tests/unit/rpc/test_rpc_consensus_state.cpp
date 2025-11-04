#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <neo/consensus/consensus_message.h>
#include <neo/consensus/consensus_service.h>
#include <neo/cryptography/crypto.h>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/cryptography/hash.h>
#include <neo/io/uint256.h>
#include <neo/network/p2p/local_node.h>
#include <neo/rpc/rpc_methods.h>

#include <optional>
#include <string>
#include <vector>

TEST(RPCConsensusStateTest, ReturnsErrorWhenLocalNodeUnavailable)
{
    neo::rpc::RPCMethods::SetConsensusServiceOverrideForTesting(std::nullopt);
    neo::network::p2p::LocalNode::GetInstance().SetConsensusService(nullptr);

    nlohmann::json params = nlohmann::json::array();
    std::shared_ptr<neo::NeoSystem> system;
    auto result = neo::rpc::RPCMethods::GetConsensusState(system, params);

    ASSERT_TRUE(result.contains("running"));
    EXPECT_FALSE(result["running"].get<bool>());
    ASSERT_TRUE(result.contains("error"));
    EXPECT_EQ("Consensus service unavailable", result["error"].get<std::string>());
}

TEST(RPCConsensusStateTest, ReturnsDetailedConsensusState)
{
    neo::rpc::RPCMethods::SetConsensusServiceOverrideForTesting(std::nullopt);
    neo::network::p2p::LocalNode::GetInstance().SetConsensusService(nullptr);

    std::vector<std::string> validatorHex = {
        "03b209fd4f53a7170ea4444e0cb0a6bb6a53c2bd016926989cf85f9b0fba17a70c",
        "02df48f60e8f3e01c48ff40b9b7f1310d7a8b2a193188befe1c2e3df740e895093",
        "03ab2f4f40f4f06bdbd293c9c530f5dbe9a359d8a20b19be3cfa4d8e436a6fd9de"};

    std::vector<neo::cryptography::ecc::ECPoint> validators;
    validators.reserve(validatorHex.size());
    for (const auto& hex : validatorHex)
    {
        validators.emplace_back(neo::cryptography::ecc::ECPoint::FromHex(hex));
    }

    neo::consensus::ConsensusService::Status status;
    status.running = true;
    status.blockIndex = 42;
    status.viewNumber = 3;
    status.phase = neo::consensus::ConsensusPhase::Primary;
    status.prepareResponseCount = 5;
    status.commitCount = 4;
    status.viewChangeCount = 1;
    status.validators = validators;
    status.primaryIndex = 1;
    status.validatorIndex = 2;
    status.validatorStates.resize(validators.size());
    status.validatorStates[0].hasProposal = true;
    status.validatorStates[1].hasPrepareResponse = true;
    status.validatorStates[1].viewChangeReason = neo::consensus::ChangeViewReason::TxInvalid;
    status.validatorStates[2].hasCommit = true;
    status.validatorStates[2].requestedView = 4;
    status.expectedTransactionCount = 5;
    status.transactionCount = 3;
    status.timestampMilliseconds = 123456789ULL;
    status.nonce = 424242ULL;
    status.prepareRequestHash = neo::io::UInt256::Parse("0x1");

    std::vector<neo::io::UInt160> validatorHashes;
    validatorHashes.reserve(validators.size());
    for (const auto& validator : validators)
    {
        auto script = neo::cryptography::Crypto::CreateSignatureRedeemScript(validator);
        validatorHashes.emplace_back(neo::cryptography::Hash::Hash160(script.AsSpan()));
    }

    neo::rpc::RPCMethods::SetConsensusServiceOverrideForTesting(status, validatorHashes);

    nlohmann::json params = nlohmann::json::array();
    auto result = neo::rpc::RPCMethods::GetConsensusState(nullptr, params);

    ASSERT_TRUE(result.contains("running"));
    EXPECT_TRUE(result["running"].get<bool>());
    EXPECT_EQ(status.blockIndex, result["blockindex"].get<uint32_t>());
    EXPECT_EQ(status.viewNumber, result["viewnumber"].get<uint32_t>());
    EXPECT_EQ("Primary", result["phase"].get<std::string>());
    EXPECT_EQ(status.prepareResponseCount, result["prepareresponses"].get<size_t>());
    EXPECT_EQ(status.commitCount, result["commits"].get<size_t>());
    EXPECT_EQ(status.viewChangeCount, result["viewchanges"].get<size_t>());
    EXPECT_EQ(status.primaryIndex, result["primaryindex"].get<uint32_t>());
    EXPECT_EQ(status.validatorIndex, result["validatorindex"].get<uint16_t>());
    EXPECT_EQ(validators.size(), result["validatorcount"].get<size_t>());
    EXPECT_EQ(status.expectedTransactionCount, result["expectedtransactions"].get<size_t>());
    EXPECT_EQ(status.transactionCount, result["transactioncount"].get<size_t>());
    EXPECT_TRUE(result["proposalhash"].is_string());
    EXPECT_EQ(*status.timestampMilliseconds, result["timestamp"].get<uint64_t>());
    EXPECT_EQ(*status.nonce, result["nonce"].get<uint64_t>());

    const auto& validatorsJson = result["validators"];
    ASSERT_EQ(validators.size(), validatorsJson.size());
    for (size_t i = 0; i < validators.size(); ++i)
    {
        EXPECT_EQ(validators[i].ToString(), validatorsJson[i]["publickey"].get<std::string>());
        EXPECT_EQ(validatorHashes[i].ToString(), validatorsJson[i]["scripthash"].get<std::string>());
        EXPECT_TRUE(validatorsJson[i].contains("hasproposal"));
        EXPECT_TRUE(validatorsJson[i].contains("hasprepareresponse"));
        EXPECT_TRUE(validatorsJson[i].contains("hascommit"));
        EXPECT_TRUE(validatorsJson[i].contains("viewchangereason"));
        EXPECT_TRUE(validatorsJson[i].contains("requestedview"));
        if (i != 2)
        {
            EXPECT_TRUE(validatorsJson[i]["requestedview"].is_null());
        }
    }
    EXPECT_TRUE(validatorsJson[status.primaryIndex]["isprimary"].get<bool>());
    EXPECT_TRUE(validatorsJson[status.validatorIndex]["isme"].get<bool>());
    EXPECT_TRUE(validatorsJson[0]["hasproposal"].get<bool>());
    EXPECT_TRUE(validatorsJson[1]["hasprepareresponse"].get<bool>());
    EXPECT_TRUE(validatorsJson[2]["hascommit"].get<bool>());
    EXPECT_EQ("TxInvalid", validatorsJson[1]["viewchangereason"].get<std::string>());
    EXPECT_TRUE(validatorsJson[2]["viewchangereason"].is_null());
    EXPECT_TRUE(validatorsJson[2]["requestedview"].is_number());

    neo::rpc::RPCMethods::SetConsensusServiceOverrideForTesting(std::nullopt);
    neo::network::p2p::LocalNode::GetInstance().SetConsensusService(nullptr);
}
