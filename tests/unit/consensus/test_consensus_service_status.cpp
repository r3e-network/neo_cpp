#include <gtest/gtest.h>

#include <neo/consensus/consensus_service.h>
#include <neo/consensus/dbft_consensus.h>
#include <neo/io/byte_vector.h>
#include <neo/network/p2p/payloads/neo3_transaction.h>
#include <neo/node/neo_system.h>
#include <neo/protocol_settings.h>

namespace
{
std::shared_ptr<neo::cryptography::ecc::KeyPair> GenerateKeyPair()
{
    auto key = neo::cryptography::ecc::KeyPair::Generate();
    return std::shared_ptr<neo::cryptography::ecc::KeyPair>(key.release());
}
}  // namespace

TEST(ConsensusServiceStatusTest, InitializesValidatorsFromProtocolSettings)
{
    auto protocolSettings = std::make_shared<neo::ProtocolSettings>();
    auto keyPair = GenerateKeyPair();

    std::vector<neo::cryptography::ecc::ECPoint> committee;
    committee.push_back(keyPair->GetPublicKey());
    protocolSettings->SetStandbyCommittee(committee);
    protocolSettings->SetValidatorsCount(static_cast<int>(committee.size()));

    auto system = std::make_shared<neo::node::NeoSystem>(protocolSettings, "memory");

    neo::consensus::ConsensusService service(system, keyPair);

    auto status = service.GetStatus();
    EXPECT_FALSE(status.running);
    ASSERT_EQ(committee.size(), status.validators.size());
    EXPECT_EQ(committee.front(), status.validators.front());
}

TEST(ConsensusServiceStatusTest, DbftConsensusRemovesCachedTransactions)
{
    auto protocolSettings = std::make_shared<neo::ProtocolSettings>();
    std::vector<neo::cryptography::ecc::ECPoint> committee(7);
    protocolSettings->SetStandbyCommittee(committee);
    protocolSettings->SetValidatorsCount(static_cast<int>(committee.size()));

    neo::node::NeoSystem system(protocolSettings, "memory");

    // Build validator list of UInt160 script hashes
    std::vector<neo::io::UInt160> validators;
    validators.reserve(committee.size());
    for (size_t i = 0; i < committee.size(); ++i)
    {
        neo::io::UInt160 hash;
        hash.Data()[0] = static_cast<uint8_t>(i + 1);
        validators.push_back(hash);
    }

    neo::io::UInt160 node_id = validators.front();

    auto blockchain = system.GetBlockchain();
    auto mempool = system.GetMemoryPool();

    neo::consensus::ConsensusConfig config;
    // Ensure max block size not zero
    config.max_block_size = 1024 * 1024;

    neo::consensus::DbftConsensus consensus(config, node_id, validators, mempool, blockchain);

    neo::network::p2p::payloads::Neo3Transaction tx;
    tx.SetNonce(42);
    tx.SetSystemFee(0);
    tx.SetNetworkFee(0);
    tx.SetValidUntilBlock(100);
    tx.SetScript(neo::io::ByteVector({0x01}));

    const auto hash = tx.GetHash();

    ASSERT_TRUE(consensus.AddTransaction(tx));
    ASSERT_TRUE(consensus.GetState().GetCachedTransaction(hash).has_value());

    consensus.RemoveCachedTransaction(hash);
    EXPECT_FALSE(consensus.GetState().GetCachedTransaction(hash).has_value());
}
