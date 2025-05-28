#include <gtest/gtest.h>
#include <neo/cryptography/bls12_381.h>
#include <neo/cryptography/crypto.h>
#include <neo/io/byte_vector.h>

using namespace neo::cryptography;
using namespace neo::cryptography::bls12_381;
using namespace neo::io;

TEST(BLS12_381Test, G1PointBasic)
{
    // Create a G1Point at infinity
    G1Point p1;
    EXPECT_TRUE(p1.IsInfinity());
    
    // Get the generator point
    G1Point g1 = G1Point::Generator();
    EXPECT_FALSE(g1.IsInfinity());
    
    // Serialize and deserialize
    ByteVector bytes = g1.ToBytes(true);
    EXPECT_EQ(bytes.Size(), G1Point::CompressedSize);
    
    G1Point g1_2 = G1Point(bytes.AsSpan());
    EXPECT_EQ(g1, g1_2);
    
    // Serialize and deserialize (uncompressed)
    ByteVector bytes2 = g1.ToBytes(false);
    EXPECT_EQ(bytes2.Size(), G1Point::UncompressedSize);
    
    G1Point g1_3 = G1Point(bytes2.AsSpan());
    EXPECT_EQ(g1, g1_3);
    
    // FromHex
    std::string hex = g1.ToHex(true);
    G1Point g1_4 = G1Point::FromHex(hex);
    EXPECT_EQ(g1, g1_4);
    
    // Invalid data
    ByteVector invalidData(10);
    EXPECT_THROW(G1Point(invalidData.AsSpan()), std::invalid_argument);
    
    // Invalid hex
    EXPECT_THROW(G1Point::FromHex("invalid"), std::invalid_argument);
}

TEST(BLS12_381Test, G1PointOperations)
{
    // Get the generator point
    G1Point g1 = G1Point::Generator();
    
    // Add two points
    G1Point g1_2 = g1.Add(g1);
    EXPECT_NE(g1, g1_2);
    
    // Multiply by scalar
    ByteVector scalar = ByteVector::Parse("0000000000000000000000000000000000000000000000000000000000000002");
    G1Point g1_3 = g1.Multiply(scalar.AsSpan());
    EXPECT_EQ(g1_2, g1_3);
    
    // Multiply by zero
    ByteVector zero = ByteVector::Parse("0000000000000000000000000000000000000000000000000000000000000000");
    G1Point g1_0 = g1.Multiply(zero.AsSpan());
    EXPECT_TRUE(g1_0.IsInfinity());
    
    // Equality
    EXPECT_TRUE(g1 == g1);
    EXPECT_FALSE(g1 == g1_2);
    EXPECT_TRUE(g1 != g1_2);
    EXPECT_FALSE(g1 != g1);
}

TEST(BLS12_381Test, G2PointBasic)
{
    // Create a G2Point at infinity
    G2Point p2;
    EXPECT_TRUE(p2.IsInfinity());
    
    // Get the generator point
    G2Point g2 = G2Point::Generator();
    EXPECT_FALSE(g2.IsInfinity());
    
    // Serialize and deserialize
    ByteVector bytes = g2.ToBytes(true);
    EXPECT_EQ(bytes.Size(), G2Point::CompressedSize);
    
    G2Point g2_2 = G2Point(bytes.AsSpan());
    EXPECT_EQ(g2, g2_2);
    
    // Serialize and deserialize (uncompressed)
    ByteVector bytes2 = g2.ToBytes(false);
    EXPECT_EQ(bytes2.Size(), G2Point::UncompressedSize);
    
    G2Point g2_3 = G2Point(bytes2.AsSpan());
    EXPECT_EQ(g2, g2_3);
    
    // FromHex
    std::string hex = g2.ToHex(true);
    G2Point g2_4 = G2Point::FromHex(hex);
    EXPECT_EQ(g2, g2_4);
    
    // Invalid data
    ByteVector invalidData(10);
    EXPECT_THROW(G2Point(invalidData.AsSpan()), std::invalid_argument);
    
    // Invalid hex
    EXPECT_THROW(G2Point::FromHex("invalid"), std::invalid_argument);
}

TEST(BLS12_381Test, G2PointOperations)
{
    // Get the generator point
    G2Point g2 = G2Point::Generator();
    
    // Add two points
    G2Point g2_2 = g2.Add(g2);
    EXPECT_NE(g2, g2_2);
    
    // Multiply by scalar
    ByteVector scalar = ByteVector::Parse("0000000000000000000000000000000000000000000000000000000000000002");
    G2Point g2_3 = g2.Multiply(scalar.AsSpan());
    EXPECT_EQ(g2_2, g2_3);
    
    // Multiply by zero
    ByteVector zero = ByteVector::Parse("0000000000000000000000000000000000000000000000000000000000000000");
    G2Point g2_0 = g2.Multiply(zero.AsSpan());
    EXPECT_TRUE(g2_0.IsInfinity());
    
    // Equality
    EXPECT_TRUE(g2 == g2);
    EXPECT_FALSE(g2 == g2_2);
    EXPECT_TRUE(g2 != g2_2);
    EXPECT_FALSE(g2 != g2);
}

TEST(BLS12_381Test, GTPointBasic)
{
    // Create a GTPoint at identity
    GTPoint gt;
    EXPECT_TRUE(gt.IsIdentity());
    
    // Serialize and deserialize
    ByteVector bytes = gt.ToBytes();
    EXPECT_EQ(bytes.Size(), GTPoint::Size);
    
    GTPoint gt_2 = GTPoint(bytes.AsSpan());
    EXPECT_EQ(gt, gt_2);
    
    // FromHex
    std::string hex = gt.ToHex();
    GTPoint gt_3 = GTPoint::FromHex(hex);
    EXPECT_EQ(gt, gt_3);
    
    // Invalid data
    ByteVector invalidData(10);
    EXPECT_THROW(GTPoint(invalidData.AsSpan()), std::invalid_argument);
    
    // Invalid hex
    EXPECT_THROW(GTPoint::FromHex("invalid"), std::invalid_argument);
}

TEST(BLS12_381Test, Pairing)
{
    // Get the generator points
    G1Point g1 = G1Point::Generator();
    G2Point g2 = G2Point::Generator();
    
    // Compute the pairing
    GTPoint gt = Pairing(g1, g2);
    EXPECT_FALSE(gt.IsIdentity());
    
    // Pairing with infinity
    G1Point inf1;
    GTPoint gt_inf = Pairing(inf1, g2);
    EXPECT_TRUE(gt_inf.IsIdentity());
    
    // Pairing with infinity
    G2Point inf2;
    GTPoint gt_inf2 = Pairing(g1, inf2);
    EXPECT_TRUE(gt_inf2.IsIdentity());
    
    // Multi-pairing
    std::vector<G1Point> ps = {g1, g1};
    std::vector<G2Point> qs = {g2, g2};
    GTPoint gt_multi = MultiPairing(ps, qs);
    EXPECT_FALSE(gt_multi.IsIdentity());
    
    // Multi-pairing with empty vectors
    std::vector<G1Point> empty_ps;
    std::vector<G2Point> empty_qs;
    GTPoint gt_empty = MultiPairing(empty_ps, empty_qs);
    EXPECT_TRUE(gt_empty.IsIdentity());
    
    // Multi-pairing with different sizes
    std::vector<G1Point> ps2 = {g1};
    std::vector<G2Point> qs2 = {g2, g2};
    EXPECT_THROW(MultiPairing(ps2, qs2), std::invalid_argument);
}

TEST(BLS12_381Test, GTPointOperations)
{
    // Get the generator points
    G1Point g1 = G1Point::Generator();
    G2Point g2 = G2Point::Generator();
    
    // Compute the pairing
    GTPoint gt = Pairing(g1, g2);
    
    // Multiply two points
    GTPoint gt_2 = gt.Multiply(gt);
    EXPECT_NE(gt, gt_2);
    
    // Pow
    ByteVector scalar = ByteVector::Parse("0000000000000000000000000000000000000000000000000000000000000002");
    GTPoint gt_3 = gt.Pow(scalar.AsSpan());
    EXPECT_EQ(gt_2, gt_3);
    
    // Pow with zero
    ByteVector zero = ByteVector::Parse("0000000000000000000000000000000000000000000000000000000000000000");
    GTPoint gt_0 = gt.Pow(zero.AsSpan());
    EXPECT_TRUE(gt_0.IsIdentity());
    
    // Equality
    EXPECT_TRUE(gt == gt);
    EXPECT_FALSE(gt == gt_2);
    EXPECT_TRUE(gt != gt_2);
    EXPECT_FALSE(gt != gt);
}

TEST(BLS12_381Test, Signature)
{
    // Generate a private key
    ByteVector privateKey = Crypto::GenerateRandomBytes(32);
    
    // Generate a public key
    G2Point publicKey = GeneratePublicKey(privateKey.AsSpan());
    
    // Message to sign
    ByteVector message = ByteVector::Parse("010203040506070809");
    
    // Sign the message
    G1Point signature = Sign(privateKey.AsSpan(), message.AsSpan());
    
    // Verify the signature
    bool valid = VerifySignature(publicKey, message.AsSpan(), signature);
    EXPECT_TRUE(valid);
    
    // Verify with a different message
    ByteVector message2 = ByteVector::Parse("0102030405060708");
    bool valid2 = VerifySignature(publicKey, message2.AsSpan(), signature);
    EXPECT_FALSE(valid2);
    
    // Verify with a different signature
    G1Point signature2 = Sign(privateKey.AsSpan(), message2.AsSpan());
    bool valid3 = VerifySignature(publicKey, message.AsSpan(), signature2);
    EXPECT_FALSE(valid3);
    
    // Verify with a different public key
    ByteVector privateKey2 = Crypto::GenerateRandomBytes(32);
    G2Point publicKey2 = GeneratePublicKey(privateKey2.AsSpan());
    bool valid4 = VerifySignature(publicKey2, message.AsSpan(), signature);
    EXPECT_FALSE(valid4);
}

TEST(BLS12_381Test, AggregateSignature)
{
    // Generate private keys
    ByteVector privateKey1 = Crypto::GenerateRandomBytes(32);
    ByteVector privateKey2 = Crypto::GenerateRandomBytes(32);
    
    // Generate public keys
    G2Point publicKey1 = GeneratePublicKey(privateKey1.AsSpan());
    G2Point publicKey2 = GeneratePublicKey(privateKey2.AsSpan());
    
    // Messages to sign
    ByteVector message1 = ByteVector::Parse("0102030405060708");
    ByteVector message2 = ByteVector::Parse("1112131415161718");
    
    // Sign the messages
    G1Point signature1 = Sign(privateKey1.AsSpan(), message1.AsSpan());
    G1Point signature2 = Sign(privateKey2.AsSpan(), message2.AsSpan());
    
    // Aggregate the signatures
    G1Point aggregateSignature = AggregateSignatures({signature1, signature2});
    
    // Verify the aggregate signature
    bool valid = VerifyAggregateSignature({publicKey1, publicKey2}, {message1.AsSpan(), message2.AsSpan()}, aggregateSignature);
    EXPECT_TRUE(valid);
    
    // Verify with different messages
    bool valid2 = VerifyAggregateSignature({publicKey1, publicKey2}, {message2.AsSpan(), message1.AsSpan()}, aggregateSignature);
    EXPECT_FALSE(valid2);
    
    // Verify with different public keys
    bool valid3 = VerifyAggregateSignature({publicKey2, publicKey1}, {message1.AsSpan(), message2.AsSpan()}, aggregateSignature);
    EXPECT_FALSE(valid3);
    
    // Verify with different signature
    G1Point aggregateSignature2 = AggregateSignatures({signature2, signature1});
    bool valid4 = VerifyAggregateSignature({publicKey1, publicKey2}, {message1.AsSpan(), message2.AsSpan()}, aggregateSignature2);
    EXPECT_FALSE(valid4);
    
    // Verify with empty vectors
    EXPECT_THROW(AggregateSignatures({}), std::invalid_argument);
    EXPECT_THROW(VerifyAggregateSignature({publicKey1}, {}, aggregateSignature), std::invalid_argument);
}
