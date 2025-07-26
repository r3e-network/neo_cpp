#include <gtest/gtest.h>
#include <neo/cryptography/hash.h>
#include <neo/io/byte_vector.h>

using namespace neo::cryptography;
using namespace neo::io;

TEST(HashTest, Sha256)
{
    // Test vector from https://www.di-mgt.com.au/sha_testvectors.html
    ByteVector input = ByteVector::Parse("616263"); // "abc"
    UInt256 hash = Hash::Sha256(input.AsSpan());
    EXPECT_EQ(hash.ToHexString(), "BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD");
    
    // Empty input
    ByteVector empty;
    UInt256 emptyHash = Hash::Sha256(empty.AsSpan());
    EXPECT_EQ(emptyHash.ToHexString(), "E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855");
}

TEST(HashTest, Ripemd160)
{
    // Test vector from https://homes.esat.kuleuven.be/~bosselae/ripemd160.html
    ByteVector input = ByteVector::Parse("616263"); // "abc"
    UInt160 hash = Hash::Ripemd160(input.AsSpan());
    EXPECT_EQ(hash.ToHexString(), "8EB208F7E05D987A9B044A8E98C6B087F15A0BFC");
    
    // Empty input
    ByteVector empty;
    UInt160 emptyHash = Hash::Ripemd160(empty.AsSpan());
    EXPECT_EQ(emptyHash.ToHexString(), "9C1185A5C5E9FC54612808977EE8F548B2258D31");
}

TEST(HashTest, Hash256)
{
    // Double SHA-256 of "abc"
    ByteVector input = ByteVector::Parse("616263"); // "abc"
    UInt256 hash = Hash::Hash256(input.AsSpan());
    
    // First SHA-256: BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD
    // Second SHA-256: 4F8B42C22DD3729B519BA6F68D2DA7CC5B2D606D05DAED5AD5128CC03E6C6358
    EXPECT_EQ(hash.ToHexString(), "4F8B42C22DD3729B519BA6F68D2DA7CC5B2D606D05DAED5AD5128CC03E6C6358");
    
    // Empty input
    ByteVector empty;
    UInt256 emptyHash = Hash::Hash256(empty.AsSpan());
    // First SHA-256: E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855
    // Second SHA-256: 5DF6E0E2761359D30A8275058E299FCC0381534545F55CF43E41983F5D4C9456
    EXPECT_EQ(emptyHash.ToHexString(), "5DF6E0E2761359D30A8275058E299FCC0381534545F55CF43E41983F5D4C9456");
}

TEST(HashTest, Hash160)
{
    // RIPEMD-160 of SHA-256 of "abc"
    ByteVector input = ByteVector::Parse("616263"); // "abc"
    UInt160 hash = Hash::Hash160(input.AsSpan());
    
    // SHA-256: BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD
    // RIPEMD-160: BB1BE98C142444D7A56AA3981C3942A978E4DC33
    EXPECT_EQ(hash.ToHexString(), "BB1BE98C142444D7A56AA3981C3942A978E4DC33");
    
    // Empty input
    ByteVector empty;
    UInt160 emptyHash = Hash::Hash160(empty.AsSpan());
    // SHA-256: E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855
    // RIPEMD-160: B472A266D0BD89C13706A4132CCFB16F7C3B9FCB
    EXPECT_EQ(emptyHash.ToHexString(), "B472A266D0BD89C13706A4132CCFB16F7C3B9FCB");
}

TEST(HashTest, Keccak256)
{
    // Test vectors for true Keccak-256 (not SHA3-256)
    // Test vector for Keccak-256 of "abc"
    ByteVector input = ByteVector::Parse("616263"); // "abc"
    UInt256 hash = Hash::Keccak256(input.AsSpan());
    EXPECT_EQ(hash.ToHexString(), "4E03657AEA45A94FC7D47BA826C8D667C0D1E6E33A64A036EC44F58FA12D6C45");
    
    // Empty input for Keccak-256
    ByteVector empty;
    UInt256 emptyHash = Hash::Keccak256(empty.AsSpan());
    EXPECT_EQ(emptyHash.ToHexString(), "C5D2460186F7233C927E7DB2DCC703C0E500B653CA82273B7BFAD8045D85A470");
    
    // Additional test vector: "The quick brown fox jumps over the lazy dog"
    ByteVector fox = ByteVector::Parse("54686520717569636B2062726F776E20666F78206A756D7073206F76657220746865206C617A7920646F67");
    UInt256 foxHash = Hash::Keccak256(fox.AsSpan());
    EXPECT_EQ(foxHash.ToHexString(), "4D741B6F1EB29CB2A9B9911C82F56FA8D73B04959D3D9D222895DF6C0B28AA15");
    
    // Test vector for single byte 0x00
    ByteVector single = ByteVector::Parse("00");
    UInt256 singleHash = Hash::Keccak256(single.AsSpan());
    EXPECT_EQ(singleHash.ToHexString(), "BC36789E7A1E281436464229828F817D6612F7B477D66591FF96A9E064BCC98A");
}

TEST(HashTest, Murmur32)
{
    // Test vectors for MurmurHash3 32-bit
    ByteVector input = ByteVector::Parse("616263"); // "abc"
    uint32_t hash = Hash::Murmur32(input.AsSpan(), 0);
    EXPECT_EQ(hash, 0xB3DD93FA);  // Correct MurmurHash3 32-bit value
    
    // Different seed
    uint32_t hash2 = Hash::Murmur32(input.AsSpan(), 42);
    EXPECT_NE(hash, hash2);
    
    // Empty input
    ByteVector empty;
    uint32_t emptyHash = Hash::Murmur32(empty.AsSpan(), 0);
    EXPECT_EQ(emptyHash, 0x00000000);
}
