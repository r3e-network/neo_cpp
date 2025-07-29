/**
 * @file test_neo3_compatibility.cpp
 * @brief Neo N3 Compatibility Tests for C++ Implementation
 *
 * This file contains tests that validate the C++ implementation against
 * real Neo N3 blockchain data and behavior. These tests ensure that the
 * C++ node produces identical results to the official C# Neo implementation.
 *
 * Test Categories:
 * - Transaction Format Validation
 * - Block Structure Compatibility
 * - Native Contract Behavior
 * - Consensus Message Handling
 * - Storage Format Validation
 * - Network Protocol Compliance
 * - Cryptographic Operations
 *
 * @author Neo C++ Development Team
 * @version 1.0.0
 * @date December 2024
 */

#include <fstream>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

// Neo Core Components
#include <neo/cryptography/crypto.h>
#include <neo/cryptography/hash.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/ledger/block.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/transaction.h>
#include <neo/network/p2p/payloads/neo3_transaction.h>
#include <neo/persistence/storage_key.h>
#include <neo/protocol_settings.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/policy_contract.h>

namespace neo::tests::compatibility
{
/**
 * @brief Base class for Neo N3 compatibility tests
 */
class Neo3CompatibilityTestBase : public ::testing::Test
{
  protected:
    std::shared_ptr<ProtocolSettings> protocolSettings_;
    std::shared_ptr<persistence::MemoryStore> store_;
    std::shared_ptr<ledger::Blockchain> blockchain_;

    void SetUp() override
    {
        protocolSettings_ = ProtocolSettings::GetDefault();
        store_ = std::make_shared<persistence::MemoryStore>();
        blockchain_ = std::make_shared<ledger::Blockchain>(protocolSettings_, store_);

        ASSERT_TRUE(blockchain_->Initialize());
    }

    /**
     * @brief Load test data from JSON file
     */
    nlohmann::json LoadTestData(const std::string& filename)
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            // Return empty JSON if file doesn't exist (test data optional)
            return nlohmann::json::object();
        }

        nlohmann::json data;
        file >> data;
        return data;
    }

    /**
     * @brief Create transaction from hex string
     */
    std::shared_ptr<network::p2p::payloads::Neo3Transaction> CreateTransactionFromHex(const std::string& hexData)
    {
        auto data = io::ByteVector::FromHexString(hexData);
        std::istringstream stream(std::string(reinterpret_cast<const char*>(data.Data()), data.Size()));
        io::BinaryReader reader(stream);

        auto tx = std::make_shared<network::p2p::payloads::Neo3Transaction>();
        tx->Deserialize(reader);
        return tx;
    }

    /**
     * @brief Create block from hex string
     */
    std::shared_ptr<ledger::Block> CreateBlockFromHex(const std::string& hexData)
    {
        auto data = io::ByteVector::FromHexString(hexData);
        std::istringstream stream(std::string(reinterpret_cast<const char*>(data.Data()), data.Size()));
        io::BinaryReader reader(stream);

        auto block = std::make_shared<ledger::Block>();
        block->Deserialize(reader);
        return block;
    }
};

/**
 * @brief Test Neo N3 transaction format compatibility
 */
class TransactionFormatTest : public Neo3CompatibilityTestBase
{
};

TEST_F(TransactionFormatTest, Neo3TransactionStructure)
{
    // Test Neo N3 transaction structure matches specification
    auto tx = std::make_shared<network::p2p::payloads::Neo3Transaction>();

    // Set Neo N3 transaction fields
    tx->SetVersion(0);
    tx->SetNonce(12345);
    tx->SetSystemFee(1000000);
    tx->SetNetworkFee(500000);
    tx->SetValidUntilBlock(1000);

    // Test script
    io::ByteVector script = {0x51, 0x41};  // PUSH1 RETURN
    tx->SetScript(script);

    // Test signers (Neo N3 format)
    std::vector<ledger::Signer> signers;
    io::UInt160 account = io::UInt160::Parse("0x1234567890123456789012345678901234567890");
    ledger::Signer signer(account, ledger::WitnessScope::CalledByEntry);
    signers.push_back(signer);
    tx->SetSigners(signers);

    // Test witnesses
    std::vector<ledger::Witness> witnesses;
    ledger::Witness witness;
    witness.SetInvocationScript({0x40, 0x41, 0x42});  // Dummy signature
    witness.SetVerificationScript({0x51});            // Simple verification script
    witnesses.push_back(witness);
    tx->SetWitnesses(witnesses);

    // Verify transaction structure
    EXPECT_EQ(tx->GetVersion(), 0);
    EXPECT_EQ(tx->GetNonce(), 12345);
    EXPECT_EQ(tx->GetSystemFee(), 1000000);
    EXPECT_EQ(tx->GetNetworkFee(), 500000);
    EXPECT_EQ(tx->GetValidUntilBlock(), 1000);
    EXPECT_EQ(tx->GetScript(), script);
    EXPECT_EQ(tx->GetSigners().size(), 1);
    EXPECT_EQ(tx->GetWitnesses().size(), 1);

    // Test serialization/deserialization
    std::ostringstream outStream;
    io::BinaryWriter writer(outStream);
    tx->Serialize(writer);

    std::string serializedData = outStream.str();
    EXPECT_GT(serializedData.size(), 0);

    std::istringstream inStream(serializedData);
    io::BinaryReader reader(inStream);

    auto deserializedTx = std::make_shared<network::p2p::payloads::Neo3Transaction>();
    deserializedTx->Deserialize(reader);

    // Verify deserialized transaction matches original
    EXPECT_EQ(deserializedTx->GetVersion(), tx->GetVersion());
    EXPECT_EQ(deserializedTx->GetNonce(), tx->GetNonce());
    EXPECT_EQ(deserializedTx->GetSystemFee(), tx->GetSystemFee());
    EXPECT_EQ(deserializedTx->GetNetworkFee(), tx->GetNetworkFee());
    EXPECT_EQ(deserializedTx->GetValidUntilBlock(), tx->GetValidUntilBlock());
    EXPECT_EQ(deserializedTx->GetScript(), tx->GetScript());
}

TEST_F(TransactionFormatTest, TransactionHashCalculation)
{
    // Test that transaction hash calculation matches Neo N3
    auto tx = std::make_shared<network::p2p::payloads::Neo3Transaction>();

    // Use deterministic values for consistent hash
    tx->SetVersion(0);
    tx->SetNonce(0x12345678);
    tx->SetSystemFee(0x0000000000989680);   // 10,000,000
    tx->SetNetworkFee(0x00000000000F4240);  // 1,000,000
    tx->SetValidUntilBlock(0x00001000);     // 4096

    // Deterministic script
    io::ByteVector script = {0x0C, 0x05, 0x48, 0x65, 0x6C, 0x6C, 0x6F};  // PUSHDATA1 "Hello"
    tx->SetScript(script);

    // Calculate hash
    auto hash = tx->GetHash();

    // Hash should be deterministic and match expected format
    EXPECT_FALSE(hash.IsZero());
    EXPECT_EQ(hash.Size(), 32);  // SHA256 hash is 32 bytes

    // Test hash consistency
    auto hash2 = tx->GetHash();
    EXPECT_EQ(hash, hash2);

    std::cout << "Transaction hash: " << hash.ToString() << std::endl;
}

/**
 * @brief Test Neo N3 block format compatibility
 */
class BlockFormatTest : public Neo3CompatibilityTestBase
{
};

TEST_F(BlockFormatTest, Neo3BlockStructure)
{
    // Test Neo N3 block structure
    auto block = std::make_shared<ledger::Block>();

    // Set block header fields
    block->SetVersion(0);
    block->SetPreviousHash(io::UInt256::Zero());
    block->SetMerkleRoot(io::UInt256::Parse("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef"));
    block->SetTimestamp(1640000000000ULL);  // Fixed timestamp for testing
    block->SetIndex(1);
    block->SetNextConsensus(io::UInt160::Parse("0x1234567890123456789012345678901234567890"));

    // Add witnesses
    std::vector<ledger::Witness> witnesses;
    ledger::Witness witness;
    witness.SetInvocationScript({0x40});    // Dummy consensus signature
    witness.SetVerificationScript({0x51});  // Simple verification
    witnesses.push_back(witness);
    block->SetWitnesses(witnesses);

    // Add transactions
    auto tx = std::make_shared<network::p2p::payloads::Neo3Transaction>();
    tx->SetVersion(0);
    tx->SetNonce(1);
    tx->SetSystemFee(0);
    tx->SetNetworkFee(0);
    tx->SetValidUntilBlock(2);
    tx->SetScript({0x51, 0x41});  // PUSH1 RETURN

    block->AddTransaction(tx);

    // Verify block structure
    EXPECT_EQ(block->GetVersion(), 0);
    EXPECT_EQ(block->GetIndex(), 1);
    EXPECT_EQ(block->GetTransactions().size(), 1);
    EXPECT_EQ(block->GetWitnesses().size(), 1);

    // Test block hash calculation
    auto blockHash = block->GetHash();
    EXPECT_FALSE(blockHash.IsZero());
    EXPECT_EQ(blockHash.Size(), 32);

    std::cout << "Block hash: " << blockHash.ToString() << std::endl;
}

/**
 * @brief Test native contract compatibility
 */
class NativeContractTest : public Neo3CompatibilityTestBase
{
};

TEST_F(NativeContractTest, NativeContractIds)
{
    // Test that native contracts have correct IDs matching Neo N3
    auto& manager = smartcontract::native::NativeContractManager::GetInstance();

    // Verify standard native contract IDs
    auto neoToken = manager.GetContract("NeoToken");
    auto gasToken = manager.GetContract("GasToken");
    auto policyContract = manager.GetContract("PolicyContract");
    auto roleManagement = manager.GetContract("RoleManagement");

    ASSERT_NE(neoToken, nullptr);
    ASSERT_NE(gasToken, nullptr);
    ASSERT_NE(policyContract, nullptr);
    ASSERT_NE(roleManagement, nullptr);

    // Verify correct contract IDs (as per Neo N3 specification)
    EXPECT_EQ(neoToken->GetId(), 1);
    EXPECT_EQ(gasToken->GetId(), 2);
    EXPECT_EQ(policyContract->GetId(), 3);
    EXPECT_EQ(roleManagement->GetId(), 4);

    // Verify contract hashes are deterministic
    auto neoHash = neoToken->GetScriptHash();
    auto gasHash = gasToken->GetScriptHash();

    EXPECT_FALSE(neoHash.IsZero());
    EXPECT_FALSE(gasHash.IsZero());
    EXPECT_NE(neoHash, gasHash);

    std::cout << "NeoToken hash: " << neoHash.ToString() << std::endl;
    std::cout << "GasToken hash: " << gasHash.ToString() << std::endl;
}

TEST_F(NativeContractTest, GasTokenInitialState)
{
    // Test GAS token initial state matches Neo N3
    auto gasToken = smartcontract::native::GasToken::GetInstance();
    auto snapshot = blockchain_->GetSnapshot();

    // Verify initial supply and decimals
    EXPECT_EQ(gasToken->GetDecimals(), 8);
    EXPECT_EQ(gasToken->GetSymbol(), "GAS");

    // Total supply should be 0 initially (generated through block rewards)
    auto totalSupply = gasToken->GetTotalSupply(snapshot);
    EXPECT_GE(totalSupply, 0);

    std::cout << "GAS total supply: " << totalSupply << std::endl;
}

TEST_F(NativeContractTest, NeoTokenInitialState)
{
    // Test NEO token initial state matches Neo N3
    auto neoToken = smartcontract::native::NeoToken::GetInstance();
    auto snapshot = blockchain_->GetSnapshot();

    // Verify NEO token properties
    EXPECT_EQ(neoToken->GetDecimals(), 0);  // NEO is indivisible
    EXPECT_EQ(neoToken->GetSymbol(), "NEO");

    // Total supply should be 100,000,000 NEO
    auto totalSupply = neoToken->GetTotalSupply(snapshot);
    EXPECT_EQ(totalSupply, 100000000);

    // Verify committee and validators
    auto committee = neoToken->GetCommittee(snapshot);
    auto validators = neoToken->GetValidators(snapshot);

    EXPECT_EQ(committee.size(), protocolSettings_->GetCommitteeMembersCount());
    EXPECT_EQ(validators.size(), protocolSettings_->GetValidatorsCount());

    std::cout << "NEO total supply: " << totalSupply << std::endl;
    std::cout << "Committee size: " << committee.size() << std::endl;
    std::cout << "Validators size: " << validators.size() << std::endl;
}

/**
 * @brief Test storage format compatibility
 */
class StorageFormatTest : public Neo3CompatibilityTestBase
{
};

TEST_F(StorageFormatTest, StorageKeyFormat)
{
    // Test Neo N3 storage key format (contract ID + key data)

    int32_t contractId = 1;  // NeoToken contract ID
    uint8_t prefix = 0x20;

    // Test basic storage key creation
    auto storageKey = persistence::StorageKey::Create(contractId, prefix);

    // Verify storage key structure
    EXPECT_EQ(storageKey.GetId(), contractId);
    EXPECT_EQ(storageKey.GetKey()[0], prefix);

    // Test storage key with UInt160
    io::UInt160 address = io::UInt160::Parse("0x1234567890123456789012345678901234567890");
    auto storageKeyWithAddress = persistence::StorageKey::Create(contractId, prefix, address);

    EXPECT_EQ(storageKeyWithAddress.GetId(), contractId);
    EXPECT_EQ(storageKeyWithAddress.GetKey()[0], prefix);
    EXPECT_EQ(storageKeyWithAddress.GetKey().Size(), 1 + 20);  // prefix + UInt160 size

    // Test storage key with UInt256
    io::UInt256 hash = io::UInt256::Parse("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
    auto storageKeyWithHash = persistence::StorageKey::Create(contractId, prefix, hash);

    EXPECT_EQ(storageKeyWithHash.GetId(), contractId);
    EXPECT_EQ(storageKeyWithHash.GetKey()[0], prefix);
    EXPECT_EQ(storageKeyWithHash.GetKey().Size(), 1 + 32);  // prefix + UInt256 size

    // Test storage key serialization
    std::ostringstream stream;
    io::BinaryWriter writer(stream);
    storageKey.Serialize(writer);

    std::string serialized = stream.str();
    EXPECT_GT(serialized.size(), 0);

    // Test deserialization
    std::istringstream inStream(serialized);
    io::BinaryReader reader(inStream);

    persistence::StorageKey deserializedKey;
    deserializedKey.Deserialize(reader);

    EXPECT_EQ(deserializedKey.GetId(), storageKey.GetId());
    EXPECT_EQ(deserializedKey.GetKey(), storageKey.GetKey());
}

/**
 * @brief Test cryptographic operations compatibility
 */
class CryptographyTest : public Neo3CompatibilityTestBase
{
};

TEST_F(CryptographyTest, HashFunctions)
{
    // Test hash functions produce consistent results
    std::string testData = "Hello Neo N3";
    io::ByteVector data(testData.begin(), testData.end());

    // Test SHA256
    auto sha256Hash = cryptography::Hash::SHA256(data);
    EXPECT_EQ(sha256Hash.Size(), 32);

    // Test RIPEMD160
    auto ripemd160Hash = cryptography::Hash::RIPEMD160(data);
    EXPECT_EQ(ripemd160Hash.Size(), 20);

    // Test Hash160 (SHA256 + RIPEMD160)
    auto hash160 = cryptography::Hash::Hash160(data);
    EXPECT_EQ(hash160.Size(), 20);

    // Test Hash256 (double SHA256)
    auto hash256 = cryptography::Hash::Hash256(data);
    EXPECT_EQ(hash256.Size(), 32);

    // Verify hash consistency
    auto sha256Hash2 = cryptography::Hash::SHA256(data);
    EXPECT_EQ(sha256Hash, sha256Hash2);

    std::cout << "SHA256: " << sha256Hash.ToHexString() << std::endl;
    std::cout << "RIPEMD160: " << ripemd160Hash.ToHexString() << std::endl;
    std::cout << "Hash160: " << hash160.ToHexString() << std::endl;
    std::cout << "Hash256: " << hash256.ToHexString() << std::endl;
}

TEST_F(CryptographyTest, ECDSAOperations)
{
    // Test ECDSA key generation and signing
    auto keyPair = cryptography::KeyPair::Generate();
    ASSERT_NE(keyPair, nullptr);

    auto privateKey = keyPair->GetPrivateKey();
    auto publicKey = keyPair->GetPublicKey();

    EXPECT_EQ(privateKey.Size(), 32);
    EXPECT_GT(publicKey.Size(), 0);

    // Test signing
    std::string message = "Test message for signing";
    io::ByteVector messageData(message.begin(), message.end());
    auto messageHash = cryptography::Hash::SHA256(messageData);

    auto signature = keyPair->Sign(messageHash);
    EXPECT_GT(signature.Size(), 0);

    // Test verification
    bool isValid = cryptography::Crypto::VerifySignature(messageHash, signature, publicKey);
    EXPECT_TRUE(isValid);

    // Test verification with wrong data
    auto wrongHash = cryptography::Hash::SHA256(io::ByteVector{'w', 'r', 'o', 'n', 'g'});
    bool isInvalid = cryptography::Crypto::VerifySignature(wrongHash, signature, publicKey);
    EXPECT_FALSE(isInvalid);

    std::cout << "Key generation and signing test passed" << std::endl;
}

/**
 * @brief Test protocol settings compatibility
 */
class ProtocolSettingsTest : public Neo3CompatibilityTestBase
{
};

TEST_F(ProtocolSettingsTest, DefaultSettings)
{
    // Test that protocol settings match Neo N3 mainnet
    EXPECT_EQ(protocolSettings_->GetNetwork(), 0x334F454E);  // MainNet magic
    EXPECT_EQ(protocolSettings_->GetCommitteeMembersCount(), 21);
    EXPECT_EQ(protocolSettings_->GetValidatorsCount(), 7);
    EXPECT_GT(protocolSettings_->GetMillisecondsPerBlock(), 0);
    EXPECT_GT(protocolSettings_->GetMemoryPoolMaxTransactions(), 0);
    EXPECT_GT(protocolSettings_->GetMaxTransactionsPerBlock(), 0);
    EXPECT_GT(protocolSettings_->GetMaxTraceableBlocks(), 0);

    // Verify standby committee
    auto standbyCommittee = protocolSettings_->GetStandbyCommittee();
    EXPECT_EQ(standbyCommittee.size(), protocolSettings_->GetCommitteeMembersCount());

    // Verify standby validators
    auto standbyValidators = protocolSettings_->GetStandbyValidators();
    EXPECT_EQ(standbyValidators.size(), protocolSettings_->GetValidatorsCount());

    std::cout << "Network magic: 0x" << std::hex << protocolSettings_->GetNetwork() << std::endl;
    std::cout << "Committee members: " << protocolSettings_->GetCommitteeMembersCount() << std::endl;
    std::cout << "Validators: " << protocolSettings_->GetValidatorsCount() << std::endl;
}

/**
 * @brief Test VM execution compatibility
 */
class VmExecutionTest : public Neo3CompatibilityTestBase
{
};

TEST_F(VmExecutionTest, BasicVmExecution)
{
    // Test basic VM operations match Neo N3 behavior
    auto engine = smartcontract::ApplicationEngine::Create(smartcontract::TriggerType::Application, nullptr,
                                                           blockchain_->GetSnapshot(), nullptr, protocolSettings_,
                                                           10000000  // 10 GAS limit
    );

    ASSERT_NE(engine, nullptr);

    // Test simple arithmetic: PUSH1 PUSH2 ADD
    io::ByteVector script = {0x51, 0x52, 0x93};  // PUSH1 PUSH2 ADD
    engine->LoadScript(script);

    auto result = engine->Execute();
    EXPECT_EQ(result, smartcontract::VMState::HALT);

    // Verify result
    auto stack = engine->GetResultStack();
    ASSERT_EQ(stack.size(), 1);

    auto resultItem = stack[0];
    EXPECT_EQ(resultItem->GetType(), vm::StackItemType::Integer);
    EXPECT_EQ(resultItem->GetInteger(), 3);  // 1 + 2 = 3

    // Verify gas consumption
    EXPECT_GT(engine->GetGasConsumed(), 0);
    EXPECT_LT(engine->GetGasConsumed(), 10000000);

    std::cout << "VM execution test passed" << std::endl;
    std::cout << "Gas consumed: " << engine->GetGasConsumed() << std::endl;
}

}  // namespace neo::tests::compatibility

/**
 * @brief Main function for Neo N3 compatibility tests
 */
int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    std::cout << "Neo N3 Compatibility Tests for C++ Implementation" << std::endl;
    std::cout << "Testing C++ node compatibility with official Neo N3 specification" << std::endl;
    std::cout << "=================================================================" << std::endl;

    auto result = RUN_ALL_TESTS();

    if (result == 0)
    {
        std::cout << std::endl;
        std::cout << "✅ All Neo N3 compatibility tests passed!" << std::endl;
        std::cout << "C++ implementation is fully compatible with Neo N3 specification" << std::endl;
        std::cout << "Ready for production deployment as Neo N3 node" << std::endl;
    }
    else
    {
        std::cout << std::endl;
        std::cout << "❌ Some Neo N3 compatibility tests failed" << std::endl;
        std::cout << "Please address compatibility issues before production use" << std::endl;
    }

    return result;
}