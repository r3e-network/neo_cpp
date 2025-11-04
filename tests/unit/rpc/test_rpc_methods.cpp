#include <gtest/gtest.h>

#include <memory>
#include <sstream>

#include <neo/cryptography/base64.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/io/json.h>
#include <neo/io/uint160.h>
#include <neo/ledger/block.h>
#include <neo/ledger/memory_pool.h>
#include <neo/ledger/signer.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/witness.h>
#include <neo/node/neo_system.h>
#include <neo/network/p2p/payloads/neo3_transaction.h>
#include <neo/persistence/data_cache.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>
#include <neo/protocol_settings.h>
#include <neo/rpc/error_codes.h>
#include <neo/rpc/rpc_methods.h>
#include <neo/wallets/helper.h>
#include <neo/smartcontract/native/contract_management.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/rpc/rpc_session_manager.h>

using namespace neo::rpc;
using namespace neo::node;

class RPCMethodsTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Create protocol settings
        auto protocolSettings = std::make_shared<neo::ProtocolSettings>();

        // Create a neo system with memory store
        neoSystem = std::make_shared<NeoSystem>(protocolSettings);
        ASSERT_TRUE(neoSystem->Start());
    }

    void TearDown() override
    {
        if (neoSystem)
        {
            neoSystem->Stop();
        }
    }

    std::shared_ptr<NeoSystem> neoSystem;

    neo::network::p2p::payloads::Neo3Transaction CreateTestTransaction(uint32_t validUntilBlock) const
    {
        neo::network::p2p::payloads::Neo3Transaction tx;
        tx.SetVersion(0);
        tx.SetNonce(42);
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
};

TEST_F(RPCMethodsTest, GetVersion)
{
    nlohmann::json params = nlohmann::json::array();
    auto result = RPCMethods::GetVersion(neoSystem, params);

    EXPECT_TRUE(result.contains("port"));
    EXPECT_TRUE(result.contains("nonce"));
    EXPECT_TRUE(result.contains("useragent"));
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

TEST_F(RPCMethodsTest, ListPluginsReturnsArray)
{
    auto result = RPCMethods::ListPlugins(neoSystem, nlohmann::json::array());

    ASSERT_TRUE(result.is_array());
    for (const auto& entry : result)
    {
        ASSERT_TRUE(entry.is_object());
        EXPECT_TRUE(entry.contains("name"));
        EXPECT_TRUE(entry.contains("running"));
    }
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
    nlohmann::json params = nlohmann::json::array();
    auto result = RPCMethods::GetPeers(neoSystem, params);

    EXPECT_TRUE(result.contains("connected"));
    EXPECT_TRUE(result["connected"].is_array());
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

TEST_F(RPCMethodsTest, SendRawTransaction_InvalidTransaction_ThrowsRpcException)
{
    // Build a minimal transaction that fails validation (missing signers/witnesses).
    neo::ledger::Transaction tx;
    tx.SetVersion(0);
    tx.SetNonce(1);
    tx.SetSystemFee(0);
    tx.SetNetworkFee(0);
    tx.SetValidUntilBlock(neoSystem->GetBlockchain()->GetHeight() + 10);
    tx.SetScript(neo::io::ByteVector::FromHexString("00"));

    std::ostringstream stream;
    neo::io::BinaryWriter writer(stream);
    tx.Serialize(writer);
    const auto payload = stream.str();

    const auto base64 =
        neo::cryptography::Base64::Encode(neo::io::ByteSpan(reinterpret_cast<const uint8_t*>(payload.data()),
                                                            payload.size()));

    nlohmann::json params = nlohmann::json::array({base64});

    try
    {
        (void)RPCMethods::SendRawTransaction(neoSystem, params);
        FAIL() << "Expected RpcException";
    }
    catch (const neo::rpc::RpcException& ex)
    {
        EXPECT_EQ(static_cast<int>(ex.GetCode()),
                  static_cast<int>(neo::rpc::ErrorCode::TransactionVerificationFailed));
        EXPECT_NE(ex.GetMessage().find("Transaction rejected"), std::string::npos);
    }
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
    auto blockchain = neoSystem->GetBlockchain();
    ASSERT_TRUE(blockchain);
    auto genesis = blockchain->GetBlock(0);
    ASSERT_TRUE(genesis);

    neo::ledger::Block blockCopy = *genesis;
    blockCopy.SetIndex(genesis->GetIndex() + 1);
    blockCopy.SetPreviousHash(genesis->GetHash());
    neo::io::ByteVector buffer;
    neo::io::BinaryWriter writer(buffer);
    blockCopy.Serialize(writer);

    auto base64 = neo::cryptography::Base64::Encode(buffer.AsSpan());

    auto result = RPCMethods::SubmitBlock(neoSystem, nlohmann::json::array({base64}));
    ASSERT_TRUE(result.is_object());
    ASSERT_TRUE(result.contains("hash"));
    EXPECT_EQ(result["hash"].get<std::string>(), blockCopy.GetHash().ToString());
}
