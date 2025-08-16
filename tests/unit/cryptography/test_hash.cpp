#include <gtest/gtest.h>
#include <neo/cryptography/hash.h>
#include <neo/io/byte_vector.h>

using namespace neo::cryptography;
using namespace neo::io;

TEST(HashTest, Sha256)
{
    // Test vector from https://www.di-mgt.com.au/sha_testvectors.html
    ByteVector input = ByteVector::Parse("616263");  // "abc"
    UInt256 hash = Hash::Sha256(input.AsSpan());
    EXPECT_EQ(hash.ToHexString(), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

    // Empty input
    ByteVector empty;
    UInt256 emptyHash = Hash::Sha256(empty.AsSpan());
    EXPECT_EQ(emptyHash.ToHexString(), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(HashTest, Ripemd160)
{
    // Test vector from https://homes.esat.kuleuven.be/~bosselae/ripemd160.html
    ByteVector input = ByteVector::Parse("616263");  // "abc"
    UInt160 hash = Hash::Ripemd160(input.AsSpan());
    EXPECT_EQ(hash.ToHexString(), "8eb208f7e05d987a9b044a8e98c6b087f15a0bfc");

    // Empty input
    ByteVector empty;
    UInt160 emptyHash = Hash::Ripemd160(empty.AsSpan());
    EXPECT_EQ(emptyHash.ToHexString(), "9c1185a5c5e9fc54612808977ee8f548b2258d31");
}

TEST(HashTest, Hash256)
{
    // Double SHA-256 of "abc"
    ByteVector input = ByteVector::Parse("616263");  // "abc"
    UInt256 hash = Hash::Hash256(input.AsSpan());

    // First SHA-256: BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD
    // Second SHA-256: 4F8B42C22DD3729B519BA6F68D2DA7CC5B2D606D05DAED5AD5128CC03E6C6358
    EXPECT_EQ(hash.ToHexString(), "4f8b42c22dd3729b519ba6f68d2da7cc5b2d606d05daed5ad5128cc03e6c6358");

    // Empty input
    ByteVector empty;
    UInt256 emptyHash = Hash::Hash256(empty.AsSpan());
    // First SHA-256: E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855
    // Second SHA-256: 5DF6E0E2761359D30A8275058E299FCC0381534545F55CF43E41983F5D4C9456
    EXPECT_EQ(emptyHash.ToHexString(), "5df6e0e2761359d30a8275058e299fcc0381534545f55cf43e41983f5d4c9456");
}

TEST(HashTest, Hash160)
{
    // RIPEMD-160 of SHA-256 of "abc"
    ByteVector input = ByteVector::Parse("616263");  // "abc"
    UInt160 hash = Hash::Hash160(input.AsSpan());

    // SHA-256: BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD
    // RIPEMD-160: BB1BE98C142444D7A56AA3981C3942A978E4DC33
    EXPECT_EQ(hash.ToHexString(), "bb1be98c142444d7a56aa3981c3942a978e4dc33");

    // Empty input
    ByteVector empty;
    UInt160 emptyHash = Hash::Hash160(empty.AsSpan());
    // SHA-256: E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855
    // RIPEMD-160: B472A266D0BD89C13706A4132CCFB16F7C3B9FCB
    EXPECT_EQ(emptyHash.ToHexString(), "b472a266d0bd89c13706a4132ccfb16f7c3b9fcb");
}

TEST(HashTest, Keccak256)
{
    // Test vectors for true Keccak-256 (not SHA3-256)
    // Test vector for Keccak-256 of "abc"
    ByteVector input = ByteVector::Parse("616263");  // "abc"
    UInt256 hash = Hash::Keccak256(input.AsSpan());
    EXPECT_EQ(hash.ToHexString(), "4e03657aea45a94fc7d47ba826c8d667c0d1e6e33a64a036ec44f58fa12d6c45");

    // Empty input for Keccak-256
    ByteVector empty;
    UInt256 emptyHash = Hash::Keccak256(empty.AsSpan());
    EXPECT_EQ(emptyHash.ToHexString(), "c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470");

    // Additional test vector: "The quick brown fox jumps over the lazy dog"
    ByteVector fox =
        ByteVector::Parse("54686520717569636B2062726F776E20666F78206A756D7073206F76657220746865206C617A7920646F67");
    UInt256 foxHash = Hash::Keccak256(fox.AsSpan());
    EXPECT_EQ(foxHash.ToHexString(), "4d741b6f1eb29cb2a9b9911c82f56fa8d73b04959d3d9d222895df6c0b28aa15");

    // Test vector for single byte 0x00
    ByteVector single = ByteVector::Parse("00");
    UInt256 singleHash = Hash::Keccak256(single.AsSpan());
    EXPECT_EQ(singleHash.ToHexString(), "bc36789e7a1e281436464229828f817d6612f7b477d66591ff96a9e064bcc98a");
}

TEST(HashTest, Murmur32)
{
    // Test vectors for MurmurHash3 32-bit
    ByteVector input = ByteVector::Parse("616263");  // "abc"
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
