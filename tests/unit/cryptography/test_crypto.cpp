#include <gtest/gtest.h>
#include <neo/cryptography/crypto.h>
#include <neo/io/byte_vector.h>

using namespace neo::cryptography;
using namespace neo::io;

TEST(CryptoTest, GenerateRandomBytes)
{
    // Generate random bytes of different lengths
    ByteVector random1 = Crypto::GenerateRandomBytes(16);
    EXPECT_EQ(random1.Size(), 16);
    
    ByteVector random2 = Crypto::GenerateRandomBytes(32);
    EXPECT_EQ(random2.Size(), 32);
    
    // Ensure different random bytes are generated
    ByteVector random3 = Crypto::GenerateRandomBytes(16);
    EXPECT_NE(random1, random3);
}

TEST(CryptoTest, AesEncryptDecrypt)
{
    // Generate key and IV
    ByteVector key = Crypto::GenerateRandomBytes(32);
    ByteVector iv = Crypto::GenerateRandomBytes(16);
    
    // Data to encrypt
    ByteVector data = ByteVector::Parse("000102030405060708090a0b0c0d0e0f");
    
    // Encrypt
    ByteVector encrypted = Crypto::AesEncrypt(data.AsSpan(), key.AsSpan(), iv.AsSpan());
    EXPECT_NE(encrypted, data);
    
    // Decrypt
    ByteVector decrypted = Crypto::AesDecrypt(encrypted.AsSpan(), key.AsSpan(), iv.AsSpan());
    EXPECT_EQ(decrypted, data);
    
    // Invalid key size
    ByteVector invalidKey = Crypto::GenerateRandomBytes(16);
    EXPECT_THROW(Crypto::AesEncrypt(data.AsSpan(), invalidKey.AsSpan(), iv.AsSpan()), std::invalid_argument);
    
    // Invalid IV size
    ByteVector invalidIv = Crypto::GenerateRandomBytes(8);
    EXPECT_THROW(Crypto::AesEncrypt(data.AsSpan(), key.AsSpan(), invalidIv.AsSpan()), std::invalid_argument);
}

TEST(CryptoTest, PBKDF2)
{
    // Test vector from https://stackoverflow.com/questions/15593184/pbkdf2-hmac-sha-256-test-vectors
    ByteVector password = ByteVector(ByteSpan(reinterpret_cast<const uint8_t*>("password"), 8));
    ByteVector salt = ByteVector(ByteSpan(reinterpret_cast<const uint8_t*>("salt"), 4));
    
    ByteVector key = Crypto::PBKDF2(password.AsSpan(), salt.AsSpan(), 1, 32);
    EXPECT_EQ(key.ToHexString(), "120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b");
    
    // Different iterations
    ByteVector key2 = Crypto::PBKDF2(password.AsSpan(), salt.AsSpan(), 2, 32);
    EXPECT_NE(key, key2);
    EXPECT_EQ(key2.ToHexString(), "ae4d0c95af6b46d32d0adff928f06dd02a303f8ef3c251dfd6e2d85a95474c43");
    
    // Different key length
    ByteVector key3 = Crypto::PBKDF2(password.AsSpan(), salt.AsSpan(), 1, 16);
    EXPECT_EQ(key3.Size(), 16);
    EXPECT_EQ(key3.ToHexString(), "120fb6cffcf8b32c43e7225256c4f837");
}

TEST(CryptoTest, HmacSha256)
{
    // Test vector from https://tools.ietf.org/html/rfc4231
    ByteVector key = ByteVector::Parse("0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b");
    ByteVector data = ByteVector(ByteSpan(reinterpret_cast<const uint8_t*>("Hi There"), 8));
    
    ByteVector hmac = Crypto::HmacSha256(key.AsSpan(), data.AsSpan());
    EXPECT_EQ(hmac.ToHexString(), "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");
    
    // Empty key
    ByteVector emptyKey;
    ByteVector hmac2 = Crypto::HmacSha256(emptyKey.AsSpan(), data.AsSpan());
    EXPECT_NE(hmac, hmac2);
    
    // Empty data
    ByteVector emptyData;
    ByteVector hmac3 = Crypto::HmacSha256(key.AsSpan(), emptyData.AsSpan());
    EXPECT_NE(hmac, hmac3);
}

TEST(CryptoTest, Base64EncodeDecode)
{
    // Test data
    ByteVector data = ByteVector::Parse("000102030405060708090a0b0c0d0e0f");
    
    // Encode
    std::string base64 = Crypto::Base64Encode(data.AsSpan());
    EXPECT_EQ(base64, "AAECAwQFBgcICQoLDA0ODw==");
    
    // Decode
    ByteVector decoded = Crypto::Base64Decode(base64);
    EXPECT_EQ(decoded, data);
    
    // Empty data
    ByteVector emptyData;
    std::string emptyBase64 = Crypto::Base64Encode(emptyData.AsSpan());
    EXPECT_EQ(emptyBase64, "");
    
    ByteVector emptyDecoded = Crypto::Base64Decode(emptyBase64);
    EXPECT_EQ(emptyDecoded.Size(), 0);
    
    // Invalid Base64
    EXPECT_THROW(Crypto::Base64Decode("Invalid!"), std::runtime_error);
}
