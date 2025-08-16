/**
 * @file benchmark_crypto.cpp
 * @brief Performance benchmarks for cryptographic operations
 * Measures performance of hashing, signing, verification, and encryption
 */

#include <benchmark/benchmark.h>
#include <neo/cryptography/crypto.h>
#include <neo/cryptography/key_pair.h>
#include <neo/cryptography/ecdsa.h>
#include <neo/cryptography/aes.h>
#include <neo/cryptography/base58.h>
#include <neo/io/byte_vector.h>
#include <random>
#include <vector>

using namespace neo::cryptography;
using namespace neo::io;

// ============================================================================
// Hash Algorithm Benchmarks
// ============================================================================

static void BM_SHA256_Small(benchmark::State& state) {
    ByteVector data(32);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (auto& byte : data) {
        byte = static_cast<uint8_t>(dis(gen));
    }
    
    for (auto _ : state) {
        auto hash = Crypto::Hash256(data.AsSpan());
        benchmark::DoNotOptimize(hash);
    }
    
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(data.size()));
}
BENCHMARK(BM_SHA256_Small);

static void BM_SHA256_Large(benchmark::State& state) {
    ByteVector data(1024 * 1024); // 1MB
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (auto& byte : data) {
        byte = static_cast<uint8_t>(dis(gen));
    }
    
    for (auto _ : state) {
        auto hash = Crypto::Hash256(data.AsSpan());
        benchmark::DoNotOptimize(hash);
    }
    
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(data.size()));
}
BENCHMARK(BM_SHA256_Large);

static void BM_RIPEMD160(benchmark::State& state) {
    ByteVector data(64);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (auto& byte : data) {
        byte = static_cast<uint8_t>(dis(gen));
    }
    
    for (auto _ : state) {
        auto hash = Crypto::Hash160(data.AsSpan());
        benchmark::DoNotOptimize(hash);
    }
    
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(data.size()));
}
BENCHMARK(BM_RIPEMD160);

static void BM_DoubleSHA256(benchmark::State& state) {
    ByteVector data(80); // Bitcoin block header size
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (auto& byte : data) {
        byte = static_cast<uint8_t>(dis(gen));
    }
    
    for (auto _ : state) {
        auto hash1 = Crypto::Hash256(data.AsSpan());
        auto hash2 = Crypto::Hash256(hash1.AsSpan());
        benchmark::DoNotOptimize(hash2);
    }
    
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(data.size()));
}
BENCHMARK(BM_DoubleSHA256);

// ============================================================================
// ECDSA Operations Benchmarks
// ============================================================================

static void BM_ECDSA_KeyGeneration(benchmark::State& state) {
    for (auto _ : state) {
        auto keyPair = KeyPair::Generate();
        benchmark::DoNotOptimize(keyPair);
    }
    
    state.SetItemsProcessed(int64_t(state.iterations()));
}
BENCHMARK(BM_ECDSA_KeyGeneration);

static void BM_ECDSA_Sign(benchmark::State& state) {
    auto keyPair = KeyPair::Generate();
    ByteVector message(32);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (auto& byte : message) {
        byte = static_cast<uint8_t>(dis(gen));
    }
    
    for (auto _ : state) {
        auto signature = keyPair.Sign(message);
        benchmark::DoNotOptimize(signature);
    }
    
    state.SetItemsProcessed(int64_t(state.iterations()));
}
BENCHMARK(BM_ECDSA_Sign);

static void BM_ECDSA_Verify(benchmark::State& state) {
    auto keyPair = KeyPair::Generate();
    ByteVector message(32);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (auto& byte : message) {
        byte = static_cast<uint8_t>(dis(gen));
    }
    
    auto signature = keyPair.Sign(message);
    auto publicKey = keyPair.GetPublicKey();
    
    for (auto _ : state) {
        bool valid = Crypto::VerifySignature(message, signature, publicKey);
        benchmark::DoNotOptimize(valid);
    }
    
    state.SetItemsProcessed(int64_t(state.iterations()));
}
BENCHMARK(BM_ECDSA_Verify);

static void BM_ECDSA_BatchVerify(benchmark::State& state) {
    const int batch_size = state.range(0);
    std::vector<KeyPair> keyPairs;
    std::vector<ByteVector> messages;
    std::vector<ByteVector> signatures;
    std::vector<ECPoint> publicKeys;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (int i = 0; i < batch_size; ++i) {
        keyPairs.push_back(KeyPair::Generate());
        
        ByteVector message(32);
        for (auto& byte : message) {
            byte = static_cast<uint8_t>(dis(gen));
        }
        messages.push_back(message);
        
        signatures.push_back(keyPairs[i].Sign(messages[i]));
        publicKeys.push_back(keyPairs[i].GetPublicKey());
    }
    
    for (auto _ : state) {
        for (int i = 0; i < batch_size; ++i) {
            bool valid = Crypto::VerifySignature(messages[i], signatures[i], publicKeys[i]);
            benchmark::DoNotOptimize(valid);
        }
    }
    
    state.SetItemsProcessed(int64_t(state.iterations()) * batch_size);
}
BENCHMARK(BM_ECDSA_BatchVerify)->Arg(10)->Arg(100)->Arg(1000);

// ============================================================================
// AES Encryption Benchmarks
// ============================================================================

static void BM_AES_Encrypt(benchmark::State& state) {
    const size_t data_size = state.range(0);
    ByteVector key(32); // 256-bit key
    ByteVector iv(16);  // 128-bit IV
    ByteVector plaintext(data_size);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (auto& byte : key) byte = static_cast<uint8_t>(dis(gen));
    for (auto& byte : iv) byte = static_cast<uint8_t>(dis(gen));
    for (auto& byte : plaintext) byte = static_cast<uint8_t>(dis(gen));
    
    for (auto _ : state) {
        auto ciphertext = AES::Encrypt(plaintext, key, iv);
        benchmark::DoNotOptimize(ciphertext);
    }
    
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(data_size));
}
BENCHMARK(BM_AES_Encrypt)->Arg(16)->Arg(1024)->Arg(1024*1024);

static void BM_AES_Decrypt(benchmark::State& state) {
    const size_t data_size = state.range(0);
    ByteVector key(32);
    ByteVector iv(16);
    ByteVector plaintext(data_size);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (auto& byte : key) byte = static_cast<uint8_t>(dis(gen));
    for (auto& byte : iv) byte = static_cast<uint8_t>(dis(gen));
    for (auto& byte : plaintext) byte = static_cast<uint8_t>(dis(gen));
    
    auto ciphertext = AES::Encrypt(plaintext, key, iv);
    
    for (auto _ : state) {
        auto decrypted = AES::Decrypt(ciphertext, key, iv);
        benchmark::DoNotOptimize(decrypted);
    }
    
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(data_size));
}
BENCHMARK(BM_AES_Decrypt)->Arg(16)->Arg(1024)->Arg(1024*1024);

// ============================================================================
// Base58 Encoding/Decoding Benchmarks
// ============================================================================

static void BM_Base58_Encode(benchmark::State& state) {
    const size_t data_size = state.range(0);
    ByteVector data(data_size);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (auto& byte : data) {
        byte = static_cast<uint8_t>(dis(gen));
    }
    
    for (auto _ : state) {
        auto encoded = Base58::Encode(data);
        benchmark::DoNotOptimize(encoded);
    }
    
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(data_size));
}
BENCHMARK(BM_Base58_Encode)->Arg(20)->Arg(32)->Arg(64)->Arg(128);

static void BM_Base58_Decode(benchmark::State& state) {
    const size_t data_size = state.range(0);
    ByteVector data(data_size);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (auto& byte : data) {
        byte = static_cast<uint8_t>(dis(gen));
    }
    
    auto encoded = Base58::Encode(data);
    
    for (auto _ : state) {
        auto decoded = Base58::Decode(encoded);
        benchmark::DoNotOptimize(decoded);
    }
    
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(encoded.length()));
}
BENCHMARK(BM_Base58_Decode)->Arg(20)->Arg(32)->Arg(64)->Arg(128);

// ============================================================================
// Multi-Signature Benchmarks
// ============================================================================

static void BM_MultiSig_Verify(benchmark::State& state) {
    const int m = state.range(0); // Required signatures
    const int n = state.range(1); // Total keys
    
    std::vector<KeyPair> keyPairs;
    std::vector<ECPoint> publicKeys;
    std::vector<ByteVector> signatures;
    
    for (int i = 0; i < n; ++i) {
        keyPairs.push_back(KeyPair::Generate());
        publicKeys.push_back(keyPairs[i].GetPublicKey());
    }
    
    ByteVector message(32);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (auto& byte : message) {
        byte = static_cast<uint8_t>(dis(gen));
    }
    
    // Sign with first m keys
    for (int i = 0; i < m; ++i) {
        signatures.push_back(keyPairs[i].Sign(message));
    }
    
    for (auto _ : state) {
        int valid_count = 0;
        for (int i = 0; i < m; ++i) {
            for (int j = 0; j < n; ++j) {
                if (Crypto::VerifySignature(message, signatures[i], publicKeys[j])) {
                    valid_count++;
                    break;
                }
            }
        }
        benchmark::DoNotOptimize(valid_count == m);
    }
    
    state.SetItemsProcessed(int64_t(state.iterations()));
}
BENCHMARK(BM_MultiSig_Verify)->Args({2, 3})->Args({3, 5})->Args({7, 10});

// ============================================================================
// Merkle Tree Benchmarks
// ============================================================================

static void BM_MerkleTree_Build(benchmark::State& state) {
    const int num_leaves = state.range(0);
    std::vector<ByteVector> leaves;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (int i = 0; i < num_leaves; ++i) {
        ByteVector leaf(32);
        for (auto& byte : leaf) {
            byte = static_cast<uint8_t>(dis(gen));
        }
        leaves.push_back(leaf);
    }
    
    for (auto _ : state) {
        std::vector<ByteVector> tree = leaves;
        while (tree.size() > 1) {
            std::vector<ByteVector> next_level;
            for (size_t i = 0; i < tree.size(); i += 2) {
                if (i + 1 < tree.size()) {
                    ByteVector combined = tree[i];
                    combined.insert(combined.end(), tree[i+1].begin(), tree[i+1].end());
                    next_level.push_back(Crypto::Hash256(combined.AsSpan()));
                } else {
                    next_level.push_back(tree[i]);
                }
            }
            tree = next_level;
        }
        benchmark::DoNotOptimize(tree[0]);
    }
    
    state.SetItemsProcessed(int64_t(state.iterations()) * num_leaves);
}
BENCHMARK(BM_MerkleTree_Build)->Arg(10)->Arg(100)->Arg(1000)->Arg(10000);

BENCHMARK_MAIN();