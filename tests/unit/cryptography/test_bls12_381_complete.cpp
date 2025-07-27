#include <gtest/gtest.h>
#include <neo/cryptography/bls12_381.h>
#include <neo/cryptography/crypto.h>
#include <neo/cryptography/hash.h>
#include <neo/io/byte_vector.h>
#include <chrono>
#include <vector>

using namespace neo::cryptography;
using namespace neo::cryptography::bls12_381;
using namespace neo::io;

class BLS12381CompleteTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Any setup needed
    }

    void TearDown() override {
        // Any cleanup needed
    }
};

// Test G1Point construction and serialization
TEST_F(BLS12381CompleteTest, G1PointConstruction) {
    // Test infinity point
    G1Point infinity;
    EXPECT_TRUE(infinity.IsInfinity());
    
    // Test generator
    G1Point generator = G1Point::Generator();
    EXPECT_FALSE(generator.IsInfinity());
    
    // Test compressed serialization
    ByteVector compressed = generator.ToBytes(true);
    EXPECT_EQ(compressed.Size(), 48);
    EXPECT_TRUE((compressed[0] & 0x80) != 0); // Compression flag set
    
    // Test uncompressed serialization
    ByteVector uncompressed = generator.ToBytes(false);
    EXPECT_EQ(uncompressed.Size(), 96);
    EXPECT_FALSE((uncompressed[0] & 0x80) != 0); // No compression flag
    
    // Test deserialization
    G1Point fromCompressed(compressed.AsSpan());
    G1Point fromUncompressed(uncompressed.AsSpan());
    EXPECT_EQ(generator, fromCompressed);
    EXPECT_EQ(generator, fromUncompressed);
    
    // Test hex conversion
    std::string hex = generator.ToHex(true);
    G1Point fromHex = G1Point::FromHex(hex);
    EXPECT_EQ(generator, fromHex);
}

// Test G1Point arithmetic operations
TEST_F(BLS12381CompleteTest, G1PointArithmetic) {
    G1Point g = G1Point::Generator();
    G1Point infinity;
    
    // Test addition with infinity
    EXPECT_EQ(g.Add(infinity), g);
    EXPECT_EQ(infinity.Add(g), g);
    EXPECT_EQ(infinity.Add(infinity), infinity);
    
    // Test point doubling
    G1Point doubled = g.Add(g);
    EXPECT_NE(doubled, g);
    EXPECT_FALSE(doubled.IsInfinity());
    
    // Test scalar multiplication
    ByteVector scalar2 = ByteVector(32);
    scalar2[31] = 2; // Little-endian 2
    G1Point multiplied = g.Multiply(scalar2.AsSpan());
    EXPECT_EQ(multiplied, doubled);
    
    // Test multiplication by zero
    ByteVector scalar0 = ByteVector(32); // All zeros
    G1Point zero = g.Multiply(scalar0.AsSpan());
    EXPECT_TRUE(zero.IsInfinity());
    
    // Test multiplication by one
    ByteVector scalar1 = ByteVector(32);
    scalar1[31] = 1;
    G1Point one = g.Multiply(scalar1.AsSpan());
    EXPECT_EQ(one, g);
}

// Test G2Point construction and serialization
TEST_F(BLS12381CompleteTest, G2PointConstruction) {
    // Test infinity point
    G2Point infinity;
    EXPECT_TRUE(infinity.IsInfinity());
    
    // Test generator
    G2Point generator = G2Point::Generator();
    EXPECT_FALSE(generator.IsInfinity());
    
    // Test compressed serialization
    ByteVector compressed = generator.ToBytes(true);
    EXPECT_EQ(compressed.Size(), 96);
    EXPECT_TRUE((compressed[0] & 0x80) != 0); // Compression flag set
    
    // Test uncompressed serialization
    ByteVector uncompressed = generator.ToBytes(false);
    EXPECT_EQ(uncompressed.Size(), 192);
    EXPECT_FALSE((uncompressed[0] & 0x80) != 0); // No compression flag
    
    // Test deserialization
    G2Point fromCompressed(compressed.AsSpan());
    G2Point fromUncompressed(uncompressed.AsSpan());
    EXPECT_EQ(generator, fromCompressed);
    EXPECT_EQ(generator, fromUncompressed);
    
    // Test hex conversion
    std::string hex = generator.ToHex(true);
    G2Point fromHex = G2Point::FromHex(hex);
    EXPECT_EQ(generator, fromHex);
}

// Test G2Point arithmetic operations
TEST_F(BLS12381CompleteTest, G2PointArithmetic) {
    G2Point g = G2Point::Generator();
    G2Point infinity;
    
    // Test addition with infinity
    EXPECT_EQ(g.Add(infinity), g);
    EXPECT_EQ(infinity.Add(g), g);
    EXPECT_EQ(infinity.Add(infinity), infinity);
    
    // Test point doubling
    G2Point doubled = g.Add(g);
    EXPECT_NE(doubled, g);
    EXPECT_FALSE(doubled.IsInfinity());
    
    // Test scalar multiplication
    ByteVector scalar2 = ByteVector(32);
    scalar2[31] = 2;
    G2Point multiplied = g.Multiply(scalar2.AsSpan());
    EXPECT_EQ(multiplied, doubled);
    
    // Test multiplication by zero
    ByteVector scalar0 = ByteVector(32);
    G2Point zero = g.Multiply(scalar0.AsSpan());
    EXPECT_TRUE(zero.IsInfinity());
}

// Test GTPoint construction and operations
TEST_F(BLS12381CompleteTest, GTPointOperations) {
    // Test identity element
    GTPoint identity;
    EXPECT_TRUE(identity.IsIdentity());
    
    // Test serialization
    ByteVector bytes = identity.ToBytes();
    EXPECT_EQ(bytes.Size(), 576);
    
    // Test deserialization
    GTPoint fromBytes(bytes.AsSpan());
    EXPECT_EQ(identity, fromBytes);
    
    // Test hex conversion
    std::string hex = identity.ToHex();
    GTPoint fromHex = GTPoint::FromHex(hex);
    EXPECT_EQ(identity, fromHex);
}

// Test pairing operations
TEST_F(BLS12381CompleteTest, PairingOperations) {
    G1Point g1 = G1Point::Generator();
    G2Point g2 = G2Point::Generator();
    
    // Test basic pairing
    GTPoint e_g1_g2 = Pairing(g1, g2);
    EXPECT_FALSE(e_g1_g2.IsIdentity());
    
    // Test pairing with infinity
    G1Point inf1;
    G2Point inf2;
    GTPoint e_inf1 = Pairing(inf1, g2);
    GTPoint e_inf2 = Pairing(g1, inf2);
    EXPECT_TRUE(e_inf1.IsIdentity());
    EXPECT_TRUE(e_inf2.IsIdentity());
    
    // Test multi-pairing
    std::vector<G1Point> g1s = {g1, g1};
    std::vector<G2Point> g2s = {g2, g2};
    GTPoint multi = MultiPairing(g1s, g2s);
    EXPECT_FALSE(multi.IsIdentity());
    
    // Test GT multiplication
    GTPoint product = e_g1_g2.Multiply(e_g1_g2);
    EXPECT_NE(product, e_g1_g2);
    
    // Test GT exponentiation
    ByteVector exp = ByteVector(32);
    exp[31] = 2;
    GTPoint power = e_g1_g2.Pow(exp.AsSpan());
    EXPECT_EQ(power, product);
}

// Test BLS signature scheme
TEST_F(BLS12381CompleteTest, BLSSignatures) {
    // Generate key pair
    ByteVector privateKey = Crypto::GenerateRandomBytes(32);
    G2Point publicKey = GeneratePublicKey(privateKey.AsSpan());
    
    // Test message signing and verification
    ByteVector message = ByteVector::Parse("48656c6c6f20426c6f636b636861696e"); // "Hello Blockchain"
    G1Point signature = Sign(privateKey.AsSpan(), message.AsSpan());
    
    // Verify correct signature
    EXPECT_TRUE(VerifySignature(publicKey, message.AsSpan(), signature));
    
    // Verify with wrong message
    ByteVector wrongMessage = ByteVector::Parse("48656c6c6f20576f726c64"); // "Hello World"
    EXPECT_FALSE(VerifySignature(publicKey, wrongMessage.AsSpan(), signature));
    
    // Verify with wrong public key
    ByteVector wrongPrivateKey = Crypto::GenerateRandomBytes(32);
    G2Point wrongPublicKey = GeneratePublicKey(wrongPrivateKey.AsSpan());
    EXPECT_FALSE(VerifySignature(wrongPublicKey, message.AsSpan(), signature));
}

// Test aggregate signatures
TEST_F(BLS12381CompleteTest, AggregateSignatures) {
    // Generate multiple key pairs
    std::vector<ByteVector> privateKeys;
    std::vector<G2Point> publicKeys;
    std::vector<ByteVector> messages;
    std::vector<G1Point> signatures;
    
    for (int i = 0; i < 3; i++) {
        privateKeys.push_back(Crypto::GenerateRandomBytes(32));
        publicKeys.push_back(GeneratePublicKey(privateKeys[i].AsSpan()));
        
        ByteVector msg(32);
        msg[0] = static_cast<uint8_t>(i);
        messages.push_back(msg);
        
        signatures.push_back(Sign(privateKeys[i].AsSpan(), messages[i].AsSpan()));
    }
    
    // Aggregate signatures
    G1Point aggregateSig = AggregateSignatures(signatures);
    
    // Verify aggregate signature
    std::vector<io::ByteSpan> messageSpans;
    for (const auto& msg : messages) {
        messageSpans.push_back(msg.AsSpan());
    }
    
    EXPECT_TRUE(VerifyAggregateSignature(publicKeys, messageSpans, aggregateSig));
    
    // Verify with wrong order
    std::vector<G2Point> wrongOrderKeys = {publicKeys[1], publicKeys[0], publicKeys[2]};
    EXPECT_FALSE(VerifyAggregateSignature(wrongOrderKeys, messageSpans, aggregateSig));
    
    // Test empty signature aggregation
    EXPECT_THROW(AggregateSignatures({}), std::invalid_argument);
}

// Test hash-to-curve functionality
TEST_F(BLS12381CompleteTest, HashToG1) {
    // Test basic hash-to-curve
    ByteVector message1 = ByteVector::Parse("746573745f6d657373616765"); // "test_message"
    G1Point point1 = HashToG1(message1.AsSpan());
    EXPECT_FALSE(point1.IsInfinity());
    
    // Test different messages produce different points
    ByteVector message2 = ByteVector::Parse("6f746865725f6d657373616765"); // "other_message"
    G1Point point2 = HashToG1(message2.AsSpan());
    EXPECT_FALSE(point2.IsInfinity());
    EXPECT_NE(point1, point2);
    
    // Test same message produces same point
    G1Point point1_again = HashToG1(message1.AsSpan());
    EXPECT_EQ(point1, point1_again);
}

// Test helper functions
TEST_F(BLS12381CompleteTest, HelperFunctions) {
    // Test GetG2Generator
    G2Point gen = GetG2Generator();
    EXPECT_EQ(gen, G2Point::Generator());
    
    // Test NegateG2
    G2Point g2 = G2Point::Generator();
    G2Point neg = NegateG2(g2);
    EXPECT_NE(neg, g2);
    
    // Negating infinity should return infinity
    G2Point inf;
    G2Point negInf = NegateG2(inf);
    EXPECT_TRUE(negInf.IsInfinity());
    
    // Test MultiplyGT
    G1Point g1 = G1Point::Generator();
    GTPoint gt = Pairing(g1, g2);
    GTPoint product = MultiplyGT(gt, gt);
    EXPECT_EQ(product, gt.Multiply(gt));
    
    // Test IsIdentityGT
    GTPoint identity;
    EXPECT_TRUE(IsIdentityGT(identity));
    EXPECT_FALSE(IsIdentityGT(gt));
    
    // Test deserialization functions
    ByteVector g1Bytes = g1.ToBytes(true);
    ByteVector g2Bytes = g2.ToBytes(true);
    
    G1Point deserializedG1;
    G2Point deserializedG2;
    
    EXPECT_TRUE(DeserializeG1Point(g1Bytes.AsSpan(), deserializedG1));
    EXPECT_EQ(deserializedG1, g1);
    
    EXPECT_TRUE(DeserializeG2Point(g2Bytes.AsSpan(), deserializedG2));
    EXPECT_EQ(deserializedG2, g2);
    
    // Test invalid deserialization
    ByteVector invalidData(10); // Too small
    G1Point invalidG1;
    G2Point invalidG2;
    
    EXPECT_FALSE(DeserializeG1Point(invalidData.AsSpan(), invalidG1));
    EXPECT_FALSE(DeserializeG2Point(invalidData.AsSpan(), invalidG2));
}

// Test edge cases and error handling
TEST_F(BLS12381CompleteTest, EdgeCases) {
    // Test invalid data sizes
    ByteVector tooSmall(10);
    EXPECT_THROW(G1Point(tooSmall.AsSpan()), std::invalid_argument);
    EXPECT_THROW(G2Point(tooSmall.AsSpan()), std::invalid_argument);
    
    ByteVector wrongSize(500);
    EXPECT_THROW(GTPoint(wrongSize.AsSpan()), std::invalid_argument);
    
    // Test mismatched vector sizes in multi-pairing
    std::vector<G1Point> g1s = {G1Point::Generator()};
    std::vector<G2Point> g2s = {G2Point::Generator(), G2Point::Generator()};
    EXPECT_THROW(MultiPairing(g1s, g2s), std::invalid_argument);
    
    // Test mismatched sizes in aggregate signature verification
    std::vector<G2Point> pubKeys = {G2Point::Generator()};
    std::vector<io::ByteSpan> messages;
    G1Point sig = G1Point::Generator();
    EXPECT_THROW(VerifyAggregateSignature(pubKeys, messages, sig), std::invalid_argument);
}

// Performance benchmarks (disabled by default)
TEST_F(BLS12381CompleteTest, DISABLED_PerformanceBenchmarks) {
    const int iterations = 100;
    
    // Benchmark G1 scalar multiplication
    G1Point g1 = G1Point::Generator();
    ByteVector scalar = Crypto::GenerateRandomBytes(32);
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        G1Point result = g1.Multiply(scalar.AsSpan());
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "G1 scalar multiplication: " 
              << duration.count() / iterations << " µs per operation" << std::endl;
    
    // Benchmark pairing
    G2Point g2 = G2Point::Generator();
    
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        GTPoint result = Pairing(g1, g2);
    }
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Pairing: " 
              << duration.count() / iterations << " µs per operation" << std::endl;
    
    // Benchmark signature verification
    ByteVector privateKey = Crypto::GenerateRandomBytes(32);
    G2Point publicKey = GeneratePublicKey(privateKey.AsSpan());
    ByteVector message = Crypto::GenerateRandomBytes(32);
    G1Point signature = Sign(privateKey.AsSpan(), message.AsSpan());
    
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        bool valid = VerifySignature(publicKey, message.AsSpan(), signature);
    }
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Signature verification: " 
              << duration.count() / iterations << " µs per operation" << std::endl;
}

// Test Neo protocol compatibility
TEST_F(BLS12381CompleteTest, NeoProtocolCompatibility) {
    // Test that serialization formats match Neo protocol expectations
    G1Point g1 = G1Point::Generator();
    G2Point g2 = G2Point::Generator();
    
    // Compressed sizes must match Neo protocol
    EXPECT_EQ(g1.ToBytes(true).Size(), 48);
    EXPECT_EQ(g2.ToBytes(true).Size(), 96);
    
    // Uncompressed sizes must match Neo protocol
    EXPECT_EQ(g1.ToBytes(false).Size(), 96);
    EXPECT_EQ(g2.ToBytes(false).Size(), 192);
    
    // GT size must match Neo protocol
    GTPoint gt = Pairing(g1, g2);
    EXPECT_EQ(gt.ToBytes().Size(), 576);
    
    // Test infinity point encoding
    G1Point inf1;
    ByteVector infBytes1 = inf1.ToBytes(true);
    EXPECT_EQ(infBytes1[0] & 0xC0, 0xC0); // Compression + infinity flags
    
    G2Point inf2;
    ByteVector infBytes2 = inf2.ToBytes(true);
    EXPECT_EQ(infBytes2[0] & 0xC0, 0xC0); // Compression + infinity flags
}

// Test field arithmetic consistency
TEST_F(BLS12381CompleteTest, FieldArithmeticConsistency) {
    // Test that point operations maintain field properties
    G1Point g = G1Point::Generator();
    
    // Test associativity: (g + g) + g = g + (g + g)
    G1Point left = g.Add(g).Add(g);
    G1Point right = g.Add(g.Add(g));
    EXPECT_EQ(left, right);
    
    // Test scalar multiplication distributivity
    ByteVector scalar1 = ByteVector(32);
    ByteVector scalar2 = ByteVector(32);
    scalar1[31] = 3;
    scalar2[31] = 4;
    
    // 3g + 4g should equal 7g
    G1Point sum = g.Multiply(scalar1.AsSpan()).Add(g.Multiply(scalar2.AsSpan()));
    
    ByteVector scalar7 = ByteVector(32);
    scalar7[31] = 7;
    G1Point product = g.Multiply(scalar7.AsSpan());
    
    EXPECT_EQ(sum, product);
}

// Test extension methods
TEST_F(BLS12381CompleteTest, ExtensionMethods) {
    // Test G1PointDouble
    G1Point g = G1Point::Generator();
    G1Point doubled1 = G1PointDouble(g);
    G1Point doubled2 = g.Add(g);
    EXPECT_EQ(doubled1, doubled2);
    
    // Test G1PointNegate
    G1Point neg = G1PointNegate(g);
    EXPECT_NE(neg, g);
    
    // Test GTPointIdentity
    GTPoint identity = GTPointIdentity();
    EXPECT_TRUE(identity.IsIdentity());
}