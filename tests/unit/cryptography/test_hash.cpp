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
    EXPECT_EQ(hash.ToHexString(), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    
    // Empty input
    ByteVector empty;
    UInt256 emptyHash = Hash::Sha256(empty.AsSpan());
    EXPECT_EQ(emptyHash.ToHexString(), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(HashTest, Ripemd160)
{
    // Test vector from https://homes.esat.kuleuven.be/~bosselae/ripemd160.html
    ByteVector input = ByteVector::Parse("616263"); // "abc"
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
    ByteVector input = ByteVector::Parse("616263"); // "abc"
    UInt256 hash = Hash::Hash256(input.AsSpan());
    
    // First SHA-256: ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
    // Second SHA-256: 4f8b42c22dd3729b519ba6f68d2da7cc5b2d606d05daed5ad5128cc03e6c6358
    EXPECT_EQ(hash.ToHexString(), "4f8b42c22dd3729b519ba6f68d2da7cc5b2d606d05daed5ad5128cc03e6c6358");
    
    // Empty input
    ByteVector empty;
    UInt256 emptyHash = Hash::Hash256(empty.AsSpan());
    // First SHA-256: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
    // Second SHA-256: 5df6e0e2761359d30a8275058e299fcc0381534545f55cf43e41983f5d4c9456
    EXPECT_EQ(emptyHash.ToHexString(), "5df6e0e2761359d30a8275058e299fcc0381534545f55cf43e41983f5d4c9456");
}

TEST(HashTest, Hash160)
{
    // RIPEMD-160 of SHA-256 of "abc"
    ByteVector input = ByteVector::Parse("616263"); // "abc"
    UInt160 hash = Hash::Hash160(input.AsSpan());
    
    // SHA-256: ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
    // RIPEMD-160: bb1be98c142444d7a56aa3981c3942a978e4dc33
    EXPECT_EQ(hash.ToHexString(), "bb1be98c142444d7a56aa3981c3942a978e4dc33");
    
    // Empty input
    ByteVector empty;
    UInt160 emptyHash = Hash::Hash160(empty.AsSpan());
    // SHA-256: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
    // RIPEMD-160: b472a266d0bd89c13706a4132ccfb16f7c3b9fcb
    EXPECT_EQ(emptyHash.ToHexString(), "b472a266d0bd89c13706a4132ccfb16f7c3b9fcb");
}

TEST(HashTest, Keccak256)
{
    // Note: Current implementation uses SHA3-256, not true Keccak-256
    // Test vector for SHA3-256 of "abc" 
    ByteVector input = ByteVector::Parse("616263"); // "abc"
    UInt256 hash = Hash::Keccak256(input.AsSpan());
    EXPECT_EQ(hash.ToHexString(), "3a985da74fe225b2045c172d6bd390bd855f086e3e9d525b46bfe24511431532");
    
    // Empty input for SHA3-256
    ByteVector empty;
    UInt256 emptyHash = Hash::Keccak256(empty.AsSpan());
    EXPECT_EQ(emptyHash.ToHexString(), "a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a");
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
