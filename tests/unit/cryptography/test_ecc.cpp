#include <gtest/gtest.h>
#include <neo/cryptography/ecc.h>
#include <neo/cryptography/crypto.h>
#include <neo/io/byte_vector.h>

using namespace neo::cryptography;
using namespace neo::cryptography::ecc;
using namespace neo::io;

TEST(ECCTest, Secp256r1GenerateKeyPair)
{
    // Generate a random private key
    ByteVector privateKey = Secp256r1::GeneratePrivateKey();
    
    // Check private key is valid
    EXPECT_TRUE(Secp256r1::IsValidPrivateKey(privateKey));
    EXPECT_EQ(privateKey.Size(), Secp256r1::PRIVATE_KEY_SIZE);
    
    // Generate public key from private key
    ByteVector publicKey = Secp256r1::ComputePublicKey(privateKey);
    
    // Check that the public key is valid
    EXPECT_TRUE(Secp256r1::IsValidPublicKey(publicKey));
    EXPECT_EQ(publicKey.Size(), Secp256r1::PUBLIC_KEY_SIZE); // Compressed public key
    
    // Test with invalid private key size
    ByteVector invalidPrivateKey(10); // Too short
    EXPECT_FALSE(Secp256r1::IsValidPrivateKey(invalidPrivateKey));
}

TEST(ECCTest, Secp256r1SignVerify)
{
    // Generate a key pair
    ByteVector privateKey = Secp256r1::GeneratePrivateKey();
    ByteVector publicKey = Secp256r1::ComputePublicKey(privateKey);
    
    // Message to sign
    ByteVector message = ByteVector::Parse("010203040506070809");
    
    // Sign the message
    ByteVector signature = Secp256r1::Sign(message, privateKey);
    
    // Verify the signature
    bool valid = Secp256r1::Verify(message, signature, publicKey);
    EXPECT_TRUE(valid);
    
    // Verify with a different message
    ByteVector message2 = ByteVector::Parse("0102030405060708");
    bool valid2 = Secp256r1::Verify(message2, signature, publicKey);
    EXPECT_FALSE(valid2);
    
    // Verify with a different signature
    ByteVector signature2 = Secp256r1::Sign(message2, privateKey);
    bool valid3 = Secp256r1::Verify(message, signature2, publicKey);
    EXPECT_FALSE(valid3);
    
    // Verify with a different public key
    ByteVector privateKey2 = Secp256r1::GeneratePrivateKey();
    ByteVector publicKey2 = Secp256r1::ComputePublicKey(privateKey2);
    bool valid4 = Secp256r1::Verify(message, signature, publicKey2);
    EXPECT_FALSE(valid4);
}

TEST(ECCTest, KeyPairClass)
{
    // Create a key pair from a generated private key
    ByteVector privateKey = Secp256r1::GeneratePrivateKey();
    KeyPair keyPair(privateKey);
    
    // Check that keys are valid
    EXPECT_TRUE(Secp256r1::IsValidPrivateKey(keyPair.GetPrivateKey()));
    EXPECT_EQ(keyPair.GetPrivateKey(), privateKey);
    
    // Get public key as ByteVector for validation
    ByteVector publicKeyBytes = keyPair.GetPublicKey().ToBytes(true);
    EXPECT_TRUE(Secp256r1::IsValidPublicKey(publicKeyBytes));
    EXPECT_EQ(publicKeyBytes, Secp256r1::ComputePublicKey(privateKey));
}

TEST(ECCTest, ECPointClass)
{
    // Generate a key pair
    ByteVector privateKey = Secp256r1::GeneratePrivateKey();
    ByteVector publicKeyBytes = Secp256r1::ComputePublicKey(privateKey);
    
    // Parse ECPoint from hex string
    std::string hex = publicKeyBytes.AsSpan().ToHexString();
    ECPoint point = ECPoint::Parse(hex);
    
    // Get encoded bytes
    ByteVector encoded = point.ToBytes(true); // compressed
    EXPECT_EQ(encoded, publicKeyBytes);
    
    // Test with KeyPair
    KeyPair keyPair(privateKey);
    ECPoint point2 = keyPair.GetPublicKey();
    EXPECT_EQ(point2.ToBytes(true), publicKeyBytes);
}

TEST(ECCTest, InvalidOperations)
{
    // Test invalid private key
    ByteVector invalidPrivateKey(10); // Too short
    EXPECT_FALSE(Secp256r1::IsValidPrivateKey(invalidPrivateKey));
    
    // Test invalid public key
    ByteVector invalidPublicKey(20); // Wrong size
    EXPECT_FALSE(Secp256r1::IsValidPublicKey(invalidPublicKey));
    
    // Test signing with invalid private key
    ByteVector message = ByteVector::Parse("010203040506070809");
    EXPECT_THROW(Secp256r1::Sign(message, invalidPrivateKey), std::invalid_argument);
    
    // Test verifying with invalid public key
    ByteVector validPrivateKey = Secp256r1::GeneratePrivateKey();
    ByteVector signature = Secp256r1::Sign(message, validPrivateKey);
    EXPECT_FALSE(Secp256r1::Verify(message, signature, invalidPublicKey));
}