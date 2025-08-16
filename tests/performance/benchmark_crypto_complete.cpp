/**
 * @file benchmark_crypto_complete.cpp
 * @brief Comprehensive cryptography performance benchmarks
 */

#include <benchmark/benchmark.h>
#include <neo/cryptography/sha256.h>
#include <neo/cryptography/ripemd160.h>
#include <neo/cryptography/hash.h>
#include <neo/cryptography/key_pair.h>
#include <neo/cryptography/ecdsa.h>
#include <neo/cryptography/aes.h>
#include <neo/cryptography/base58.h>
#include <neo/cryptography/base64.h>
#include <neo/cryptography/bloom_filter.h>
#include <neo/cryptography/merkle_tree.h>
#include <neo/io/byte_vector.h>
#include <random>
#include <vector>
#include <string>

using namespace neo::cryptography;
using namespace neo::io;

// ============================================================================
// Test Data Generation
// ============================================================================

static std::vector<uint8_t> GenerateRandomData(size_t size) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    std::vector<uint8_t> data(size);
    for (size_t i = 0; i < size; ++i) {
        data[i] = static_cast<uint8_t>(dis(gen));
    }
    return data;
}

// ============================================================================
// SHA256 Benchmarks
// ============================================================================

static void BM_SHA256_Small(benchmark::State& state) {
    auto data = GenerateRandomData(32);
    ByteVector input(data);
    
    for (auto _ : state) {
        auto hash = SHA256::ComputeHash(input);
        benchmark::DoNotOptimize(hash);
    }
    
    state.SetBytesProcessed(state.iterations() * 32);
}
BENCHMARK(BM_SHA256_Small);

static void BM_SHA256_Medium(benchmark::State& state) {
    auto data = GenerateRandomData(1024);
    ByteVector input(data);
    
    for (auto _ : state) {
        auto hash = SHA256::ComputeHash(input);
        benchmark::DoNotOptimize(hash);
    }
    
    state.SetBytesProcessed(state.iterations() * 1024);
}
BENCHMARK(BM_SHA256_Medium);

static void BM_SHA256_Large(benchmark::State& state) {
    auto data = GenerateRandomData(1024 * 1024); // 1MB
    ByteVector input(data);
    
    for (auto _ : state) {
        auto hash = SHA256::ComputeHash(input);
        benchmark::DoNotOptimize(hash);
    }
    
    state.SetBytesProcessed(state.iterations() * 1024 * 1024);
}
BENCHMARK(BM_SHA256_Large);

static void BM_SHA256_Double(benchmark::State& state) {
    auto data = GenerateRandomData(32);
    ByteVector input(data);
    
    for (auto _ : state) {
        auto hash = SHA256::ComputeHash(SHA256::ComputeHash(input));
        benchmark::DoNotOptimize(hash);
    }
    
    state.SetBytesProcessed(state.iterations() * 32);
}
BENCHMARK(BM_SHA256_Double);

// ============================================================================
// RIPEMD160 Benchmarks
// ============================================================================

static void BM_RIPEMD160_Small(benchmark::State& state) {
    auto data = GenerateRandomData(32);
    ByteVector input(data);
    
    for (auto _ : state) {
        auto hash = RIPEMD160::ComputeHash(input);
        benchmark::DoNotOptimize(hash);
    }
    
    state.SetBytesProcessed(state.iterations() * 32);
}
BENCHMARK(BM_RIPEMD160_Small);

static void BM_RIPEMD160_Large(benchmark::State& state) {
    auto data = GenerateRandomData(1024 * 1024); // 1MB
    ByteVector input(data);
    
    for (auto _ : state) {
        auto hash = RIPEMD160::ComputeHash(input);
        benchmark::DoNotOptimize(hash);
    }
    
    state.SetBytesProcessed(state.iterations() * 1024 * 1024);
}
BENCHMARK(BM_RIPEMD160_Large);

// ============================================================================
// ECDSA Key Generation and Signing Benchmarks
// ============================================================================

static void BM_ECDSA_KeyGeneration(benchmark::State& state) {
    for (auto _ : state) {
        KeyPair keypair;
        benchmark::DoNotOptimize(keypair.GetPublicKey());
        benchmark::DoNotOptimize(keypair.GetPrivateKey());
    }
}
BENCHMARK(BM_ECDSA_KeyGeneration);

static void BM_ECDSA_Sign(benchmark::State& state) {
    KeyPair keypair;
    auto data = GenerateRandomData(32);
    ByteVector message(data);
    
    for (auto _ : state) {
        auto signature = keypair.Sign(message);
        benchmark::DoNotOptimize(signature);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ECDSA_Sign);

static void BM_ECDSA_Verify(benchmark::State& state) {
    KeyPair keypair;
    auto data = GenerateRandomData(32);
    ByteVector message(data);
    auto signature = keypair.Sign(message);
    
    for (auto _ : state) {
        bool valid = keypair.Verify(message, signature);
        benchmark::DoNotOptimize(valid);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ECDSA_Verify);

static void BM_ECDSA_SignAndVerify(benchmark::State& state) {
    KeyPair keypair;
    auto data = GenerateRandomData(32);
    ByteVector message(data);
    
    for (auto _ : state) {
        auto signature = keypair.Sign(message);
        bool valid = keypair.Verify(message, signature);
        benchmark::DoNotOptimize(signature);
        benchmark::DoNotOptimize(valid);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ECDSA_SignAndVerify);

// ============================================================================
// AES Encryption/Decryption Benchmarks
// ============================================================================

static void BM_AES_Encrypt_128(benchmark::State& state) {
    auto key = GenerateRandomData(16); // 128-bit key
    auto iv = GenerateRandomData(16);
    auto plaintext = GenerateRandomData(1024);
    
    AES aes(ByteVector(key), ByteVector(iv));
    ByteVector input(plaintext);
    
    for (auto _ : state) {
        auto ciphertext = aes.Encrypt(input);
        benchmark::DoNotOptimize(ciphertext);
    }
    
    state.SetBytesProcessed(state.iterations() * 1024);
}
BENCHMARK(BM_AES_Encrypt_128);

static void BM_AES_Decrypt_128(benchmark::State& state) {
    auto key = GenerateRandomData(16); // 128-bit key
    auto iv = GenerateRandomData(16);
    auto plaintext = GenerateRandomData(1024);
    
    AES aes(ByteVector(key), ByteVector(iv));
    ByteVector input(plaintext);
    auto ciphertext = aes.Encrypt(input);
    
    for (auto _ : state) {
        auto decrypted = aes.Decrypt(ciphertext);
        benchmark::DoNotOptimize(decrypted);
    }
    
    state.SetBytesProcessed(state.iterations() * 1024);
}
BENCHMARK(BM_AES_Decrypt_128);

static void BM_AES_Encrypt_256(benchmark::State& state) {
    auto key = GenerateRandomData(32); // 256-bit key
    auto iv = GenerateRandomData(16);
    auto plaintext = GenerateRandomData(1024);
    
    AES aes(ByteVector(key), ByteVector(iv));
    ByteVector input(plaintext);
    
    for (auto _ : state) {
        auto ciphertext = aes.Encrypt(input);
        benchmark::DoNotOptimize(ciphertext);
    }
    
    state.SetBytesProcessed(state.iterations() * 1024);
}
BENCHMARK(BM_AES_Encrypt_256);

// ============================================================================
// Base58/Base64 Encoding Benchmarks
// ============================================================================

static void BM_Base58_Encode(benchmark::State& state) {
    auto data = GenerateRandomData(32);
    ByteVector input(data);
    
    for (auto _ : state) {
        auto encoded = Base58::Encode(input);
        benchmark::DoNotOptimize(encoded);
    }
    
    state.SetBytesProcessed(state.iterations() * 32);
}
BENCHMARK(BM_Base58_Encode);

static void BM_Base58_Decode(benchmark::State& state) {
    auto data = GenerateRandomData(32);
    ByteVector input(data);
    auto encoded = Base58::Encode(input);
    
    for (auto _ : state) {
        auto decoded = Base58::Decode(encoded);
        benchmark::DoNotOptimize(decoded);
    }
    
    state.SetBytesProcessed(state.iterations() * encoded.length());
}
BENCHMARK(BM_Base58_Decode);

static void BM_Base64_Encode(benchmark::State& state) {
    auto data = GenerateRandomData(1024);
    ByteVector input(data);
    
    for (auto _ : state) {
        auto encoded = Base64::Encode(input);
        benchmark::DoNotOptimize(encoded);
    }
    
    state.SetBytesProcessed(state.iterations() * 1024);
}
BENCHMARK(BM_Base64_Encode);

static void BM_Base64_Decode(benchmark::State& state) {
    auto data = GenerateRandomData(1024);
    ByteVector input(data);
    auto encoded = Base64::Encode(input);
    
    for (auto _ : state) {
        auto decoded = Base64::Decode(encoded);
        benchmark::DoNotOptimize(decoded);
    }
    
    state.SetBytesProcessed(state.iterations() * encoded.length());
}
BENCHMARK(BM_Base64_Decode);

// ============================================================================
// Bloom Filter Benchmarks
// ============================================================================

static void BM_BloomFilter_Add(benchmark::State& state) {
    BloomFilter filter(1024 * 8, 3); // 1KB filter, 3 hash functions
    auto data = GenerateRandomData(32);
    ByteVector item(data);
    
    for (auto _ : state) {
        filter.Add(item);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_BloomFilter_Add);

static void BM_BloomFilter_Contains(benchmark::State& state) {
    BloomFilter filter(1024 * 8, 3); // 1KB filter, 3 hash functions
    
    // Add some items
    for (int i = 0; i < 100; ++i) {
        auto data = GenerateRandomData(32);
        filter.Add(ByteVector(data));
    }
    
    auto test_data = GenerateRandomData(32);
    ByteVector test_item(test_data);
    
    for (auto _ : state) {
        bool contains = filter.Contains(test_item);
        benchmark::DoNotOptimize(contains);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_BloomFilter_Contains);

// ============================================================================
// Merkle Tree Benchmarks
// ============================================================================

static void BM_MerkleTree_Build_Small(benchmark::State& state) {
    std::vector<ByteVector> hashes;
    for (int i = 0; i < 10; ++i) {
        auto data = GenerateRandomData(32);
        hashes.push_back(ByteVector(data));
    }
    
    for (auto _ : state) {
        MerkleTree tree(hashes);
        auto root = tree.GetRoot();
        benchmark::DoNotOptimize(root);
    }
    
    state.SetItemsProcessed(state.iterations() * 10);
}
BENCHMARK(BM_MerkleTree_Build_Small);

static void BM_MerkleTree_Build_Large(benchmark::State& state) {
    std::vector<ByteVector> hashes;
    for (int i = 0; i < 1000; ++i) {
        auto data = GenerateRandomData(32);
        hashes.push_back(ByteVector(data));
    }
    
    for (auto _ : state) {
        MerkleTree tree(hashes);
        auto root = tree.GetRoot();
        benchmark::DoNotOptimize(root);
    }
    
    state.SetItemsProcessed(state.iterations() * 1000);
}
BENCHMARK(BM_MerkleTree_Build_Large);

static void BM_MerkleTree_GetProof(benchmark::State& state) {
    std::vector<ByteVector> hashes;
    for (int i = 0; i < 100; ++i) {
        auto data = GenerateRandomData(32);
        hashes.push_back(ByteVector(data));
    }
    
    MerkleTree tree(hashes);
    
    for (auto _ : state) {
        auto proof = tree.GetProof(50); // Get proof for middle element
        benchmark::DoNotOptimize(proof);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MerkleTree_GetProof);

static void BM_MerkleTree_VerifyProof(benchmark::State& state) {
    std::vector<ByteVector> hashes;
    for (int i = 0; i < 100; ++i) {
        auto data = GenerateRandomData(32);
        hashes.push_back(ByteVector(data));
    }
    
    MerkleTree tree(hashes);
    auto proof = tree.GetProof(50);
    auto root = tree.GetRoot();
    
    for (auto _ : state) {
        bool valid = MerkleTree::VerifyProof(root, hashes[50], proof);
        benchmark::DoNotOptimize(valid);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MerkleTree_VerifyProof);

// ============================================================================
// Combined Cryptographic Operations
// ============================================================================

static void BM_Hash160(benchmark::State& state) {
    auto data = GenerateRandomData(32);
    ByteVector input(data);
    
    for (auto _ : state) {
        // Hash160 = RIPEMD160(SHA256(data))
        auto sha = SHA256::ComputeHash(input);
        auto hash160 = RIPEMD160::ComputeHash(sha);
        benchmark::DoNotOptimize(hash160);
    }
    
    state.SetBytesProcessed(state.iterations() * 32);
}
BENCHMARK(BM_Hash160);

static void BM_Hash256(benchmark::State& state) {
    auto data = GenerateRandomData(32);
    ByteVector input(data);
    
    for (auto _ : state) {
        // Hash256 = SHA256(SHA256(data))
        auto hash = SHA256::ComputeHash(SHA256::ComputeHash(input));
        benchmark::DoNotOptimize(hash);
    }
    
    state.SetBytesProcessed(state.iterations() * 32);
}
BENCHMARK(BM_Hash256);

static void BM_ScriptHash(benchmark::State& state) {
    auto data = GenerateRandomData(100); // Typical script size
    ByteVector script(data);
    
    for (auto _ : state) {
        // ScriptHash = Hash160 of script
        auto sha = SHA256::ComputeHash(script);
        auto hash = RIPEMD160::ComputeHash(sha);
        benchmark::DoNotOptimize(hash);
    }
    
    state.SetBytesProcessed(state.iterations() * 100);
}
BENCHMARK(BM_ScriptHash);

// ============================================================================
// Batch Operations
// ============================================================================

static void BM_BatchSign_10(benchmark::State& state) {
    KeyPair keypair;
    std::vector<ByteVector> messages;
    
    for (int i = 0; i < 10; ++i) {
        auto data = GenerateRandomData(32);
        messages.push_back(ByteVector(data));
    }
    
    for (auto _ : state) {
        std::vector<ByteVector> signatures;
        for (const auto& msg : messages) {
            signatures.push_back(keypair.Sign(msg));
        }
        benchmark::DoNotOptimize(signatures);
    }
    
    state.SetItemsProcessed(state.iterations() * 10);
}
BENCHMARK(BM_BatchSign_10);

static void BM_BatchVerify_10(benchmark::State& state) {
    KeyPair keypair;
    std::vector<ByteVector> messages;
    std::vector<ByteVector> signatures;
    
    for (int i = 0; i < 10; ++i) {
        auto data = GenerateRandomData(32);
        messages.push_back(ByteVector(data));
        signatures.push_back(keypair.Sign(messages.back()));
    }
    
    for (auto _ : state) {
        bool all_valid = true;
        for (size_t i = 0; i < messages.size(); ++i) {
            all_valid &= keypair.Verify(messages[i], signatures[i]);
        }
        benchmark::DoNotOptimize(all_valid);
    }
    
    state.SetItemsProcessed(state.iterations() * 10);
}
BENCHMARK(BM_BatchVerify_10);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();