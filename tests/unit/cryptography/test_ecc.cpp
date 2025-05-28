#include <gtest/gtest.h>
#include <neo/cryptography/ecc.h>
#include <neo/cryptography/crypto.h>
#include <neo/io/byte_vector.h>

using namespace neo::cryptography;
using namespace neo::io;

TEST(ECCTest, Secp256r1GenerateKeyPair)
{
    // Create a curve
    auto curve = ECCurve::GetCurve("secp256r1");
    
    // Generate a random private key
    ByteVector privateKey = Crypto::GenerateRandomBytes(32);
    
    // Generate a key pair
    ECPoint publicKey = curve->GenerateKeyPair(privateKey.AsSpan());
    
    // Check that the public key is valid
    EXPECT_FALSE(publicKey.IsInfinity());
    EXPECT_EQ(publicKey.GetCurveName(), "secp256r1");
    
    // Check that the public key can be serialized and deserialized
    ByteVector publicKeyBytes = publicKey.ToBytes(true);
    EXPECT_EQ(publicKeyBytes.Size(), 33); // Compressed public key
    
    ECPoint publicKey2 = ECPoint::FromBytes(publicKeyBytes.AsSpan(), "secp256r1");
    EXPECT_EQ(publicKey, publicKey2);
    
    // Check that the public key can be serialized and deserialized in uncompressed format
    ByteVector publicKeyBytes2 = publicKey.ToBytes(false);
    EXPECT_EQ(publicKeyBytes2.Size(), 65); // Uncompressed public key
    
    ECPoint publicKey3 = ECPoint::FromBytes(publicKeyBytes2.AsSpan(), "secp256r1");
    EXPECT_EQ(publicKey, publicKey3);
    
    // Invalid private key size
    ByteVector invalidPrivateKey = Crypto::GenerateRandomBytes(16);
    EXPECT_THROW(curve->GenerateKeyPair(invalidPrivateKey.AsSpan()), std::invalid_argument);
}

TEST(ECCTest, Secp256r1SignVerify)
{
    // Create a curve
    auto curve = ECCurve::GetCurve("secp256r1");
    
    // Generate a random private key
    ByteVector privateKey = Crypto::GenerateRandomBytes(32);
    
    // Generate a key pair
    ECPoint publicKey = curve->GenerateKeyPair(privateKey.AsSpan());
    
    // Message to sign
    ByteVector message = ByteVector::Parse("010203040506070809");
    
    // Sign the message
    ByteVector signature = curve->Sign(message.AsSpan(), privateKey.AsSpan());
    
    // Verify the signature
    bool valid = curve->Verify(message.AsSpan(), signature.AsSpan(), publicKey);
    EXPECT_TRUE(valid);
    
    // Verify with a different message
    ByteVector message2 = ByteVector::Parse("0102030405060708");
    bool valid2 = curve->Verify(message2.AsSpan(), signature.AsSpan(), publicKey);
    EXPECT_FALSE(valid2);
    
    // Verify with a different signature
    ByteVector signature2 = curve->Sign(message2.AsSpan(), privateKey.AsSpan());
    bool valid3 = curve->Verify(message.AsSpan(), signature2.AsSpan(), publicKey);
    EXPECT_FALSE(valid3);
    
    // Verify with a different public key
    ByteVector privateKey2 = Crypto::GenerateRandomBytes(32);
    ECPoint publicKey2 = curve->GenerateKeyPair(privateKey2.AsSpan());
    bool valid4 = curve->Verify(message.AsSpan(), signature.AsSpan(), publicKey2);
    EXPECT_FALSE(valid4);
}

TEST(ECCTest, Secp256k1GenerateKeyPair)
{
    // Create a curve
    auto curve = ECCurve::GetCurve("secp256k1");
    
    // Generate a random private key
    ByteVector privateKey = Crypto::GenerateRandomBytes(32);
    
    // Generate a key pair
    ECPoint publicKey = curve->GenerateKeyPair(privateKey.AsSpan());
    
    // Check that the public key is valid
    EXPECT_FALSE(publicKey.IsInfinity());
    EXPECT_EQ(publicKey.GetCurveName(), "secp256k1");
    
    // Check that the public key can be serialized and deserialized
    ByteVector publicKeyBytes = publicKey.ToBytes(true);
    EXPECT_EQ(publicKeyBytes.Size(), 33); // Compressed public key
    
    ECPoint publicKey2 = ECPoint::FromBytes(publicKeyBytes.AsSpan(), "secp256k1");
    EXPECT_EQ(publicKey, publicKey2);
    
    // Check that the public key can be serialized and deserialized in uncompressed format
    ByteVector publicKeyBytes2 = publicKey.ToBytes(false);
    EXPECT_EQ(publicKeyBytes2.Size(), 65); // Uncompressed public key
    
    ECPoint publicKey3 = ECPoint::FromBytes(publicKeyBytes2.AsSpan(), "secp256k1");
    EXPECT_EQ(publicKey, publicKey3);
    
    // Invalid private key size
    ByteVector invalidPrivateKey = Crypto::GenerateRandomBytes(16);
    EXPECT_THROW(curve->GenerateKeyPair(invalidPrivateKey.AsSpan()), std::invalid_argument);
}

TEST(ECCTest, Secp256k1SignVerify)
{
    // Create a curve
    auto curve = ECCurve::GetCurve("secp256k1");
    
    // Generate a random private key
    ByteVector privateKey = Crypto::GenerateRandomBytes(32);
    
    // Generate a key pair
    ECPoint publicKey = curve->GenerateKeyPair(privateKey.AsSpan());
    
    // Message to sign
    ByteVector message = ByteVector::Parse("010203040506070809");
    
    // Sign the message
    ByteVector signature = curve->Sign(message.AsSpan(), privateKey.AsSpan());
    
    // Verify the signature
    bool valid = curve->Verify(message.AsSpan(), signature.AsSpan(), publicKey);
    EXPECT_TRUE(valid);
    
    // Verify with a different message
    ByteVector message2 = ByteVector::Parse("0102030405060708");
    bool valid2 = curve->Verify(message2.AsSpan(), signature.AsSpan(), publicKey);
    EXPECT_FALSE(valid2);
    
    // Verify with a different signature
    ByteVector signature2 = curve->Sign(message2.AsSpan(), privateKey.AsSpan());
    bool valid3 = curve->Verify(message.AsSpan(), signature2.AsSpan(), publicKey);
    EXPECT_FALSE(valid3);
    
    // Verify with a different public key
    ByteVector privateKey2 = Crypto::GenerateRandomBytes(32);
    ECPoint publicKey2 = curve->GenerateKeyPair(privateKey2.AsSpan());
    bool valid4 = curve->Verify(message.AsSpan(), signature.AsSpan(), publicKey2);
    EXPECT_FALSE(valid4);
}

TEST(ECCTest, ECPointFromHex)
{
    // Create a curve
    auto curve = ECCurve::GetCurve("secp256r1");
    
    // Generate a random private key
    ByteVector privateKey = Crypto::GenerateRandomBytes(32);
    
    // Generate a key pair
    ECPoint publicKey = curve->GenerateKeyPair(privateKey.AsSpan());
    
    // Convert to hex
    std::string hex = publicKey.ToHex(true);
    
    // Parse from hex
    ECPoint publicKey2 = ECPoint::FromHex(hex, "secp256r1");
    
    // Check that the public keys are equal
    EXPECT_EQ(publicKey, publicKey2);
    
    // Invalid hex
    EXPECT_THROW(ECPoint::FromHex("invalid", "secp256r1"), std::invalid_argument);
    
    // Invalid curve
    EXPECT_THROW(ECPoint::FromHex(hex, "invalid"), std::invalid_argument);
}

TEST(ECCTest, GetCurve)
{
    // Get a curve by name
    auto curve1 = ECCurve::GetCurve("secp256r1");
    EXPECT_EQ(curve1->GetName(), "secp256r1");
    
    auto curve2 = ECCurve::GetCurve("secp256k1");
    EXPECT_EQ(curve2->GetName(), "secp256k1");
    
    // Invalid curve name
    EXPECT_THROW(ECCurve::GetCurve("invalid"), std::invalid_argument);
}
