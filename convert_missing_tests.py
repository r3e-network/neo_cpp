#!/usr/bin/env python3
"""
Neo C# to C++ Missing Test Converter
Focuses on converting the specific missing tests identified in the coverage report
"""

import os
import re
from pathlib import Path
from typing import Dict, List, Tuple

class MissingTestConverter:
    def __init__(self):
        self.missing_tests = {}
        self.converted_count = 0
        
    def load_missing_tests(self):
        """Load the list of missing tests from the coverage report"""
        missing = {
            'Consensus': [
                'TestConsensusServiceCreation',
                'TestConsensusServiceStart',
                'TestConsensusServiceReceivesBlockchainMessages',
                'TestConsensusServiceHandlesExtensiblePayload',
                'TestConsensusServiceHandlesValidConsensusMessage',
                'TestConsensusServiceRejectsInvalidPayload'
            ],
            'Cryptography': [
                'TestVerifySignature',
                'TestSecp256k1',
                'TestSecp256r1',
                'TestSignatureRecover',
                'TestHashFunction',
                'TestBase58Encoding',
                'TestBloomFilter',
                'TestMerkleTree',
                'TestMurmur32',
                'TestRIPEMD160',
                'TestSCrypt'
            ],
            'IO': [
                'TestGetVarSizeInt',
                'TestGetVarSizeGeneric',
                'TestMemoryReader',
                'TestCaching',
                'TestByteVector',
                'TestBinaryReader',
                'TestBinaryWriter'
            ],
            'Ledger': [
                'TestGetBlock_Genesis',
                'TestGetBlock_NoTransactions',
                'TestGetBlockCount',
                'TestGetBlockHeaderCount',
                'TestGetBlockHeader',
                'TestGetContractState',
                'TestGetRawMemPool'
            ],
            'Network': [
                'TestRemoteNode',
                'TestP2PMessage',
                'TestVersionPayload',
                'TestAddrPayload',
                'TestGetBlocksPayload',
                'TestInvPayload',
                'TestMerkleBlockPayload'
            ],
            'SmartContract': [
                'TestContract',
                'TestManifest',
                'TestNefFile',
                'TestApplicationEngine',
                'TestKeyBuilder',
                'TestNotifyEventArgs',
                'TestStackItem'
            ],
            'Wallet': [
                'TestWallet',
                'TestAccount',
                'TestKeyPair',
                'TestNEP6Wallet',
                'TestWalletAccount',
                'TestAssetDescriptor'
            ]
        }
        return missing
    
    def create_consensus_tests(self):
        """Create missing consensus tests"""
        content = """#include <gtest/gtest.h>
#include <neo/consensus/consensus_service.h>
#include <neo/consensus/consensus_context.h>
#include <neo/network/message.h>
#include <memory>

using namespace neo::consensus;
using namespace neo::network;

class ConsensusServiceTest : public ::testing::Test
{
protected:
    std::unique_ptr<ConsensusService> service;
    std::unique_ptr<ConsensusContext> context;
    
    void SetUp() override
    {
        context = std::make_unique<ConsensusContext>();
        service = std::make_unique<ConsensusService>(context.get());
    }
};

TEST_F(ConsensusServiceTest, TestConsensusServiceCreation)
{
    EXPECT_NE(service, nullptr);
    EXPECT_NE(context, nullptr);
    EXPECT_EQ(service->GetState(), ConsensusState::Initial);
}

TEST_F(ConsensusServiceTest, TestConsensusServiceStart)
{
    EXPECT_TRUE(service->Start());
    EXPECT_EQ(service->GetState(), ConsensusState::Running);
    
    // Should not start twice
    EXPECT_FALSE(service->Start());
}

TEST_F(ConsensusServiceTest, TestConsensusServiceReceivesBlockchainMessages)
{
    service->Start();
    
    // Create a mock blockchain message
    auto message = std::make_shared<Message>(MessageType::Block);
    EXPECT_TRUE(service->ProcessMessage(message));
    
    // Verify message was processed
    EXPECT_GT(service->GetProcessedMessageCount(), 0);
}

TEST_F(ConsensusServiceTest, TestConsensusServiceHandlesExtensiblePayload)
{
    service->Start();
    
    // Create extensible payload
    ExtensiblePayload payload;
    payload.Category = "dBFT";
    payload.ValidBlockStart = 0;
    payload.ValidBlockEnd = 100;
    
    EXPECT_TRUE(service->ProcessExtensiblePayload(payload));
}

TEST_F(ConsensusServiceTest, TestConsensusServiceHandlesValidConsensusMessage)
{
    service->Start();
    
    // Create valid consensus message
    ConsensusMessage msg;
    msg.Type = ConsensusMessageType::PrepareRequest;
    msg.ViewNumber = 0;
    
    EXPECT_TRUE(service->ProcessConsensusMessage(msg));
    EXPECT_EQ(context->GetViewNumber(), msg.ViewNumber);
}

TEST_F(ConsensusServiceTest, TestConsensusServiceRejectsInvalidPayload)
{
    service->Start();
    
    // Create invalid payload (wrong category)
    ExtensiblePayload payload;
    payload.Category = "Invalid";
    
    EXPECT_FALSE(service->ProcessExtensiblePayload(payload));
    
    // Create expired payload
    ExtensiblePayload expiredPayload;
    expiredPayload.Category = "dBFT";
    expiredPayload.ValidBlockEnd = 0; // Expired
    
    EXPECT_FALSE(service->ProcessExtensiblePayload(expiredPayload));
}
"""
        return content
    
    def create_cryptography_tests(self):
        """Create missing cryptography tests"""
        content = """#include <gtest/gtest.h>
#include <neo/cryptography/crypto.h>
#include <neo/cryptography/ecc/ecc_curve.h>
#include <neo/cryptography/base58.h>
#include <neo/cryptography/bloom_filter.h>
#include <neo/cryptography/merkle_tree.h>
#include <neo/cryptography/murmur32.h>
#include <neo/cryptography/ripemd160.h>
#include <neo/cryptography/scrypt.h>
#include <neo/wallets/key_pair.h>
#include <neo/io/byte_vector.h>

using namespace neo::cryptography;
using namespace neo::wallets;
using namespace neo::io;

class CryptoExtendedTest : public ::testing::Test
{
protected:
    KeyPair key;
    
    void SetUp() override
    {
        // Generate test key
        auto privateKey = ByteVector::GenerateRandom(32);
        key = KeyPair(privateKey);
    }
};

TEST_F(CryptoExtendedTest, TestVerifySignature)
{
    ByteVector message = ByteVector::FromString("HelloWorld");
    auto signature = Crypto::Sign(message.AsSpan(), key.GetPrivateKey());
    
    // Valid signature should verify
    EXPECT_TRUE(Crypto::VerifySignature(message.AsSpan(), signature, key.GetPublicKey()));
    
    // Wrong key should fail
    KeyPair wrongKey(ByteVector::GenerateRandom(32));
    EXPECT_FALSE(Crypto::VerifySignature(message.AsSpan(), signature, wrongKey.GetPublicKey()));
    
    // Modified message should fail
    ByteVector wrongMessage = ByteVector::FromString("WrongMessage");
    EXPECT_FALSE(Crypto::VerifySignature(wrongMessage.AsSpan(), signature, key.GetPublicKey()));
}

TEST_F(CryptoExtendedTest, TestSecp256k1)
{
    ByteVector privkey = ByteVector::Parse("7177f0d04c79fa0b8c91fe90c1cf1d44772d1fba6e5eb9b281a22cd3aafb51fe");
    ByteVector message = ByteVector::Parse("2d46a712699bae19a634563d74d04cc2da497b841456da270dccb75ac2f7c4e7");
    
    auto signature = Crypto::Sign(message.AsSpan(), privkey.AsSpan(), ECCurve::Secp256k1);
    
    KeyPair keyPair(privkey, ECCurve::Secp256k1);
    EXPECT_TRUE(Crypto::VerifySignature(message.AsSpan(), signature, keyPair.GetPublicKey(), ECCurve::Secp256k1));
}

TEST_F(CryptoExtendedTest, TestSecp256r1)
{
    ByteVector privkey = ByteVector::GenerateRandom(32);
    ByteVector message = ByteVector::GenerateRandom(32);
    
    auto signature = Crypto::Sign(message.AsSpan(), privkey.AsSpan(), ECCurve::Secp256r1);
    
    KeyPair keyPair(privkey, ECCurve::Secp256r1);
    EXPECT_TRUE(Crypto::VerifySignature(message.AsSpan(), signature, keyPair.GetPublicKey(), ECCurve::Secp256r1));
}

TEST_F(CryptoExtendedTest, TestSignatureRecover)
{
    ByteVector message = ByteVector::GenerateRandom(32);
    auto signature = Crypto::Sign(message.AsSpan(), key.GetPrivateKey());
    
    // Should be able to recover public key from signature
    auto recoveredKey = Crypto::RecoverPublicKey(message.AsSpan(), signature);
    EXPECT_EQ(recoveredKey, key.GetPublicKey());
}

TEST_F(CryptoExtendedTest, TestHashFunction)
{
    ByteVector data = ByteVector::FromString("test data");
    
    // Test SHA256
    auto sha256Hash = Crypto::SHA256(data.AsSpan());
    EXPECT_EQ(sha256Hash.Size(), 32);
    
    // Test RIPEMD160
    auto ripemd160Hash = Crypto::RIPEMD160(data.AsSpan());
    EXPECT_EQ(ripemd160Hash.Size(), 20);
    
    // Test Hash256 (double SHA256)
    auto hash256 = Crypto::Hash256(data.AsSpan());
    EXPECT_EQ(hash256.Size(), 32);
    
    // Test Hash160 (SHA256 + RIPEMD160)
    auto hash160 = Crypto::Hash160(data.AsSpan());
    EXPECT_EQ(hash160.Size(), 20);
}

TEST_F(CryptoExtendedTest, TestBase58Encoding)
{
    ByteVector data = ByteVector::Parse("00112233445566778899aabbccddeeff");
    
    // Encode to Base58
    std::string encoded = Base58::Encode(data.AsSpan());
    EXPECT_FALSE(encoded.empty());
    
    // Decode from Base58
    ByteVector decoded = Base58::Decode(encoded);
    EXPECT_EQ(decoded, data);
    
    // Test with check
    std::string encodedCheck = Base58::EncodeWithCheckSum(data.AsSpan());
    ByteVector decodedCheck = Base58::DecodeWithCheckSum(encodedCheck);
    EXPECT_EQ(decodedCheck, data);
}

TEST_F(CryptoExtendedTest, TestBloomFilter)
{
    BloomFilter filter(100, 3, ByteVector::GenerateRandom(4).ToUInt32());
    
    // Add elements
    ByteVector element1 = ByteVector::FromString("element1");
    ByteVector element2 = ByteVector::FromString("element2");
    ByteVector element3 = ByteVector::FromString("element3");
    
    filter.Add(element1.AsSpan());
    filter.Add(element2.AsSpan());
    
    // Test contains
    EXPECT_TRUE(filter.Contains(element1.AsSpan()));
    EXPECT_TRUE(filter.Contains(element2.AsSpan()));
    EXPECT_FALSE(filter.Contains(element3.AsSpan()));
}

TEST_F(CryptoExtendedTest, TestMerkleTree)
{
    std::vector<ByteVector> hashes;
    hashes.push_back(Crypto::SHA256(ByteVector::FromString("tx1").AsSpan()));
    hashes.push_back(Crypto::SHA256(ByteVector::FromString("tx2").AsSpan()));
    hashes.push_back(Crypto::SHA256(ByteVector::FromString("tx3").AsSpan()));
    hashes.push_back(Crypto::SHA256(ByteVector::FromString("tx4").AsSpan()));
    
    MerkleTree tree(hashes);
    auto root = tree.GetRoot();
    
    EXPECT_FALSE(root.IsEmpty());
    EXPECT_EQ(root.Size(), 32);
    
    // Verify proof
    auto proof = tree.GetProof(1);
    EXPECT_TRUE(tree.VerifyProof(hashes[1], proof, root));
}

TEST_F(CryptoExtendedTest, TestMurmur32)
{
    ByteVector data = ByteVector::FromString("test data");
    uint32_t seed = 0x12345678;
    
    uint32_t hash = Murmur32::Hash(data.AsSpan(), seed);
    EXPECT_NE(hash, 0);
    
    // Same data and seed should produce same hash
    uint32_t hash2 = Murmur32::Hash(data.AsSpan(), seed);
    EXPECT_EQ(hash, hash2);
    
    // Different seed should produce different hash
    uint32_t hash3 = Murmur32::Hash(data.AsSpan(), seed + 1);
    EXPECT_NE(hash, hash3);
}

TEST_F(CryptoExtendedTest, TestRIPEMD160)
{
    ByteVector data = ByteVector::FromString("test data");
    
    auto hash = RIPEMD160::Hash(data.AsSpan());
    EXPECT_EQ(hash.Size(), 20);
    
    // Known test vector
    ByteVector testData = ByteVector::FromString("abc");
    auto testHash = RIPEMD160::Hash(testData.AsSpan());
    EXPECT_EQ(testHash.ToHexString(), "8eb208f7e05d987a9b044a8e98c6b087f15a0bfc");
}

TEST_F(CryptoExtendedTest, TestSCrypt)
{
    ByteVector password = ByteVector::FromString("password");
    ByteVector salt = ByteVector::FromString("salt");
    int N = 16384;
    int r = 8;
    int p = 1;
    int dkLen = 32;
    
    auto key = SCrypt::Generate(password.AsSpan(), salt.AsSpan(), N, r, p, dkLen);
    EXPECT_EQ(key.Size(), dkLen);
    
    // Same parameters should produce same key
    auto key2 = SCrypt::Generate(password.AsSpan(), salt.AsSpan(), N, r, p, dkLen);
    EXPECT_EQ(key, key2);
    
    // Different salt should produce different key
    ByteVector salt2 = ByteVector::FromString("salt2");
    auto key3 = SCrypt::Generate(password.AsSpan(), salt2.AsSpan(), N, r, p, dkLen);
    EXPECT_NE(key, key3);
}
"""
        return content
    
    def create_io_tests(self):
        """Create missing IO tests"""
        content = """#include <gtest/gtest.h>
#include <neo/io/io_helper.h>
#include <neo/io/memory_reader.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/io/caching/cache.h>

using namespace neo::io;

class IOExtendedTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

TEST_F(IOExtendedTest, TestGetVarSizeInt)
{
    // Test various integer sizes
    EXPECT_EQ(IOHelper::GetVarSize(0), 1);
    EXPECT_EQ(IOHelper::GetVarSize(0xFC), 1);
    EXPECT_EQ(IOHelper::GetVarSize(0xFD), 3);
    EXPECT_EQ(IOHelper::GetVarSize(0xFFFF), 3);
    EXPECT_EQ(IOHelper::GetVarSize(0x10000), 5);
    EXPECT_EQ(IOHelper::GetVarSize(0xFFFFFFFF), 5);
    EXPECT_EQ(IOHelper::GetVarSize(0x100000000ULL), 9);
}

TEST_F(IOExtendedTest, TestGetVarSizeGeneric)
{
    // Test string
    std::string str = "Hello World";
    size_t strSize = IOHelper::GetVarSize(str);
    EXPECT_EQ(strSize, 1 + str.length()); // 1 byte for length + string data
    
    // Test vector
    std::vector<uint8_t> vec(100);
    size_t vecSize = IOHelper::GetVarSize(vec);
    EXPECT_EQ(vecSize, 1 + vec.size()); // 1 byte for count + data
    
    // Test large vector
    std::vector<uint8_t> largeVec(300);
    size_t largeVecSize = IOHelper::GetVarSize(largeVec);
    EXPECT_EQ(largeVecSize, 3 + largeVec.size()); // 3 bytes for count + data
}

TEST_F(IOExtendedTest, TestMemoryReader)
{
    ByteVector data;
    data.WriteUInt32(0x12345678);
    data.WriteUInt64(0x123456789ABCDEF0ULL);
    data.WriteString("test");
    
    MemoryReader reader(data);
    
    EXPECT_EQ(reader.ReadUInt32(), 0x12345678);
    EXPECT_EQ(reader.ReadUInt64(), 0x123456789ABCDEF0ULL);
    EXPECT_EQ(reader.ReadString(), "test");
    EXPECT_TRUE(reader.IsEnd());
}

TEST_F(IOExtendedTest, TestCaching)
{
    // Create a simple cache
    Cache<int, std::string> cache(100);
    
    // Add items
    cache.Add(1, "one");
    cache.Add(2, "two");
    cache.Add(3, "three");
    
    // Test contains
    EXPECT_TRUE(cache.Contains(1));
    EXPECT_TRUE(cache.Contains(2));
    EXPECT_TRUE(cache.Contains(3));
    EXPECT_FALSE(cache.Contains(4));
    
    // Test get
    EXPECT_EQ(cache.Get(1), "one");
    EXPECT_EQ(cache.Get(2), "two");
    EXPECT_EQ(cache.Get(3), "three");
    
    // Test remove
    cache.Remove(2);
    EXPECT_FALSE(cache.Contains(2));
}

TEST_F(IOExtendedTest, TestByteVector)
{
    ByteVector vec1 = ByteVector::Parse("0102030405");
    EXPECT_EQ(vec1.Size(), 5);
    EXPECT_EQ(vec1[0], 0x01);
    EXPECT_EQ(vec1[4], 0x05);
    
    ByteVector vec2 = ByteVector::FromString("Hello");
    EXPECT_EQ(vec2.Size(), 5);
    EXPECT_EQ(vec2.ToString(), "Hello");
    
    // Test concatenation
    ByteVector vec3 = vec1 + vec2;
    EXPECT_EQ(vec3.Size(), 10);
    
    // Test comparison
    ByteVector vec4 = ByteVector::Parse("0102030405");
    EXPECT_EQ(vec1, vec4);
    EXPECT_NE(vec1, vec2);
}

TEST_F(IOExtendedTest, TestBinaryReader)
{
    ByteVector data;
    BinaryWriter writer(data);
    writer.Write((uint8_t)0x01);
    writer.Write((uint16_t)0x0203);
    writer.Write((uint32_t)0x04050607);
    writer.Write((uint64_t)0x08090A0B0C0D0E0FULL);
    writer.Write("test string");
    writer.WriteVarInt(1000);
    
    BinaryReader reader(data);
    EXPECT_EQ(reader.ReadByte(), 0x01);
    EXPECT_EQ(reader.ReadUInt16(), 0x0203);
    EXPECT_EQ(reader.ReadUInt32(), 0x04050607);
    EXPECT_EQ(reader.ReadUInt64(), 0x08090A0B0C0D0E0FULL);
    EXPECT_EQ(reader.ReadString(), "test string");
    EXPECT_EQ(reader.ReadVarInt(), 1000);
}

TEST_F(IOExtendedTest, TestBinaryWriter)
{
    ByteVector buffer;
    BinaryWriter writer(buffer);
    
    // Write various types
    writer.Write(true);
    writer.Write((int8_t)-128);
    writer.Write((uint8_t)255);
    writer.Write((int16_t)-32768);
    writer.Write((uint16_t)65535);
    writer.Write((int32_t)-2147483648);
    writer.Write((uint32_t)4294967295U);
    writer.Write((int64_t)-9223372036854775808LL);
    writer.Write((uint64_t)18446744073709551615ULL);
    writer.Write(3.14159f);
    writer.Write(2.71828);
    
    // Verify buffer size
    size_t expectedSize = 1 + 1 + 1 + 2 + 2 + 4 + 4 + 8 + 8 + 4 + 8;
    EXPECT_EQ(buffer.Size(), expectedSize);
    
    // Read back and verify
    BinaryReader reader(buffer);
    EXPECT_TRUE(reader.ReadBoolean());
    EXPECT_EQ(reader.ReadInt8(), -128);
    EXPECT_EQ(reader.ReadByte(), 255);
    EXPECT_EQ(reader.ReadInt16(), -32768);
    EXPECT_EQ(reader.ReadUInt16(), 65535);
    EXPECT_EQ(reader.ReadInt32(), -2147483648);
    EXPECT_EQ(reader.ReadUInt32(), 4294967295U);
    EXPECT_EQ(reader.ReadInt64(), -9223372036854775808LL);
    EXPECT_EQ(reader.ReadUInt64(), 18446744073709551615ULL);
    EXPECT_FLOAT_EQ(reader.ReadFloat(), 3.14159f);
    EXPECT_DOUBLE_EQ(reader.ReadDouble(), 2.71828);
}
"""
        return content
    
    def create_test_files(self):
        """Create all missing test files"""
        print("\n📝 Creating missing test files...")
        
        # Create consensus tests
        consensus_file = "tests/unit/consensus/test_consensus_extended.cpp"
        os.makedirs(os.path.dirname(consensus_file), exist_ok=True)
        with open(consensus_file, 'w') as f:
            f.write(self.create_consensus_tests())
        print(f"  ✅ Created {consensus_file}")
        
        # Create cryptography tests
        crypto_file = "tests/unit/cryptography/test_crypto_extended.cpp"
        os.makedirs(os.path.dirname(crypto_file), exist_ok=True)
        with open(crypto_file, 'w') as f:
            f.write(self.create_cryptography_tests())
        print(f"  ✅ Created {crypto_file}")
        
        # Create IO tests
        io_file = "tests/unit/io/test_io_extended.cpp"
        os.makedirs(os.path.dirname(io_file), exist_ok=True)
        with open(io_file, 'w') as f:
            f.write(self.create_io_tests())
        print(f"  ✅ Created {io_file}")
        
        return 3

def main():
    print("""
╔════════════════════════════════════════════════════════════╗
║      Neo C# to C++ Missing Test Converter v1.0           ║
║        Creating Missing Test Implementations              ║
╚════════════════════════════════════════════════════════════╝
""")
    
    converter = MissingTestConverter()
    
    # Load missing tests
    missing_tests = converter.load_missing_tests()
    
    print("\n📊 Missing Test Summary:")
    total_missing = 0
    for category, tests in missing_tests.items():
        count = len(tests)
        total_missing += count
        print(f"  {category}: {count} tests missing")
    print(f"\n  Total: {total_missing} tests need conversion")
    
    # Create test files
    files_created = converter.create_test_files()
    
    print(f"\n{'='*60}")
    print(f"✅ Created {files_created} test files with core missing tests")
    print(f"📝 These files contain the most critical missing tests")
    print(f"🔧 Additional tests can be added incrementally")
    print(f"{'='*60}")

if __name__ == "__main__":
    main()