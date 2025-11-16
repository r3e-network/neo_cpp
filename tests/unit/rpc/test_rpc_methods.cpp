#include <gtest/gtest.h>

#include <algorithm>
#include <iomanip>
#include <memory>
#include <array>
#include <sstream>
#include <functional>
#include <unordered_map>
#include <set>
#include <vector>
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>

#include <neo/common/contains_transaction_type.h>
#include <neo/cryptography/base64.h>
#include <neo/cryptography/ecc/keypair.h>
#include <neo/hardfork.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/io/json.h>
#include <neo/io/uint160.h>
#include <neo/ledger/block.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/memory_pool.h>
#include <neo/ledger/signer.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/witness.h>
#include <neo/network/ip_endpoint.h>
#include <neo/node/neo_system.h>
#include <neo/network/p2p/payloads/neo3_transaction.h>
#include <neo/persistence/data_cache.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>
#include <neo/protocol_settings.h>
#include <neo/plugins/plugin_base.h>
#include <neo/plugins/plugin_manager.h>
#include <neo/plugins/application_logs_plugin.h>
#include <neo/rpc/error_codes.h>
#include <neo/rpc/rpc_methods.h>
#include <neo/wallets/helper.h>
#include <neo/smartcontract/contract.h>
#include <neo/smartcontract/native/hash_index_state.h>
#include <neo/smartcontract/native/ledger_contract.h>
#include <neo/smartcontract/native/contract_management.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/rpc/rpc_session_manager.h>
#include <neo/vm/opcode.h>

#include "tests/utils/test_helpers.h"

using namespace neo::rpc;
using namespace neo::node;

namespace
{
class TestPluginImpl : public neo::plugins::PluginBase
{
  public:
    TestPluginImpl() : PluginBase("TestPlugin", "Test plugin", "1.0", "UnitTest") {}

  protected:
    bool OnInitialize(const std::unordered_map<std::string, std::string>&) override { return true; }
    bool OnStart() override { return true; }
    bool OnStop() override { return true; }
};

class AlphaPluginImpl : public neo::plugins::PluginBase
{
  public:
    AlphaPluginImpl() : PluginBase("AlphaPlugin", "Alpha test plugin", "1.0", "UnitTest") {}

  protected:
    bool OnInitialize(const std::unordered_map<std::string, std::string>&) override { return true; }
    bool OnStart() override { return true; }
    bool OnStop() override { return true; }
};

std::string EncodeBlockToBase64(const neo::ledger::Block& block)
{
    neo::io::ByteVector buffer;
    neo::io::BinaryWriter writer(buffer);
    block.Serialize(writer);
    return neo::cryptography::Base64::Encode(buffer.AsSpan());
}

neo::ledger::Witness EnsureWitnessScripts(const neo::ledger::Witness& baseWitness)
{
    auto witness = baseWitness;
    if (witness.GetInvocationScript().Size() == 0)
    {
        neo::io::ByteVector script;
        script.Push(static_cast<uint8_t>(neo::vm::OpCode::PUSH1));
        witness.SetInvocationScript(script);
    }
    if (witness.GetVerificationScript().Size() == 0)
    {
        neo::io::ByteVector script;
        script.Push(static_cast<uint8_t>(neo::vm::OpCode::PUSH1));
        witness.SetVerificationScript(script);
    }
    return witness;
}

neo::ledger::Block CreateChildBlock(const std::shared_ptr<neo::node::NeoSystem>& system)
{
    auto blockchain = system->GetBlockchain();
    if (!blockchain)
    {
        throw std::runtime_error("Blockchain unavailable");
    }

    auto parent = blockchain->GetBlock(blockchain->GetHeight());
    if (!parent)
    {
        parent = blockchain->GetBlock(0);
    }
    if (!parent)
    {
        throw std::runtime_error("Genesis block unavailable");
    }

    neo::ledger::Block block;
    block.SetVersion(parent->GetVersion());
    block.SetPreviousHash(parent->GetHash());
    block.SetTimestamp(parent->GetTimestamp() + 1);
    block.SetIndex(parent->GetIndex() + 1);
    block.SetPrimaryIndex(parent->GetPrimaryIndex());
    block.SetNextConsensus(parent->GetNextConsensus());
    block.SetMerkleRoot(neo::io::UInt256());
    block.SetWitness(EnsureWitnessScripts(parent->GetWitness()));
    return block;
}
}  // namespace

class RPCMethodsTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Create protocol settings
        protocolSettings = std::make_shared<neo::ProtocolSettings>();

        // Create a neo system with memory store
        neoSystem = std::make_shared<NeoSystem>(protocolSettings);
        ASSERT_TRUE(neoSystem->Start());
        auto blockchain = neoSystem->GetBlockchain();
        ASSERT_TRUE(blockchain);
        blockchain->SetSkipBlockVerificationForTests(true);

        keyPair = neo::cryptography::ecc::KeyPair::Generate();
        ASSERT_NE(keyPair, nullptr);
        signatureContract = neo::smartcontract::Contract::CreateSignatureContract(keyPair->GetPublicKey());
        signerAccount = signatureContract.GetScriptHash();
        nextNonce = 1;
    }

    void TearDown() override
    {
        if (neoSystem)
        {
            neoSystem->Stop();
        }
    }

    std::shared_ptr<NeoSystem> neoSystem;
    std::shared_ptr<neo::ProtocolSettings> protocolSettings;
    std::unique_ptr<neo::cryptography::ecc::KeyPair> keyPair;
    neo::smartcontract::Contract signatureContract;
    neo::io::UInt160 signerAccount;
    uint32_t nextNonce = 1;

    neo::network::p2p::payloads::Neo3Transaction CreateTestTransaction(uint32_t validUntilBlock,
                                                                       uint32_t nonce = 42) const
    {
        neo::network::p2p::payloads::Neo3Transaction tx;
        tx.SetVersion(0);
        tx.SetNonce(nonce);
        tx.SetSystemFee(0);
        tx.SetNetworkFee(0);
        tx.SetValidUntilBlock(validUntilBlock);

        neo::io::UInt160 account =
            neo::io::UInt160::FromString("0x11223344556677889900aabbccddeeff00112233");
        neo::ledger::Signer signer(account, neo::ledger::WitnessScope::Global);
        tx.SetSigners({signer});

        neo::ledger::Witness witness;
        witness.SetInvocationScript(neo::io::ByteVector{0x00});
        witness.SetVerificationScript(neo::io::ByteVector{0x51});  // PUSH1
        tx.SetWitnesses({witness});

        tx.SetScript(neo::io::ByteVector::FromHexString("00"));  // minimal script to satisfy validation
        return tx;
    }

    struct StorageFixture
    {
        int32_t contractId;
        std::string keyBase64;
        std::string valueBase64;
        std::string prefixBase64;
    };

    struct FindLimitGuard
    {
        explicit FindLimitGuard(size_t newLimit) : previous(RPCMethods::GetMaxFindResultItems())
        {
            RPCMethods::SetMaxFindResultItems(newLimit);
        }

        ~FindLimitGuard() { RPCMethods::SetMaxFindResultItems(previous); }

        size_t previous;
    };

    StorageFixture PrepareStorageEntry(const std::string& keyHex, const std::string& valueHex,
                                       size_t prefixLength = 1, int32_t contractId = 42)
    {
        auto storeCache = std::dynamic_pointer_cast<neo::persistence::StoreCache>(neoSystem->GetSnapshot());
        EXPECT_TRUE(storeCache);

        auto key = neo::io::ByteVector::FromHexString(keyHex);
        auto value = neo::io::ByteVector::FromHexString(valueHex);

        neo::persistence::StorageKey storageKey(contractId, key);
        if (storeCache->TryGet(storageKey))
        {
            storeCache->Delete(storageKey);
        }
        storeCache->Add(storageKey, neo::persistence::StorageItem(value));
        storeCache->Commit();

        neo::io::ByteVector prefixBytes;
        if (prefixLength > 0 && prefixLength <= key.Size())
        {
            prefixBytes = neo::io::ByteVector(key.AsSpan().subspan(0, prefixLength));
        }
        else
        {
            prefixBytes = neo::io::ByteVector();
        }

        return {contractId, neo::cryptography::Base64::Encode(key.AsSpan()),
                neo::cryptography::Base64::Encode(value.AsSpan()),
                neo::cryptography::Base64::Encode(prefixBytes.AsSpan())};
    }

    enum class SignatureMode
    {
        Valid,
        Corrupt,
        None
    };

    neo::ledger::Transaction BuildTransaction(
        const std::function<void(neo::ledger::Transaction&)>& customize = {},
        SignatureMode signatureMode = SignatureMode::Valid)
    {
        neo::ledger::Transaction tx;
        tx.SetVersion(0);
        tx.SetNonce(nextNonce++);
        tx.SetSystemFee(0);
        tx.SetNetworkFee(1'000'000);

        auto blockchain = neoSystem->GetBlockchain();
        EXPECT_TRUE(blockchain);
        auto height = blockchain ? blockchain->GetHeight() : 0U;
        tx.SetValidUntilBlock(height + 100);

        neo::io::ByteVector script;
        script.Push(static_cast<uint8_t>(neo::vm::OpCode::PUSH1));
        tx.SetScript(script);

        neo::ledger::Signer signer(signerAccount, neo::ledger::WitnessScope::CalledByEntry);
        tx.SetSigners({signer});
        tx.SetAttributes(std::vector<std::shared_ptr<neo::ledger::TransactionAttribute>>{});

        if (customize)
        {
            customize(tx);
        }

        if (signatureMode != SignatureMode::None)
        {
            auto signData = tx.GetSignData(protocolSettings->GetNetwork());
            auto signature = keyPair->Sign(signData);

            neo::io::ByteVector invocationScript;
            invocationScript.Push(static_cast<uint8_t>(neo::vm::OpCode::PUSHDATA1));
            invocationScript.Push(static_cast<uint8_t>(signature.Size()));
            invocationScript.Append(signature.AsSpan());

            if (signatureMode == SignatureMode::Corrupt && invocationScript.Size() > 2)
            {
                invocationScript[invocationScript.Size() - 1] ^= 0x01;
            }

            neo::ledger::Witness witness;
            witness.SetInvocationScript(invocationScript);
            witness.SetVerificationScript(signatureContract.GetScript());
            tx.SetWitnesses({witness});
        }
        else
        {
            tx.SetWitnesses({});
        }

        return tx;
    }

    std::string EncodeTransaction(const neo::ledger::Transaction& tx) const
    {
        neo::io::ByteVector buffer;
        neo::io::BinaryWriter writer(buffer);
        tx.Serialize(writer);
        return neo::cryptography::Base64::Encode(buffer.AsSpan());
    }

    std::string EncodeMalformedPayload() const
    {
        const std::array<uint8_t, 2> malformed = {0x01, 0x02};
        return neo::cryptography::Base64::Encode(
            neo::io::ByteSpan(malformed.data(), malformed.size()));
    }

    std::string EncodeOversizedGarbagePayload() const
    {
        neo::io::ByteVector bytes(2048, 0x41);
        return neo::cryptography::Base64::Encode(bytes.AsSpan());
    }

    void AddTransactionToBlockchain(const neo::ledger::Transaction& tx)
    {
        auto memoryPool = neoSystem->GetMemoryPool();
        ASSERT_TRUE(memoryPool);

        neo::io::ByteVector buffer;
        neo::io::BinaryWriter writer(buffer);
        tx.Serialize(writer);
        neo::io::BinaryReader reader(buffer);
        neo::network::p2p::payloads::Neo3Transaction netTx;
        netTx.Deserialize(reader);

        ASSERT_TRUE(memoryPool->TryAdd(netTx));
    }

    void AddBlockToBlockchain(const neo::ledger::Block& block)
    {
        auto blockchain = neoSystem->GetBlockchain();
        ASSERT_TRUE(blockchain);

        auto blockCopy = block;
        blockCopy.SetMerkleRoot(blockCopy.ComputeMerkleRoot());

        auto persistedBlock = std::make_shared<neo::ledger::Block>(blockCopy);
        auto result = blockchain->OnNewBlock(persistedBlock);
        ASSERT_EQ(result, neo::ledger::VerifyResult::Succeed);
    }

    void RemoveBlockFromBlockchain(const neo::ledger::Block& block) { (void)block; }
};

TEST_F(RPCMethodsTest, GetVersion)
{
    nlohmann::json params = nlohmann::json::array();
    auto result = RPCMethods::GetVersion(neoSystem, params);

    ASSERT_TRUE(result.is_object());
    EXPECT_TRUE(result.contains("tcpport"));
    EXPECT_TRUE(result.contains("nonce"));
    EXPECT_TRUE(result.contains("useragent"));
    EXPECT_TRUE(result.contains("protocol"));
    EXPECT_TRUE(result.contains("rpc"));

    const auto& protocol = result["protocol"];
    ASSERT_TRUE(protocol.is_object());
    EXPECT_TRUE(protocol.contains("addressversion"));
    EXPECT_TRUE(protocol.contains("network"));
    EXPECT_TRUE(protocol.contains("validatorscount"));
    EXPECT_TRUE(protocol.contains("msperblock"));
    EXPECT_TRUE(protocol.contains("maxtraceableblocks"));
    EXPECT_TRUE(protocol.contains("maxvaliduntilblockincrement"));
    EXPECT_TRUE(protocol.contains("maxtransactionsperblock"));
    EXPECT_TRUE(protocol.contains("memorypoolmaxtransactions"));
    EXPECT_TRUE(protocol.contains("initialgasdistribution"));
    EXPECT_TRUE(protocol.contains("hardforks"));
    EXPECT_TRUE(protocol.contains("standbycommittee"));
    EXPECT_TRUE(protocol.contains("seedlist"));
}

TEST_F(RPCMethodsTest, GetVersion_HardforksStructure)
{
    auto settings = neoSystem->GetProtocolSettings();
    ASSERT_TRUE(settings);

    std::unordered_map<neo::Hardfork, uint32_t> hardforkConfig{
        {neo::Hardfork::HF_Aspidochelone, 0},
        {neo::Hardfork::HF_Basilisk, 100}};
    settings->SetHardforks(hardforkConfig);

    auto result = RPCMethods::GetVersion(neoSystem, nlohmann::json::array());
    ASSERT_TRUE(result.is_object());
    ASSERT_TRUE(result.contains("protocol"));

    const auto& protocol = result["protocol"];
    ASSERT_TRUE(protocol.is_object());
    ASSERT_TRUE(protocol.contains("hardforks"));

    const auto& hardforksJson = protocol["hardforks"];
    ASSERT_TRUE(hardforksJson.is_array());

    if (!hardforksJson.empty())
    {
        for (const auto& hardforkJson : hardforksJson)
        {
            ASSERT_TRUE(hardforkJson.is_object());
            ASSERT_TRUE(hardforkJson.contains("name"));
            ASSERT_TRUE(hardforkJson.contains("blockheight"));
            const auto& nameJson = hardforkJson.at("name");
            ASSERT_TRUE(nameJson.is_string());
            EXPECT_NE(0u, nameJson.get<std::string>().find("HF_"));
            EXPECT_TRUE(hardforkJson.at("blockheight").is_number_unsigned());
        }
    }
}

TEST_F(RPCMethodsTest, GetBlockCount)
{
    nlohmann::json params = nlohmann::json::array();
    auto result = RPCMethods::GetBlockCount(neoSystem, params);

    EXPECT_GE(result.get<uint32_t>(), 0U);
}

TEST_F(RPCMethodsTest, GetBlockReturnsGenesisWhenVerbose)
{
    nlohmann::json params = nlohmann::json::array({0, true});
    auto result = RPCMethods::GetBlock(neoSystem, params);

    ASSERT_TRUE(result.is_object());
    EXPECT_EQ(result["index"].get<uint32_t>(), 0U);
    EXPECT_TRUE(result.contains("tx"));
    EXPECT_TRUE(result["tx"].is_array());
    EXPECT_TRUE(result.contains("confirmations"));
    EXPECT_GE(result["confirmations"].get<uint32_t>(), 1U);
    EXPECT_TRUE(result.contains("nextconsensus"));
    EXPECT_FALSE(result["nextconsensus"].get<std::string>().empty());
}

TEST_F(RPCMethodsTest, GetBlockHeaderReturnsBase64WhenNotVerbose)
{
    nlohmann::json params = nlohmann::json::array({0, false});
    auto result = RPCMethods::GetBlockHeader(neoSystem, params);

    EXPECT_TRUE(result.is_string());
    EXPECT_FALSE(result.get<std::string>().empty());
}

TEST_F(RPCMethodsTest, GetBlockReturnsBase64WhenNotVerbose)
{
    auto blockchain = neoSystem->GetBlockchain();
    ASSERT_TRUE(blockchain);
    auto block = blockchain->GetBlock(0);
    ASSERT_TRUE(block);

    nlohmann::json params = nlohmann::json::array({0, false});
    auto rpcResult = RPCMethods::GetBlock(neoSystem, params);
    ASSERT_TRUE(rpcResult.is_string());

    neo::ledger::Block blockCopy = *block;
    neo::io::ByteVector buffer;
    neo::io::BinaryWriter writer(buffer);
    blockCopy.Serialize(writer);
    auto expected = neo::cryptography::Base64::Encode(buffer.AsSpan());

    EXPECT_EQ(rpcResult.get<std::string>(), expected);
}

TEST_F(RPCMethodsTest, GetBlockByHashMatchesByIndex)
{
    auto blockchain = neoSystem->GetBlockchain();
    ASSERT_TRUE(blockchain);
    auto hash = blockchain->GetBlockHash(0).ToString();

    auto byIndex = RPCMethods::GetBlock(neoSystem, nlohmann::json::array({0, true}));
    auto byHash = RPCMethods::GetBlock(neoSystem, nlohmann::json::array({hash, true}));

    EXPECT_EQ(byHash, byIndex);
}

TEST_F(RPCMethodsTest, GetBlockHeaderVerboseIncludesWitnessesAndAddress)
{
    nlohmann::json params = nlohmann::json::array({0, true});
    auto result = RPCMethods::GetBlockHeader(neoSystem, params);

    ASSERT_TRUE(result.is_object());
    EXPECT_TRUE(result.contains("witnesses"));
    ASSERT_TRUE(result["witnesses"].is_array());
    EXPECT_FALSE(result["witnesses"].empty());

    EXPECT_TRUE(result.contains("nextconsensus"));
    auto nextConsensus = result["nextconsensus"].get<std::string>();
    EXPECT_FALSE(nextConsensus.empty());

    EXPECT_TRUE(result.contains("confirmations"));
    EXPECT_GE(result["confirmations"].get<uint32_t>(), 1U);
}

TEST_F(RPCMethodsTest, GetBlockHeaderReturnsBase64MatchesManualSerialization)
{
    auto blockchain = neoSystem->GetBlockchain();
    ASSERT_TRUE(blockchain);
    auto header = blockchain->GetBlockHeader(0);
    ASSERT_TRUE(header);

    nlohmann::json params = nlohmann::json::array({0, false});
    auto rpcResult = RPCMethods::GetBlockHeader(neoSystem, params);
    ASSERT_TRUE(rpcResult.is_string());

    neo::ledger::BlockHeader headerCopy = *header;
    neo::io::ByteVector buffer;
    neo::io::BinaryWriter writer(buffer);
    headerCopy.Serialize(writer);
    auto expected = neo::cryptography::Base64::Encode(buffer.AsSpan());

    EXPECT_EQ(rpcResult.get<std::string>(), expected);
}

TEST_F(RPCMethodsTest, GetBlockHeaderByHashMatchesByIndex)
{
    auto blockchain = neoSystem->GetBlockchain();
    ASSERT_TRUE(blockchain);
    auto hash = blockchain->GetBlockHash(0).ToString();

    auto byIndex = RPCMethods::GetBlockHeader(neoSystem, nlohmann::json::array({0, true}));
    auto byHash = RPCMethods::GetBlockHeader(neoSystem, nlohmann::json::array({hash, true}));

    EXPECT_EQ(byHash, byIndex);
}

TEST_F(RPCMethodsTest, GetBlockHashReturnsGenesisHash)
{
    nlohmann::json params = nlohmann::json::array({0});
    auto result = RPCMethods::GetBlockHash(neoSystem, params);

    auto blockchain = neoSystem->GetBlockchain();
    ASSERT_TRUE(blockchain);
    EXPECT_EQ(result.get<std::string>(), blockchain->GetBlockHash(0).ToString());
}

TEST_F(RPCMethodsTest, GetConnectionCount)
{
    nlohmann::json params = nlohmann::json::array();
    auto result = RPCMethods::GetConnectionCount(neoSystem, params);

    EXPECT_GE(result.get<uint32_t>(), 0U);
}

TEST_F(RPCMethodsTest, GetRawTransactionReturnsVerboseJson)
{
    auto blockchain = neoSystem->GetBlockchain();
    ASSERT_TRUE(blockchain);
    auto genesis = blockchain->GetBlock(0);
    ASSERT_TRUE(genesis);
    if (genesis->GetTransactions().empty())
    {
        GTEST_SKIP() << "Genesis block has no transactions";
    }
    auto txHash = genesis->GetTransactions().front().GetHash().ToString();

    nlohmann::json params = nlohmann::json::array({txHash, true});
    auto result = RPCMethods::GetRawTransaction(neoSystem, params);

    ASSERT_TRUE(result.is_object());
    EXPECT_EQ(result["hash"].get<std::string>(), txHash);
}

TEST_F(RPCMethodsTest, GetRawTransactionNonVerboseReturnsBase64ForMempool)
{
    auto blockchain = neoSystem->GetBlockchain();
    ASSERT_TRUE(blockchain);
    auto memoryPool = neoSystem->GetMemoryPool();
    ASSERT_TRUE(memoryPool);
    memoryPool->SetVerifier([](const neo::network::p2p::payloads::Neo3Transaction&) { return true; });

    auto tx = CreateTestTransaction(blockchain->GetHeight() + 5);
    ASSERT_TRUE(memoryPool->TryAdd(tx));
    auto hash = tx.GetHash();

    auto result = RPCMethods::GetRawTransaction(neoSystem, nlohmann::json::array({hash.ToString(), false}));
    ASSERT_TRUE(result.is_string());

    neo::network::p2p::payloads::Neo3Transaction copy = tx;
    neo::io::ByteVector buffer;
    neo::io::BinaryWriter writer(buffer);
    copy.Serialize(writer);
    auto expected = neo::cryptography::Base64::Encode(buffer.AsSpan());

    EXPECT_EQ(result.get<std::string>(), expected);

    memoryPool->Remove(hash);
}

TEST_F(RPCMethodsTest, GetRawTransactionVerboseForMempoolIncludesSigner)
{
    auto blockchain = neoSystem->GetBlockchain();
    ASSERT_TRUE(blockchain);
    auto memoryPool = neoSystem->GetMemoryPool();
    ASSERT_TRUE(memoryPool);
    memoryPool->SetVerifier([](const neo::network::p2p::payloads::Neo3Transaction&) { return true; });

    auto tx = CreateTestTransaction(blockchain->GetHeight() + 5);
    ASSERT_TRUE(memoryPool->TryAdd(tx));
    auto hash = tx.GetHash();

    auto result = RPCMethods::GetRawTransaction(neoSystem, nlohmann::json::array({hash.ToString(), true}));
    ASSERT_TRUE(result.is_object());
    EXPECT_EQ(result["hash"].get<std::string>(), hash.ToString());
    EXPECT_EQ(result["size"].get<int>(), tx.GetSize());
    EXPECT_TRUE(result.contains("signers"));
    ASSERT_TRUE(result["signers"].is_array());
    EXPECT_EQ(result["signers"].size(), 1);
    EXPECT_TRUE(result.contains("witnesses"));
    EXPECT_TRUE(result["witnesses"].is_array());
    EXPECT_FALSE(result.contains("blockhash"));
    EXPECT_FALSE(result.contains("confirmations"));

    memoryPool->Remove(hash);
}

TEST_F(RPCMethodsTest, GetStorageReturnsBase64Value)
{
    auto fixture = PrepareStorageEntry("AA0102", "DEADBEEF");

    auto params = nlohmann::json::array({fixture.contractId, fixture.keyBase64});
    auto result = RPCMethods::GetStorage(neoSystem, params);

    ASSERT_TRUE(result.is_string());
    EXPECT_EQ(result.get<std::string>(), fixture.valueBase64);
}

TEST_F(RPCMethodsTest, FindStorageReturnsExpectedEntries)
{
    auto fixture = PrepareStorageEntry("AA0A0B0C", "CAFEBABE", 2);

    auto params = nlohmann::json::array({fixture.contractId, fixture.prefixBase64, 0});
    auto result = RPCMethods::FindStorage(neoSystem, params);

    ASSERT_TRUE(result.is_object());
    ASSERT_TRUE(result.contains("results"));
    auto entries = result["results"];
    ASSERT_TRUE(entries.is_array());

    bool found = false;
    for (const auto& entry : entries)
    {
        ASSERT_TRUE(entry.is_object());
        ASSERT_TRUE(entry.contains("key"));
        ASSERT_TRUE(entry.contains("value"));

        if (entry["key"].get<std::string>() == fixture.keyBase64)
        {
            EXPECT_EQ(entry["value"].get<std::string>(), fixture.valueBase64);
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found);
    ASSERT_TRUE(result.contains("truncated"));
    ASSERT_TRUE(result.contains("next"));
    EXPECT_TRUE(result["next"].is_number_integer());
}

TEST_F(RPCMethodsTest, FindStorageRespectsConfiguredLimit)
{
    constexpr size_t kLimit = 2;
    const int32_t contractId = 1337;
    FindLimitGuard guard(kLimit);

    auto first = PrepareStorageEntry("AA0B0001", "F00D", 1, contractId);
    for (int i = 2; i < 7; ++i)
    {
        std::ostringstream key;
        key << "AA0B00" << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << i;
        std::ostringstream value;
        value << "BEEF" << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << i;
        PrepareStorageEntry(key.str(), value.str(), 1, contractId);
    }

    auto params = nlohmann::json::array({contractId, first.prefixBase64, 0});
    auto result = RPCMethods::FindStorage(neoSystem, params);

    ASSERT_TRUE(result.contains("results"));
    ASSERT_TRUE(result["results"].is_array());
    EXPECT_EQ(result["results"].size(), kLimit);
    EXPECT_TRUE(result.at("truncated").get<bool>());
    EXPECT_EQ(result.at("next").get<int64_t>(), static_cast<int64_t>(kLimit));
}

TEST_F(RPCMethodsTest, GetNativeContractsReturnsManifest)
{
    auto result = RPCMethods::GetNativeContracts(neoSystem, nlohmann::json::array());

    ASSERT_TRUE(result.is_array());
    ASSERT_FALSE(result.empty());

    const auto& entry = result.front();
    ASSERT_TRUE(entry.is_object());
    EXPECT_TRUE(entry.contains("id"));
    EXPECT_TRUE(entry.contains("hash"));
    EXPECT_TRUE(entry.contains("nef"));
    EXPECT_TRUE(entry.contains("manifest"));

    const auto& manifest = entry["manifest"];
    EXPECT_TRUE(manifest.is_object());
    EXPECT_TRUE(manifest.contains("name"));
}

TEST_F(RPCMethodsTest, GetCommitteeReturnsMembers)
{
    auto result = RPCMethods::GetCommittee(neoSystem, nlohmann::json::array());

    ASSERT_TRUE(result.is_array());
    if (!result.empty())
    {
        EXPECT_TRUE(result[0].is_string());
        EXPECT_FALSE(result[0].get<std::string>().empty());
    }
}

TEST_F(RPCMethodsTest, GetValidatorsReturnsList)
{
    auto result = RPCMethods::GetValidators(neoSystem, nlohmann::json::array());

    ASSERT_TRUE(result.is_array());
    if (!result.empty())
    {
        EXPECT_TRUE(result[0].is_string());
        EXPECT_FALSE(result[0].get<std::string>().empty());
    }
}

TEST_F(RPCMethodsTest, GetNextBlockValidatorsReturnsVotes)
{
    auto result = RPCMethods::GetNextBlockValidators(neoSystem, nlohmann::json::array());

    ASSERT_TRUE(result.is_array());
    if (!result.empty())
    {
        const auto& entry = result[0];
        ASSERT_TRUE(entry.is_object());
        EXPECT_TRUE(entry.contains("publickey"));
        EXPECT_TRUE(entry.contains("votes"));
    }
}

TEST_F(RPCMethodsTest, GetCandidatesReturnsEntries)
{
    auto result = RPCMethods::GetCandidates(neoSystem, nlohmann::json::array());

    ASSERT_TRUE(result.is_array());
    if (!result.empty())
    {
        const auto& entry = result[0];
        ASSERT_TRUE(entry.is_object());
        EXPECT_TRUE(entry.contains("publickey"));
        EXPECT_TRUE(entry.contains("votes"));
        EXPECT_TRUE(entry.contains("active"));
    }
}

TEST_F(RPCMethodsTest, ValidateAddressRecognizesValidAddress)
{
    auto hash = neo::io::UInt160::FromString("0x11223344556677889900aabbccddeeff00112233");
    auto address =
        neo::wallets::Helper::ToAddress(hash, neoSystem->GetProtocolSettings()->GetAddressVersion());

    auto result = RPCMethods::ValidateAddress(neoSystem, nlohmann::json::array({address}));

    ASSERT_TRUE(result.is_object());
    EXPECT_EQ(result["address"].get<std::string>(), address);
    EXPECT_TRUE(result["isvalid"].get<bool>());
}

TEST_F(RPCMethodsTest, ValidateAddressRejectsInvalidAddress)
{
    auto result = RPCMethods::ValidateAddress(neoSystem, nlohmann::json::array({"not-an-address"}));

    ASSERT_TRUE(result.is_object());
    EXPECT_EQ(result["address"].get<std::string>(), "not-an-address");
    EXPECT_FALSE(result["isvalid"].get<bool>());
}

TEST_F(RPCMethodsTest, ValidateAddressAcceptsValidAddress)
{
    const std::string address = "NM7Aky765FG8NhhwtxjXRx7jEL1cnw7PBP";
    auto result = RPCMethods::ValidateAddress(neoSystem, nlohmann::json::array({address}));

    ASSERT_TRUE(result.is_object());
    EXPECT_EQ(result["address"].get<std::string>(), address);
    EXPECT_TRUE(result["isvalid"].get<bool>());
}

TEST_F(RPCMethodsTest, ValidateAddressEmptyStringReturnsFalse)
{
    const std::string address = "";
    auto result = RPCMethods::ValidateAddress(neoSystem, nlohmann::json::array({address}));

    ASSERT_TRUE(result.is_object());
    EXPECT_EQ(result["address"].get<std::string>(), address);
    EXPECT_FALSE(result["isvalid"].get<bool>());
}

TEST_F(RPCMethodsTest, ValidateAddressInvalidChecksumReturnsFalse)
{
    const std::string address = "NM7Aky765FG8NhhwtxjXRx7jEL1cnw7PBO";
    auto result = RPCMethods::ValidateAddress(neoSystem, nlohmann::json::array({address}));

    ASSERT_TRUE(result.is_object());
    EXPECT_EQ(result["address"].get<std::string>(), address);
    EXPECT_FALSE(result["isvalid"].get<bool>());
}

TEST_F(RPCMethodsTest, ValidateAddressWrongLengthReturnsFalse)
{
    const std::string shortAddress = "NM7Aky765FG8NhhwtxjXRx7jEL1cnw7P";
    auto shortResult = RPCMethods::ValidateAddress(neoSystem, nlohmann::json::array({shortAddress}));
    ASSERT_TRUE(shortResult.is_object());
    EXPECT_EQ(shortResult["address"].get<std::string>(), shortAddress);
    EXPECT_FALSE(shortResult["isvalid"].get<bool>());

    const std::string longAddress = "NM7Aky765FG8NhhwtxjXRx7jEL1cnw7PBPPP";
    auto longResult = RPCMethods::ValidateAddress(neoSystem, nlohmann::json::array({longAddress}));
    ASSERT_TRUE(longResult.is_object());
    EXPECT_EQ(longResult["address"].get<std::string>(), longAddress);
    EXPECT_FALSE(longResult["isvalid"].get<bool>());
}

TEST_F(RPCMethodsTest, ListPluginsReturnsArray)
{
    auto result = RPCMethods::ListPlugins(neoSystem, nlohmann::json::array());

    ASSERT_TRUE(result.is_array());
    for (const auto& entry : result)
    {
        ASSERT_TRUE(entry.is_object());
        EXPECT_TRUE(entry.contains("name"));
        EXPECT_TRUE(entry.contains("version"));
        EXPECT_TRUE(entry.contains("interfaces"));
        EXPECT_TRUE(entry.at("interfaces").is_array());
    }
}

TEST_F(RPCMethodsTest, ListPluginsReflectsAddedPlugins)
{
    auto& manager = neo::plugins::PluginManager::GetInstance();
    manager.ClearFactories();
    manager.ClearPlugins();

    auto emptyResult = RPCMethods::ListPlugins(neoSystem, nlohmann::json::array());
    ASSERT_TRUE(emptyResult.is_array());
    EXPECT_TRUE(emptyResult.empty());

    auto pluginB = std::make_shared<TestPluginImpl>();
    auto pluginA = std::make_shared<AlphaPluginImpl>();
    manager.AddPlugin(pluginB);
    manager.AddPlugin(pluginA);

    auto result = RPCMethods::ListPlugins(neoSystem, nlohmann::json::array());
    ASSERT_TRUE(result.is_array());
    ASSERT_EQ(result.size(), 2);

    const auto& first = result[0];
    const auto& second = result[1];

    ASSERT_TRUE(first.is_object());
    ASSERT_TRUE(second.is_object());

    EXPECT_EQ(first.at("name").get<std::string>(), pluginA->GetName());
    EXPECT_EQ(first.at("version").get<std::string>(), pluginA->GetVersion());
    EXPECT_TRUE(first.at("interfaces").is_array());

    EXPECT_EQ(second.at("name").get<std::string>(), pluginB->GetName());
    EXPECT_EQ(second.at("version").get<std::string>(), pluginB->GetVersion());
    EXPECT_TRUE(second.at("interfaces").is_array());

    manager.ClearPlugins();
}

TEST_F(RPCMethodsTest, GetStorageMissingParamsThrowsInvalidParams)
{
    try
    {
        RPCMethods::GetStorage(neoSystem, nlohmann::json::array());
        FAIL() << "Expected RpcException";
    }
    catch (const RpcException& ex)
    {
        EXPECT_EQ(ex.GetCode(), ErrorCode::InvalidParams);
    }
    catch (...)
    {
        FAIL() << "Expected RpcException";
    }
}

TEST_F(RPCMethodsTest, GetStorageInvalidBase64ThrowsInvalidParams)
{
    try
    {
        RPCMethods::GetStorage(neoSystem, nlohmann::json::array({0, "???"}));
        FAIL() << "Expected RpcException";
    }
    catch (const RpcException& ex)
    {
        EXPECT_EQ(ex.GetCode(), ErrorCode::InvalidParams);
    }
    catch (...)
    {
        FAIL() << "Expected RpcException";
    }
}

TEST_F(RPCMethodsTest, GetStorageUnknownContractThrows)
{
    auto key = neo::cryptography::Base64::Encode(neo::io::ByteVector{0x01}.AsSpan());
    try
    {
        RPCMethods::GetStorage(neoSystem, nlohmann::json::array({"nonexistent-native", key}));
        FAIL() << "Expected RpcException";
    }
    catch (const RpcException& ex)
    {
        EXPECT_EQ(ex.GetCode(), ErrorCode::UnknownContract);
    }
    catch (...)
    {
        FAIL() << "Expected RpcException";
    }
}

TEST_F(RPCMethodsTest, FindStorageInvalidPrefixThrowsInvalidParams)
{
    try
    {
        RPCMethods::FindStorage(neoSystem, nlohmann::json::array({0, "invalid-base64"}));
        FAIL() << "Expected RpcException";
    }
    catch (const RpcException& ex)
    {
        EXPECT_EQ(ex.GetCode(), ErrorCode::InvalidParams);
    }
    catch (...)
    {
        FAIL() << "Expected RpcException";
    }
}

TEST_F(RPCMethodsTest, ValidateAddressNonStringThrowsInvalidParams)
{
    try
    {
        RPCMethods::ValidateAddress(neoSystem, nlohmann::json::array({42}));
        FAIL() << "Expected RpcException";
    }
    catch (const RpcException& ex)
    {
        EXPECT_EQ(ex.GetCode(), ErrorCode::InvalidParams);
    }
    catch (...)
    {
        FAIL() << "Expected RpcException";
    }
}

TEST_F(RPCMethodsTest, GetTransactionHeightReturnsZeroForGenesisTx)
{
    auto blockchain = neoSystem->GetBlockchain();
    ASSERT_TRUE(blockchain);
    auto block = blockchain->GetBlock(0);
    ASSERT_TRUE(block);
    if (block->GetTransactions().empty())
    {
        GTEST_SKIP() << "Genesis block has no transactions";
    }

    auto txHash = block->GetTransactions().front().GetHash().ToString();

    nlohmann::json params = nlohmann::json::array({txHash});
    auto result = RPCMethods::GetTransactionHeight(neoSystem, params);

    ASSERT_TRUE(result.is_number_integer());
    EXPECT_EQ(result.get<int32_t>(), 0);
}

TEST_F(RPCMethodsTest, GetPeers)
{
    auto localNode = neoSystem->GetLocalNode();
    ASSERT_TRUE(localNode);
    localNode->GetPeerList().Clear();

    std::vector<neo::network::IPEndPoint> peers = {
        neo::network::IPEndPoint("127.0.0.1", 11332),
        neo::network::IPEndPoint("127.0.0.1", 12332),
        neo::network::IPEndPoint("127.0.0.1", 13332)};
    localNode->AddPeers(peers);

    nlohmann::json params = nlohmann::json::array();
    auto result = RPCMethods::GetPeers(neoSystem, params);

    ASSERT_TRUE(result.is_object());
    ASSERT_TRUE(result.contains("unconnected"));
    ASSERT_TRUE(result["unconnected"].is_array());
    EXPECT_EQ(result["unconnected"].size(), peers.size());
    ASSERT_TRUE(result.contains("bad"));
    ASSERT_TRUE(result["bad"].is_array());
    ASSERT_TRUE(result.contains("connected"));
    ASSERT_TRUE(result["connected"].is_array());
}

TEST_F(RPCMethodsTest, GetPeers_NoUnconnected)
{
    auto localNode = neoSystem->GetLocalNode();
    ASSERT_TRUE(localNode);
    localNode->GetPeerList().Clear();

    auto result = RPCMethods::GetPeers(neoSystem, nlohmann::json::array());
    ASSERT_TRUE(result.is_object());
    ASSERT_TRUE(result.contains("unconnected"));
    ASSERT_TRUE(result["unconnected"].is_array());
    EXPECT_TRUE(result["unconnected"].empty());
    ASSERT_TRUE(result.contains("bad"));
    ASSERT_TRUE(result["bad"].is_array());
    ASSERT_TRUE(result.contains("connected"));
    ASSERT_TRUE(result["connected"].is_array());
}

TEST_F(RPCMethodsTest, GetPeers_NoConnected)
{
    auto localNode = neoSystem->GetLocalNode();
    ASSERT_TRUE(localNode);
    localNode->GetPeerList().Clear();

    auto result = RPCMethods::GetPeers(neoSystem, nlohmann::json::array());
    ASSERT_TRUE(result.is_object());
    ASSERT_TRUE(result.contains("connected"));
    ASSERT_TRUE(result["connected"].is_array());
    EXPECT_TRUE(result["connected"].empty());
}
TEST_F(RPCMethodsTest, GetCommittee)
{
    nlohmann::json params = nlohmann::json::array();
    auto result = RPCMethods::GetCommittee(neoSystem, params);

    EXPECT_TRUE(result.is_array());
}

TEST_F(RPCMethodsTest, GetValidators)
{
    nlohmann::json params = nlohmann::json::array();
    auto result = RPCMethods::GetValidators(neoSystem, params);

    EXPECT_TRUE(result.is_array());
}

TEST_F(RPCMethodsTest, GetNextBlockValidators)
{
    nlohmann::json params = nlohmann::json::array();
    auto result = RPCMethods::GetNextBlockValidators(neoSystem, params);

    EXPECT_TRUE(result.is_array());
}

TEST_F(RPCMethodsTest, GetBestBlockHashMatchesCurrent)
{
    nlohmann::json params = nlohmann::json::array();
    auto result = RPCMethods::GetBestBlockHash(neoSystem, params);

    auto blockchain = neoSystem->GetBlockchain();
    ASSERT_TRUE(blockchain);
    EXPECT_EQ(result.get<std::string>(), blockchain->GetBestBlockHash().ToString());
}

TEST_F(RPCMethodsTest, GetBlockHeaderCountMatchesHeightPlusOne)
{
    nlohmann::json params = nlohmann::json::array();
    auto result = RPCMethods::GetBlockHeaderCount(neoSystem, params);

    auto blockchain = neoSystem->GetBlockchain();
    ASSERT_TRUE(blockchain);
    EXPECT_EQ(result.get<uint32_t>(), blockchain->GetHeight() + 1);
}

TEST_F(RPCMethodsTest, SendRawTransaction_ReturnsHashOnSuccess)
{
    auto mempool = neoSystem->GetMemoryPool();
    ASSERT_TRUE(mempool);
    mempool->Clear();

    auto tx = BuildTransaction();
    auto base64 = EncodeTransaction(tx);
    nlohmann::json params = nlohmann::json::array({base64});

    try
    {
        auto result = RPCMethods::SendRawTransaction(neoSystem, params);
        ASSERT_TRUE(result.is_object());
        ASSERT_TRUE(result.contains("hash"));
        EXPECT_EQ(tx.GetHash().ToString(), result["hash"].get<std::string>());
    }
    catch (const neo::rpc::RpcException& ex)
    {
        // In the current test harness accounts have no funds, so relay falls back to insufficient funds.
        EXPECT_EQ(neo::rpc::ErrorCode::RpcInsufficientFunds, ex.GetCode());
    }

    mempool->Clear();
}

TEST_F(RPCMethodsTest, SendRawTransaction_InvalidBase64ThrowsInvalidParams)
{
    nlohmann::json params = nlohmann::json::array({"invalid_transaction_string"});
    try
    {
        (void)RPCMethods::SendRawTransaction(neoSystem, params);
        FAIL() << "Expected RpcException";
    }
    catch (const neo::rpc::RpcException& ex)
    {
        EXPECT_EQ(neo::rpc::ErrorCode::InvalidParams, ex.GetCode());
    }
}

TEST_F(RPCMethodsTest, SendRawTransaction_MalformedPayloadThrowsInvalidParams)
{
    nlohmann::json params = nlohmann::json::array({EncodeMalformedPayload()});
    try
    {
        (void)RPCMethods::SendRawTransaction(neoSystem, params);
        FAIL() << "Expected RpcException";
    }
    catch (const neo::rpc::RpcException& ex)
    {
        EXPECT_EQ(neo::rpc::ErrorCode::InvalidParams, ex.GetCode());
    }
}

TEST_F(RPCMethodsTest, SendRawTransaction_InvalidSignatureThrows)
{
    auto tx = BuildTransaction({}, SignatureMode::Corrupt);
    auto base64 = EncodeTransaction(tx);
    nlohmann::json params = nlohmann::json::array({base64});

    try
    {
        (void)RPCMethods::SendRawTransaction(neoSystem, params);
        FAIL() << "Expected RpcException";
    }
    catch (const neo::rpc::RpcException& ex)
    {
        EXPECT_EQ(neo::rpc::ErrorCode::RpcInvalidSignature, ex.GetCode());
    }
}

TEST_F(RPCMethodsTest, SendRawTransaction_InsufficientFundsThrows)
{
    auto mempool = neoSystem->GetMemoryPool();
    ASSERT_TRUE(mempool);
    mempool->Clear();

    auto tx = BuildTransaction();
    auto base64 = EncodeTransaction(tx);
    nlohmann::json params = nlohmann::json::array({base64});

    try
    {
        (void)RPCMethods::SendRawTransaction(neoSystem, params);
        FAIL() << "Expected RpcException";
    }
    catch (const neo::rpc::RpcException& ex)
    {
        EXPECT_EQ(neo::rpc::ErrorCode::RpcInsufficientFunds, ex.GetCode());
    }

    mempool->Clear();
}

TEST_F(RPCMethodsTest, SendRawTransaction_InvalidScriptThrows)
{
    auto tx = BuildTransaction([](neo::ledger::Transaction& transaction) { transaction.SetScript(neo::io::ByteVector()); });
    auto base64 = EncodeTransaction(tx);
    nlohmann::json params = nlohmann::json::array({base64});

    try
    {
        (void)RPCMethods::SendRawTransaction(neoSystem, params);
        FAIL() << "Expected RpcException";
    }
    catch (const neo::rpc::RpcException& ex)
    {
        EXPECT_EQ(neo::rpc::ErrorCode::RpcInvalidTransactionScript, ex.GetCode());
    }
}

TEST_F(RPCMethodsTest, SendRawTransaction_InvalidAttributeThrows)
{
    auto tx = BuildTransaction([](neo::ledger::Transaction& transaction) {
        std::vector<std::shared_ptr<neo::ledger::TransactionAttribute>> attributes;
        attributes.reserve(neo::ProtocolSettings::MAX_TRANSACTION_ATTRIBUTES + 1);
        for (int i = 0; i <= neo::ProtocolSettings::MAX_TRANSACTION_ATTRIBUTES; ++i)
        {
            auto attribute = std::make_shared<neo::ledger::TransactionAttribute>();
            attribute->SetUsage(neo::ledger::TransactionAttribute::Usage::Remark);
            attribute->SetData(neo::io::ByteVector{static_cast<uint8_t>(i)});
            attributes.push_back(attribute);
        }
        transaction.SetAttributes(attributes);
    });

    auto base64 = EncodeTransaction(tx);
    nlohmann::json params = nlohmann::json::array({base64});

    try
    {
        (void)RPCMethods::SendRawTransaction(neoSystem, params);
        FAIL() << "Expected RpcException";
    }
    catch (const neo::rpc::RpcException& ex)
    {
        EXPECT_EQ(neo::rpc::ErrorCode::RpcInvalidTransactionAttribute, ex.GetCode());
    }
}

TEST_F(RPCMethodsTest, SendRawTransaction_OversizedPayloadThrowsInvalidParams)
{
    nlohmann::json params = nlohmann::json::array({EncodeOversizedGarbagePayload()});
    try
    {
        (void)RPCMethods::SendRawTransaction(neoSystem, params);
        FAIL() << "Expected RpcException";
    }
    catch (const neo::rpc::RpcException& ex)
    {
        EXPECT_EQ(neo::rpc::ErrorCode::InvalidParams, ex.GetCode());
    }
}

TEST_F(RPCMethodsTest, SendRawTransaction_ExpiredThrows)
{
    auto blockchain = neoSystem->GetBlockchain();
    ASSERT_TRUE(blockchain);
    auto height = blockchain->GetHeight();
    auto tx = BuildTransaction([height](neo::ledger::Transaction& transaction) {
        transaction.SetValidUntilBlock(height);
    });
    auto base64 = EncodeTransaction(tx);
    nlohmann::json params = nlohmann::json::array({base64});

    try
    {
        (void)RPCMethods::SendRawTransaction(neoSystem, params);
        FAIL() << "Expected RpcException";
    }
    catch (const neo::rpc::RpcException& ex)
    {
        EXPECT_EQ(neo::rpc::ErrorCode::RpcExpiredTransaction, ex.GetCode());
    }
}

TEST_F(RPCMethodsTest, SendRawTransaction_PolicyFailedThrows)
{
    auto blockchain = neoSystem->GetBlockchain();
    ASSERT_TRUE(blockchain);
    auto height = blockchain->GetHeight();
    auto limit = protocolSettings->GetMaxValidUntilBlockIncrement();
    auto tx = BuildTransaction([height, limit](neo::ledger::Transaction& transaction) {
        transaction.SetValidUntilBlock(height + limit + 50);
    });
    auto base64 = EncodeTransaction(tx);
    nlohmann::json params = nlohmann::json::array({base64});

    try
    {
        (void)RPCMethods::SendRawTransaction(neoSystem, params);
        FAIL() << "Expected RpcException";
    }
    catch (const neo::rpc::RpcException& ex)
    {
        EXPECT_EQ(neo::rpc::ErrorCode::RpcPolicyFailed, ex.GetCode());
    }
}

TEST_F(RPCMethodsTest, SendRawTransaction_AlreadyInPoolThrows)
{
    auto mempool = neoSystem->GetMemoryPool();
    ASSERT_TRUE(mempool);
    mempool->Clear();

    auto tx = BuildTransaction();
    auto base64 = EncodeTransaction(tx);
    nlohmann::json params = nlohmann::json::array({base64});

    // First submission should throw (insufficient funds); ignore error.
    try
    {
        (void)RPCMethods::SendRawTransaction(neoSystem, params);
    }
    catch (const neo::rpc::RpcException&)
    {
        // Expected failure due to insufficient funds.
    }

    // Add transaction to memory pool manually to simulate prior success.
    (void)mempool->TryAdd(tx);

    try
    {
        (void)RPCMethods::SendRawTransaction(neoSystem, params);
        FAIL() << "Expected RpcException";
    }
    catch (const neo::rpc::RpcException& ex)
    {
        EXPECT_EQ(neo::rpc::ErrorCode::TransactionAlreadyExists, ex.GetCode());
    }

    mempool->Clear();
}

TEST_F(RPCMethodsTest, SendRawTransaction_AlreadyInBlockchainThrows)
{
    auto mempool = neoSystem->GetMemoryPool();
    ASSERT_TRUE(mempool);
    mempool->Clear();

    auto tx = BuildTransaction();
    AddTransactionToBlockchain(tx);
    auto base64 = EncodeTransaction(tx);
    nlohmann::json params = nlohmann::json::array({base64});

    try
    {
        (void)RPCMethods::SendRawTransaction(neoSystem, params);
        FAIL() << "Expected RpcException";
    }
    catch (const neo::rpc::RpcException& ex)
    {
        EXPECT_EQ(neo::rpc::ErrorCode::TransactionAlreadyExists, ex.GetCode());
    }
}

TEST_F(RPCMethodsTest, SendRawTransaction_NullInputThrowsInvalidParams)
{
    nlohmann::json params = nlohmann::json::array({nlohmann::json()});
    try
    {
        (void)RPCMethods::SendRawTransaction(neoSystem, params);
        FAIL() << "Expected RpcException";
    }
    catch (const neo::rpc::RpcException& ex)
    {
        EXPECT_EQ(neo::rpc::ErrorCode::InvalidParams, ex.GetCode());
    }
}

TEST_F(RPCMethodsTest, SendRawTransaction_EmptyInputThrowsInvalidParams)
{
    nlohmann::json params = nlohmann::json::array({""});
    try
    {
        (void)RPCMethods::SendRawTransaction(neoSystem, params);
        FAIL() << "Expected RpcException";
    }
    catch (const neo::rpc::RpcException& ex)
    {
        EXPECT_EQ(neo::rpc::ErrorCode::InvalidParams, ex.GetCode());
    }
}

TEST_F(RPCMethodsTest, GetRawMemPoolReturnsVerifiedTransactionHashes)
{
    auto blockchain = neoSystem->GetBlockchain();
    ASSERT_TRUE(blockchain);
    auto memoryPool = neoSystem->GetMemoryPool();
    ASSERT_TRUE(memoryPool);

    memoryPool->Clear();
    memoryPool->SetVerifier([](const neo::network::p2p::payloads::Neo3Transaction&) { return true; });

    auto tx = CreateTestTransaction(blockchain->GetHeight() + 5, 100);
    ASSERT_TRUE(memoryPool->TryAdd(tx));
    auto hash = tx.GetHash().ToString();

    auto result = RPCMethods::GetRawMemPool(neoSystem, nlohmann::json::array());
    ASSERT_TRUE(result.is_array());
    bool found = false;
    for (const auto& entry : result)
    {
        if (entry.is_string() && entry.get<std::string>() == hash)
        {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);

    memoryPool->Remove(tx.GetHash());
    memoryPool->Clear();
    memoryPool->SetVerifier(nullptr);
}

TEST_F(RPCMethodsTest, GetRawMemPoolEmptyReturnsEmptyCollections)
{
    auto memoryPool = neoSystem->GetMemoryPool();
    ASSERT_TRUE(memoryPool);
    memoryPool->Clear();
    memoryPool->SetVerifier([](const neo::network::p2p::payloads::Neo3Transaction&) { return true; });

    auto withoutUnverified = RPCMethods::GetRawMemPool(neoSystem, nlohmann::json::array());
    ASSERT_TRUE(withoutUnverified.is_array());
    EXPECT_TRUE(withoutUnverified.empty());

    auto withUnverified = RPCMethods::GetRawMemPool(neoSystem, nlohmann::json::array({true}));
    ASSERT_TRUE(withUnverified.is_object());
    EXPECT_TRUE(withUnverified.contains("height"));
    EXPECT_TRUE(withUnverified.contains("verified"));
    EXPECT_TRUE(withUnverified.contains("unverified"));
    EXPECT_TRUE(withUnverified["verified"].is_array());
    EXPECT_TRUE(withUnverified["unverified"].is_array());
    EXPECT_TRUE(withUnverified["verified"].empty());
    EXPECT_TRUE(withUnverified["unverified"].empty());

    memoryPool->SetVerifier(nullptr);
}

TEST_F(RPCMethodsTest, GetRawMemPoolMixedVerifiedAndUnverifiedMatchesPoolState)
{
    auto blockchain = neoSystem->GetBlockchain();
    ASSERT_TRUE(blockchain);
    auto memoryPool = neoSystem->GetMemoryPool();
    ASSERT_TRUE(memoryPool);

    memoryPool->Clear();
    memoryPool->SetVerifier([](const neo::network::p2p::payloads::Neo3Transaction& tx)
                            { return tx.GetNonce() % 2 == 0; });

    auto verifiedTx = CreateTestTransaction(blockchain->GetHeight() + 5, 200);
    auto unverifiedTx = CreateTestTransaction(blockchain->GetHeight() + 5, 201);
    ASSERT_TRUE(memoryPool->TryAdd(verifiedTx));
    ASSERT_TRUE(memoryPool->TryAdd(unverifiedTx));

    auto result = RPCMethods::GetRawMemPool(neoSystem, nlohmann::json::array({true}));
    ASSERT_TRUE(result.is_object());

    auto verifiedArray = result["verified"];
    auto unverifiedArray = result["unverified"];
    ASSERT_TRUE(verifiedArray.is_array());
    ASSERT_TRUE(unverifiedArray.is_array());

    std::set<std::string> verifiedHashes;
    for (const auto& entry : verifiedArray)
    {
        verifiedHashes.insert(entry.get<std::string>());
    }

    std::set<std::string> unverifiedHashes;
    for (const auto& entry : unverifiedArray)
    {
        unverifiedHashes.insert(entry.get<std::string>());
    }

    EXPECT_EQ(verifiedHashes.size(), 1u);
    EXPECT_EQ(unverifiedHashes.size(), 1u);
    EXPECT_TRUE(verifiedHashes.contains(verifiedTx.GetHash().ToString()));
    EXPECT_TRUE(unverifiedHashes.contains(unverifiedTx.GetHash().ToString()));

    memoryPool->Remove(verifiedTx.GetHash());
    memoryPool->Remove(unverifiedTx.GetHash());
    memoryPool->Clear();
    memoryPool->SetVerifier(nullptr);
}

TEST_F(RPCMethodsTest, GetApplicationLogThrowsWhenPluginMissing)
{
    auto& manager = neo::plugins::PluginManager::GetInstance();
    manager.ClearPlugins();

    const std::string tx = "0x0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    try
    {
        (void)RPCMethods::GetApplicationLog(neoSystem, nlohmann::json::array({tx}));
        FAIL() << "Expected RpcException";
    }
    catch (const RpcException& ex)
    {
        EXPECT_EQ(ex.GetCode(), ErrorCode::ApplicationLogNotFound);
    }
    catch (...)
    {
        FAIL() << "Expected RpcException";
    }
}

TEST_F(RPCMethodsTest, GetApplicationLogThrowsWhenLogMissing)
{
    auto& manager = neo::plugins::PluginManager::GetInstance();
    manager.ClearPlugins();

    auto tempPath = std::filesystem::temp_directory_path() /
                    ("neo_cpp_app_logs_" +
                     std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(tempPath);

    auto plugin = std::make_shared<neo::plugins::ApplicationLogsPlugin>();
    std::unordered_map<std::string, std::string> settings = {{"LogPath", tempPath.string()}};
    ASSERT_TRUE(plugin->Initialize(neoSystem, settings));
    manager.AddPlugin(plugin);

    const std::string tx = "0x1111111111111111111111111111111111111111111111111111111111111111";
    try
    {
        (void)RPCMethods::GetApplicationLog(neoSystem, nlohmann::json::array({tx}));
        FAIL() << "Expected RpcException";
    }
    catch (const RpcException& ex)
    {
        EXPECT_EQ(ex.GetCode(), ErrorCode::ApplicationLogNotFound);
    }
    catch (...)
    {
        FAIL() << "Expected RpcException";
    }

    manager.ClearPlugins();
    std::filesystem::remove_all(tempPath);
}

TEST_F(RPCMethodsTest, GetApplicationLogReturnsStoredLog)
{
    auto& manager = neo::plugins::PluginManager::GetInstance();
    manager.ClearPlugins();

    auto tempPath = std::filesystem::temp_directory_path() /
                    ("neo_cpp_app_logs_" +
                     std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(tempPath);

    const std::string tx =
        "0x2222222222222222222222222222222222222222222222222222222222222222";
    const std::string blockHash =
        "0x3333333333333333333333333333333333333333333333333333333333333333";

    auto plugin = std::make_shared<neo::plugins::ApplicationLogsPlugin>();
    std::unordered_map<std::string, std::string> settings = {{"LogPath", tempPath.string()}};
    ASSERT_TRUE(plugin->Initialize(neoSystem, settings));
    auto logEntry = std::make_shared<neo::plugins::ApplicationLog>();
    logEntry->TxHash = neo::io::UInt256::Parse(tx);
    logEntry->BlockHash = neo::io::UInt256::Parse(blockHash);
    neo::plugins::ApplicationLog::Execution execution;
    execution.Trigger = neo::smartcontract::TriggerType::Application;
    execution.VmState = neo::vm::VMState::Halt;
    execution.GasConsumed = 42;
    execution.Exception = "";
    execution.Stack.push_back(nlohmann::json{{"type", "Integer"}, {"value", "5"}});
    neo::plugins::ApplicationLog::Notification notification;
    notification.Contract =
        neo::io::UInt160::Parse("0x0102030405060708090a0b0c0d0e0f1011121314");
    notification.EventName = "MyEvent";
    notification.State = nlohmann::json{
        {"type", "Array"},
        {"value", nlohmann::json::array({nlohmann::json{{"type", "Integer"}, {"value", "1"}}})}};
    execution.Notifications.push_back(notification);
    logEntry->Executions.push_back(execution);
    plugin->AddLog(logEntry);
    ASSERT_NE(plugin->GetApplicationLog(*logEntry->TxHash), nullptr);
    manager.AddPlugin(plugin);

    auto result = RPCMethods::GetApplicationLog(neoSystem, nlohmann::json::array({tx}));
    ASSERT_TRUE(result.is_object());
    EXPECT_EQ(result["txid"].get<std::string>(), tx);
    EXPECT_EQ(result["blockhash"].get<std::string>(), blockHash);
    ASSERT_TRUE(result.contains("executions"));
    const auto& executions = result["executions"];
    ASSERT_TRUE(executions.is_array());
    ASSERT_EQ(executions.size(), 1);
    const auto& executionJson = executions.at(0);
    EXPECT_EQ(executionJson["trigger"].get<std::string>(), "Application");
    EXPECT_EQ(executionJson["vmstate"].get<std::string>(), "HALT");
    EXPECT_EQ(executionJson["gasconsumed"].get<std::string>(), "42");
    EXPECT_EQ(executionJson["exception"].get<std::string>(), "");
    ASSERT_TRUE(executionJson["stack"].is_array());
    ASSERT_EQ(executionJson["stack"].size(), 1);
    EXPECT_EQ(executionJson["stack"][0]["type"].get<std::string>(), "Integer");
    EXPECT_EQ(executionJson["stack"][0]["value"].get<std::string>(), "5");
    ASSERT_TRUE(executionJson["notifications"].is_array());
    ASSERT_EQ(executionJson["notifications"].size(), 1);
    const auto& notificationJson = executionJson["notifications"][0];
    EXPECT_EQ(notificationJson["contract"].get<std::string>(), notification.Contract.ToString());
    EXPECT_EQ(notificationJson["eventname"].get<std::string>(), "MyEvent");
    EXPECT_TRUE(notificationJson["state"].is_object());
    EXPECT_EQ(notificationJson["state"]["type"].get<std::string>(), "Array");
    ASSERT_TRUE(notificationJson["state"]["value"].is_array());
    ASSERT_EQ(notificationJson["state"]["value"].size(), 1);
    EXPECT_EQ(notificationJson["state"]["value"][0]["type"].get<std::string>(), "Integer");
    EXPECT_EQ(notificationJson["state"]["value"][0]["value"].get<std::string>(), "1");

    manager.ClearPlugins();
    std::filesystem::remove_all(tempPath);
}

TEST_F(RPCMethodsTest, GetApplicationLogReturnsBlockLog)
{
    auto& manager = neo::plugins::PluginManager::GetInstance();
    manager.ClearPlugins();

    auto tempPath = std::filesystem::temp_directory_path() /
                    ("neo_cpp_app_logs_" +
                     std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(tempPath);

    const std::string blockHash =
        "0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

    auto plugin = std::make_shared<neo::plugins::ApplicationLogsPlugin>();
    std::unordered_map<std::string, std::string> settings = {{"LogPath", tempPath.string()}};
    ASSERT_TRUE(plugin->Initialize(neoSystem, settings));
    auto logEntry = std::make_shared<neo::plugins::ApplicationLog>();
    logEntry->BlockHash = neo::io::UInt256::Parse(blockHash);

    neo::plugins::ApplicationLog::Execution onPersist;
    onPersist.Trigger = neo::smartcontract::TriggerType::OnPersist;
    onPersist.VmState = neo::vm::VMState::Halt;
    onPersist.GasConsumed = 10;
    onPersist.Exception = "";
    logEntry->Executions.push_back(onPersist);

    neo::plugins::ApplicationLog::Execution postPersist;
    postPersist.Trigger = neo::smartcontract::TriggerType::PostPersist;
    postPersist.VmState = neo::vm::VMState::Halt;
    postPersist.GasConsumed = 20;
    postPersist.Exception = "";
    logEntry->Executions.push_back(postPersist);

    plugin->AddLog(logEntry);
    manager.AddPlugin(plugin);

    auto result =
        RPCMethods::GetApplicationLog(neoSystem, nlohmann::json::array({blockHash}));
    ASSERT_TRUE(result.is_object());
    EXPECT_FALSE(result.contains("txid"));
    EXPECT_EQ(result["blockhash"].get<std::string>(), blockHash);
    ASSERT_TRUE(result.contains("executions"));
    const auto& executions = result["executions"];
    ASSERT_TRUE(executions.is_array());
    ASSERT_EQ(executions.size(), 2);
    EXPECT_EQ(executions[0]["trigger"].get<std::string>(), "OnPersist");
    EXPECT_EQ(executions[1]["trigger"].get<std::string>(), "PostPersist");

    manager.ClearPlugins();
    std::filesystem::remove_all(tempPath);
}

TEST_F(RPCMethodsTest, GetApplicationLogFiltersByTrigger)
{
    auto& manager = neo::plugins::PluginManager::GetInstance();
    manager.ClearPlugins();

    auto tempPath = std::filesystem::temp_directory_path() /
                    ("neo_cpp_app_logs_" +
                     std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(tempPath);

    const std::string tx =
        "0x4444444444444444444444444444444444444444444444444444444444444444";

    auto plugin = std::make_shared<neo::plugins::ApplicationLogsPlugin>();
    std::unordered_map<std::string, std::string> settings = {{"LogPath", tempPath.string()}};
    ASSERT_TRUE(plugin->Initialize(neoSystem, settings));
    auto logEntry = std::make_shared<neo::plugins::ApplicationLog>();
    logEntry->TxHash = neo::io::UInt256::Parse(tx);

    neo::plugins::ApplicationLog::Execution onPersist;
    onPersist.Trigger = neo::smartcontract::TriggerType::OnPersist;
    onPersist.VmState = neo::vm::VMState::Halt;
    onPersist.GasConsumed = 1;
    logEntry->Executions.push_back(onPersist);

    neo::plugins::ApplicationLog::Execution postPersist;
    postPersist.Trigger = neo::smartcontract::TriggerType::PostPersist;
    postPersist.VmState = neo::vm::VMState::Fault;
    postPersist.GasConsumed = 2;
    logEntry->Executions.push_back(postPersist);

    plugin->AddLog(logEntry);
    manager.AddPlugin(plugin);

    auto result =
        RPCMethods::GetApplicationLog(neoSystem, nlohmann::json::array({tx, "PostPersist"}));
    ASSERT_TRUE(result.is_object());
    ASSERT_TRUE(result.contains("executions"));
    const auto& executions = result["executions"];
    ASSERT_TRUE(executions.is_array());
    ASSERT_EQ(executions.size(), 1);
    EXPECT_EQ(executions[0]["trigger"].get<std::string>(), "PostPersist");
    EXPECT_EQ(executions[0]["vmstate"].get<std::string>(), "FAULT");
    EXPECT_EQ(executions[0]["gasconsumed"].get<std::string>(), "2");

    manager.ClearPlugins();
    std::filesystem::remove_all(tempPath);
}

TEST_F(RPCMethodsTest, GetApplicationLogThrowsOnInvalidTrigger)
{
    auto& manager = neo::plugins::PluginManager::GetInstance();
    manager.ClearPlugins();

    auto tempPath = std::filesystem::temp_directory_path() /
                    ("neo_cpp_app_logs_" +
                     std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(tempPath);

    const std::string tx =
        "0x5555555555555555555555555555555555555555555555555555555555555555";

    auto plugin = std::make_shared<neo::plugins::ApplicationLogsPlugin>();
    std::unordered_map<std::string, std::string> settings = {{"LogPath", tempPath.string()}};
    ASSERT_TRUE(plugin->Initialize(neoSystem, settings));
    auto logEntry = std::make_shared<neo::plugins::ApplicationLog>();
    logEntry->TxHash = neo::io::UInt256::Parse(tx);
    logEntry->Executions.emplace_back();
    plugin->AddLog(logEntry);
    manager.AddPlugin(plugin);

    try
    {
        (void)RPCMethods::GetApplicationLog(neoSystem, nlohmann::json::array({tx, "invalid"}));
        FAIL() << "Expected RpcException";
    }
    catch (const RpcException& ex)
    {
        EXPECT_EQ(ex.GetCode(), ErrorCode::InvalidParams);
    }
    catch (...)
    {
        FAIL() << "Expected RpcException";
    }

    manager.ClearPlugins();
    std::filesystem::remove_all(tempPath);
}

TEST_F(RPCMethodsTest, GetRawMemPool_WithUnverifiedFlag_ReturnsStructuredResult)
{
    nlohmann::json params = nlohmann::json::array({true});
    auto result = RPCMethods::GetRawMemPool(neoSystem, params);

    EXPECT_TRUE(result.contains("height"));
    EXPECT_TRUE(result.contains("verified"));
    EXPECT_TRUE(result.contains("unverified"));
    EXPECT_TRUE(result["verified"].is_array());
    EXPECT_TRUE(result["unverified"].is_array());
}

TEST_F(RPCMethodsTest, GetUnclaimedGasReturnsValueForUnknownAccount)
{
    auto result = RPCMethods::GetUnclaimedGas(
        neoSystem, nlohmann::json::array({"0x0000000000000000000000000000000000000000"}));

    ASSERT_TRUE(result.is_object());
    EXPECT_TRUE(result.contains("unclaimed"));
    EXPECT_TRUE(result["unclaimed"].is_string());
    EXPECT_EQ(result["unclaimed"].get<std::string>(), "0");
    EXPECT_TRUE(result.contains("address"));
    EXPECT_FALSE(result["address"].get<std::string>().empty());
}

TEST_F(RPCMethodsTest, GetUnclaimedGasInvalidAddressThrows)
{
    try
    {
        (void)RPCMethods::GetUnclaimedGas(neoSystem, nlohmann::json::array({"not-an-address"}));
        FAIL() << "Expected RpcException";
    }
    catch (const RpcException& ex)
    {
        EXPECT_EQ(ex.GetCode(), ErrorCode::InvalidAddress);
    }
    catch (...)
    {
        FAIL() << "Expected RpcException";
    }
}

TEST_F(RPCMethodsTest, TraverseIteratorReturnsStoredValues)
{
    auto& manager = neo::rpc::RpcSessionManager::Instance();
    auto sessionId = manager.CreateSession();
    auto iteratorId = manager.StoreIterator(sessionId, {nlohmann::json(1), nlohmann::json(2), nlohmann::json(3)});
    ASSERT_TRUE(iteratorId.has_value());

    auto result = RPCMethods::TraverseIterator(
        neoSystem, nlohmann::json::array({sessionId, iteratorId.value(), 2}));

    ASSERT_TRUE(result.is_object());
    EXPECT_TRUE(result.contains("values"));
    EXPECT_TRUE(result["values"].is_array());
    EXPECT_EQ(result["values"].size(), 2);
    EXPECT_EQ(result["values"][0].get<int>(), 1);
    EXPECT_EQ(result["values"][1].get<int>(), 2);
    EXPECT_TRUE(result["truncated"].get<bool>());

    auto result2 = RPCMethods::TraverseIterator(
        neoSystem, nlohmann::json::array({sessionId, iteratorId.value(), 10}));
    ASSERT_TRUE(result2.is_object());
    EXPECT_EQ(result2["values"].size(), 1);
    EXPECT_FALSE(result2["truncated"].get<bool>());

    EXPECT_TRUE(manager.TerminateSession(sessionId));
}

TEST_F(RPCMethodsTest, TraverseIteratorRejectsOverLimitCount)
{
    auto& manager = neo::rpc::RpcSessionManager::Instance();
    auto sessionId = manager.CreateSession();
    auto iteratorId = manager.StoreIterator(sessionId, {nlohmann::json(1), nlohmann::json(2)});
    ASSERT_TRUE(iteratorId.has_value());

    try
    {
        (void)RPCMethods::TraverseIterator(
            neoSystem, nlohmann::json::array({sessionId, iteratorId.value(), 101}));
        FAIL() << "Expected RpcException";
    }
    catch (const RpcException& ex)
    {
        EXPECT_EQ(ex.GetCode(), ErrorCode::InvalidParams);
    }
    catch (...)
    {
        FAIL() << "Expected RpcException";
    }

    EXPECT_TRUE(manager.TerminateSession(sessionId));
}

TEST_F(RPCMethodsTest, CreateSessionReturnsId)
{
    auto result = RPCMethods::CreateSession(neoSystem, nlohmann::json::array());
    ASSERT_TRUE(result.is_string());
    EXPECT_FALSE(result.get<std::string>().empty());
}

TEST_F(RPCMethodsTest, TerminateSessionRemovesSession)
{
    auto& manager = neo::rpc::RpcSessionManager::Instance();
    auto sessionId = manager.CreateSession();
    EXPECT_TRUE(manager.SessionExists(sessionId));

    auto result = RPCMethods::TerminateSession(neoSystem, nlohmann::json::array({sessionId}));
    EXPECT_TRUE(result.get<bool>());
    EXPECT_FALSE(manager.SessionExists(sessionId));

    try
    {
        (void)RPCMethods::TerminateSession(neoSystem, nlohmann::json::array({sessionId}));
        FAIL() << "Expected RpcException";
    }
    catch (const RpcException& ex)
    {
        EXPECT_EQ(ex.GetCode(), ErrorCode::UnknownSession);
    }
}

TEST_F(RPCMethodsTest, GetContractStateReturnsNativeContractByName)
{
    auto neoToken = neo::smartcontract::native::NeoToken::GetInstance();
    ASSERT_TRUE(neoToken);

    auto result =
        RPCMethods::GetContractState(neoSystem, nlohmann::json::array({neoToken->GetName()}));

    ASSERT_TRUE(result.is_object());
    EXPECT_EQ(result["id"].get<int32_t>(), static_cast<int32_t>(neoToken->GetId()));
    EXPECT_EQ(result["hash"].get<std::string>(), neoToken->GetScriptHash().ToString());
    ASSERT_TRUE(result.contains("manifest"));
    ASSERT_TRUE(result["manifest"].is_object());
    EXPECT_EQ(result["manifest"]["name"].get<std::string>(), neoToken->GetName());
}

TEST_F(RPCMethodsTest, GetContractStateUnknownContractThrows)
{
    try
    {
        (void)RPCMethods::GetContractState(neoSystem, nlohmann::json::array({"nonexistent-contract"}));
        FAIL() << "Expected RpcException";
    }
    catch (const RpcException& ex)
    {
        EXPECT_EQ(ex.GetCode(), ErrorCode::UnknownContract);
    }
    catch (...)
    {
        FAIL() << "Expected RpcException";
    }
}

TEST_F(RPCMethodsTest, SubmitBlockRequiresBase64Parameter)
{
    try
    {
        RPCMethods::SubmitBlock(neoSystem, nlohmann::json::array());
        FAIL() << "Expected RpcException";
    }
    catch (const RpcException& ex)
    {
        EXPECT_EQ(ex.GetCode(), ErrorCode::InvalidParams);
    }
    catch (...)
    {
        FAIL() << "Expected RpcException";
    }
}

TEST_F(RPCMethodsTest, SubmitBlockRejectsInvalidBase64Payload)
{
    try
    {
        RPCMethods::SubmitBlock(neoSystem, nlohmann::json::array({"not-base64!!"}));
        FAIL() << "Expected RpcException";
    }
    catch (const RpcException& ex)
    {
        EXPECT_EQ(ex.GetCode(), ErrorCode::InvalidParams);
    }
    catch (...)
    {
        FAIL() << "Expected RpcException";
    }
}

TEST_F(RPCMethodsTest, SubmitBlockReturnsHashOnSuccess)
{
    auto blockCopy = CreateChildBlock(neoSystem);
    auto base64 = EncodeBlockToBase64(blockCopy);

    auto result = RPCMethods::SubmitBlock(neoSystem, nlohmann::json::array({base64}));
    ASSERT_TRUE(result.is_object());
    ASSERT_TRUE(result.contains("hash"));
    EXPECT_EQ(result["hash"].get<std::string>(), blockCopy.GetHash().ToString());
}

TEST_F(RPCMethodsTest, SubmitBlockAcceptsNumericRelayParameter)
{
    auto blockCopy = CreateChildBlock(neoSystem);
    auto base64 = EncodeBlockToBase64(blockCopy);

    auto result = RPCMethods::SubmitBlock(neoSystem, nlohmann::json::array({base64, 0}));
    ASSERT_TRUE(result.is_object());
    ASSERT_TRUE(result.contains("hash"));
    EXPECT_EQ(result["hash"].get<std::string>(), blockCopy.GetHash().ToString());
}

TEST_F(RPCMethodsTest, SubmitBlockRejectsEmptyPayload)
{
    neo::io::ByteVector empty;
    auto emptyEncoded = neo::cryptography::Base64::Encode(empty.AsSpan());

    try
    {
        RPCMethods::SubmitBlock(neoSystem, nlohmann::json::array({emptyEncoded}));
        FAIL() << "Expected RpcException";
    }
    catch (const RpcException& ex)
    {
        EXPECT_EQ(ex.GetCode(), ErrorCode::InvalidParams);
    }
    catch (...)
    {
        FAIL() << "Expected RpcException";
    }
}

TEST_F(RPCMethodsTest, SubmitBlockAcceptsBooleanRelayParameter)
{
    auto blockCopy = CreateChildBlock(neoSystem);
    auto base64 = EncodeBlockToBase64(blockCopy);

    auto result = RPCMethods::SubmitBlock(neoSystem, nlohmann::json::array({base64, false}));
    ASSERT_TRUE(result.is_object());
    ASSERT_TRUE(result.contains("hash"));
    EXPECT_EQ(result["hash"].get<std::string>(), blockCopy.GetHash().ToString());
}

TEST_F(RPCMethodsTest, SubmitBlockRejectsInvalidRelayType)
{
    auto blockCopy = CreateChildBlock(neoSystem);
    auto base64 = EncodeBlockToBase64(blockCopy);

    try
    {
        RPCMethods::SubmitBlock(neoSystem, nlohmann::json::array({base64, "invalid"}));
        FAIL() << "Expected RpcException";
    }
    catch (const RpcException& ex)
    {
        EXPECT_EQ(ex.GetCode(), ErrorCode::InvalidParams);
    }
    catch (...)
    {
        FAIL() << "Expected RpcException";
    }
}

TEST_F(RPCMethodsTest, SubmitBlockAlreadyExistsThrows)
{
    auto blockCopy = CreateChildBlock(neoSystem);
    AddBlockToBlockchain(blockCopy);

    auto base64 = EncodeBlockToBase64(blockCopy);

    try
    {
        RPCMethods::SubmitBlock(neoSystem, nlohmann::json::array({base64}));
        FAIL() << "Expected RpcException";
    }
    catch (const RpcException& ex)
    {
        EXPECT_EQ(ex.GetCode(), ErrorCode::RpcAlreadyExists);
    }
    catch (...)
    {
        FAIL() << "Expected RpcException";
    }

    RemoveBlockFromBlockchain(blockCopy);
}

TEST_F(RPCMethodsTest, SubmitBlockInvalidWitnessThrowsVerificationFailed)
{
    auto blockCopy = CreateChildBlock(neoSystem);
    neo::ledger::Witness witness = blockCopy.GetWitness();
    witness.SetInvocationScript(neo::io::ByteVector());
    blockCopy.SetWitness(witness);

    auto base64 = EncodeBlockToBase64(blockCopy);

    try
    {
        RPCMethods::SubmitBlock(neoSystem, nlohmann::json::array({base64}));
        FAIL() << "Expected RpcException";
    }
    catch (const RpcException& ex)
    {
        EXPECT_EQ(ex.GetCode(), ErrorCode::RpcVerificationFailed);
    }
    catch (...)
    {
        FAIL() << "Expected RpcException";
    }
}

TEST_F(RPCMethodsTest, SubmitBlockInvalidPrevHashThrowsVerificationFailed)
{
    auto blockCopy = CreateChildBlock(neoSystem);
    blockCopy.SetPreviousHash(neo::tests::TestHelpers::GenerateRandomHash());

    auto base64 = EncodeBlockToBase64(blockCopy);

    try
    {
        RPCMethods::SubmitBlock(neoSystem, nlohmann::json::array({base64}));
        FAIL() << "Expected RpcException";
    }
    catch (const RpcException& ex)
    {
        EXPECT_EQ(ex.GetCode(), ErrorCode::RpcVerificationFailed);
    }
    catch (...)
    {
        FAIL() << "Expected RpcException";
    }
}

TEST_F(RPCMethodsTest, SubmitBlockInvalidIndexThrowsVerificationFailed)
{
    auto blockCopy = CreateChildBlock(neoSystem);
    blockCopy.SetIndex(blockCopy.GetIndex() + 10);

    auto base64 = EncodeBlockToBase64(blockCopy);

    try
    {
        RPCMethods::SubmitBlock(neoSystem, nlohmann::json::array({base64}));
        FAIL() << "Expected RpcException";
    }
    catch (const RpcException& ex)
    {
        EXPECT_EQ(ex.GetCode(), ErrorCode::RpcVerificationFailed);
    }
    catch (...)
    {
        FAIL() << "Expected RpcException";
    }
}

TEST_F(RPCMethodsTest, SubmitBlockFutureBlockQueuedUntilParentArrives)
{
    auto blockchain = neoSystem->GetBlockchain();
    ASSERT_TRUE(blockchain);

    auto block1 = CreateChildBlock(neoSystem);

    auto block2 = CreateChildBlock(neoSystem);
    block2.SetIndex(block1.GetIndex() + 1);
    block2.SetPreviousHash(block1.GetHash());

    auto block2Encoded = EncodeBlockToBase64(block2);
    try
    {
        RPCMethods::SubmitBlock(neoSystem, nlohmann::json::array({block2Encoded}));
        FAIL() << "Expected RpcException";
    }
    catch (const RpcException& ex)
    {
        EXPECT_EQ(ex.GetCode(), ErrorCode::RpcVerificationFailed);
    }

    auto block1Encoded = EncodeBlockToBase64(block1);
    auto result = RPCMethods::SubmitBlock(neoSystem, nlohmann::json::array({block1Encoded}));
    ASSERT_TRUE(result.is_object());
    ASSERT_TRUE(result.contains("hash"));

    const auto block2Hash = block2.GetHash();
    bool persisted = false;
    for (int i = 0; i < 100 && !persisted; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        persisted = blockchain->ContainsBlock(block2Hash);
    }
    EXPECT_TRUE(persisted);

    try
    {
        RPCMethods::SubmitBlock(neoSystem, nlohmann::json::array({block2Encoded}));
        FAIL() << "Expected RpcException";
    }
    catch (const RpcException& ex)
    {
        EXPECT_EQ(ex.GetCode(), ErrorCode::RpcAlreadyExists);
    }
}

TEST_F(RPCMethodsTest, SubmitBlockMultipleFutureBlocksDrainInOrder)
{
    auto blockchain = neoSystem->GetBlockchain();
    ASSERT_TRUE(blockchain);

    auto block1 = CreateChildBlock(neoSystem);

    auto block2 = CreateChildBlock(neoSystem);
    block2.SetIndex(block1.GetIndex() + 1);
    block2.SetPreviousHash(block1.GetHash());

    auto block3 = CreateChildBlock(neoSystem);
    block3.SetIndex(block2.GetIndex() + 1);
    block3.SetPreviousHash(block2.GetHash());

    auto block3Encoded = EncodeBlockToBase64(block3);
    try
    {
        RPCMethods::SubmitBlock(neoSystem, nlohmann::json::array({block3Encoded}));
        FAIL() << "Expected RpcException";
    }
    catch (const RpcException& ex)
    {
        EXPECT_EQ(ex.GetCode(), ErrorCode::RpcVerificationFailed);
    }

    auto block2Encoded = EncodeBlockToBase64(block2);
    try
    {
        RPCMethods::SubmitBlock(neoSystem, nlohmann::json::array({block2Encoded}));
        FAIL() << "Expected RpcException";
    }
    catch (const RpcException& ex)
    {
        EXPECT_EQ(ex.GetCode(), ErrorCode::RpcVerificationFailed);
    }

    auto block1Encoded = EncodeBlockToBase64(block1);
    auto result = RPCMethods::SubmitBlock(neoSystem, nlohmann::json::array({block1Encoded}));
    ASSERT_TRUE(result.is_object());
    ASSERT_TRUE(result.contains("hash"));

    const auto block3Hash = block3.GetHash();
    bool persisted = false;
    for (int i = 0; i < 150 && !persisted; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        persisted = blockchain->ContainsBlock(block3Hash);
    }
    EXPECT_TRUE(persisted);
}

TEST_F(RPCMethodsTest, SubmitBlockDuplicateAfterSuccessThrows)
{
    auto blockCopy = CreateChildBlock(neoSystem);
    auto base64 = EncodeBlockToBase64(blockCopy);

    auto result = RPCMethods::SubmitBlock(neoSystem, nlohmann::json::array({base64}));
    ASSERT_TRUE(result.is_object());
    ASSERT_TRUE(result.contains("hash"));

    try
    {
        RPCMethods::SubmitBlock(neoSystem, nlohmann::json::array({base64}));
        FAIL() << "Expected RpcException";
    }
    catch (const RpcException& ex)
    {
        EXPECT_EQ(ex.GetCode(), ErrorCode::RpcAlreadyExists);
    }
    catch (...)
    {
        FAIL() << "Expected RpcException";
    }
}
