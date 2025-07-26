#include <neo/cryptography/hash.h>
#include <cstring>
#include <array>

namespace neo::cryptography
{
    // Keccak-256 implementation (not SHA3-256!)
    // Based on the original Keccak specification used by Ethereum
    namespace keccak_internal
    {
        constexpr size_t KECCAK_ROUNDS = 24;
        constexpr size_t KECCAK_STATE_SIZE = 25;
        constexpr size_t KECCAK_RATE = 136; // 1088 bits / 8 = 136 bytes for Keccak-256
        
        // Keccak round constants
        constexpr std::array<uint64_t, KECCAK_ROUNDS> RC = {
            0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL,
            0x8000000080008000ULL, 0x000000000000808bULL, 0x0000000080000001ULL,
            0x8000000080008081ULL, 0x8000000000008009ULL, 0x000000000000008aULL,
            0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000aULL,
            0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL,
            0x8000000000008003ULL, 0x8000000000008002ULL, 0x8000000000000080ULL,
            0x000000000000800aULL, 0x800000008000000aULL, 0x8000000080008081ULL,
            0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL
        };
        
        // Rotation offsets for rho step
        constexpr std::array<int, 25> rho_offsets = {
             1,  3,  6, 10, 15, 21, 28, 36, 45, 55,  2, 14,
            27, 41, 56,  8, 25, 43, 62, 18, 39, 61, 20, 44
        };
        
        // Pi step permutation
        constexpr std::array<int, 25> pi_indices = {
            10,  7, 11, 17, 18,  3,  5, 16,  8, 21, 24,  4,
            15, 23, 19, 13, 12,  2, 20, 14, 22,  9,  6,  1
        };
        
        inline uint64_t rotl64(uint64_t n, int c) {
            return (n << c) | (n >> (64 - c));
        }
        
        void keccakf(uint64_t state[25]) {
            uint64_t C[5], D[5], B[25];
            
            for (int round = 0; round < KECCAK_ROUNDS; round++) {
                // Theta step
                C[0] = state[0] ^ state[5] ^ state[10] ^ state[15] ^ state[20];
                C[1] = state[1] ^ state[6] ^ state[11] ^ state[16] ^ state[21];
                C[2] = state[2] ^ state[7] ^ state[12] ^ state[17] ^ state[22];
                C[3] = state[3] ^ state[8] ^ state[13] ^ state[18] ^ state[23];
                C[4] = state[4] ^ state[9] ^ state[14] ^ state[19] ^ state[24];
                
                D[0] = C[4] ^ rotl64(C[1], 1);
                D[1] = C[0] ^ rotl64(C[2], 1);
                D[2] = C[1] ^ rotl64(C[3], 1);
                D[3] = C[2] ^ rotl64(C[4], 1);
                D[4] = C[3] ^ rotl64(C[0], 1);
                
                for (int i = 0; i < 25; i++) {
                    state[i] ^= D[i % 5];
                }
                
                // Rho and Pi steps
                B[0] = state[0];
                for (int i = 1; i < 25; i++) {
                    B[i] = rotl64(state[i], rho_offsets[i - 1]);
                }
                
                for (int i = 0; i < 25; i++) {
                    state[i] = B[pi_indices[i]];
                }
                
                // Chi step  
                for (int i = 0; i < 25; i += 5) {
                    C[0] = state[i];
                    C[1] = state[i + 1];
                    C[2] = state[i + 2];
                    C[3] = state[i + 3];
                    C[4] = state[i + 4];
                    
                    state[i] = C[0] ^ ((~C[1]) & C[2]);
                    state[i + 1] = C[1] ^ ((~C[2]) & C[3]);
                    state[i + 2] = C[2] ^ ((~C[3]) & C[4]);
                    state[i + 3] = C[3] ^ ((~C[4]) & C[0]);
                    state[i + 4] = C[4] ^ ((~C[0]) & C[1]);
                }
                
                // Iota step
                state[0] ^= RC[round];
            }
        }
        
        void keccak256(const uint8_t* input, size_t input_len, uint8_t* output) {
            uint64_t state[25] = {0};
            uint8_t* state_bytes = reinterpret_cast<uint8_t*>(state);
            
            // Absorbing phase
            size_t offset = 0;
            while (offset < input_len) {
                size_t block_size = std::min(KECCAK_RATE, input_len - offset);
                
                for (size_t i = 0; i < block_size; i++) {
                    state_bytes[i] ^= input[offset + i];
                }
                
                offset += block_size;
                
                if (block_size == KECCAK_RATE) {
                    keccakf(state);
                }
            }
            
            // Padding (Keccak padding, not SHA3 padding!)
            state_bytes[input_len % KECCAK_RATE] ^= 0x01;  // Keccak padding
            state_bytes[KECCAK_RATE - 1] ^= 0x80;
            
            keccakf(state);
            
            // Squeezing phase (output 256 bits = 32 bytes)
            std::memcpy(output, state_bytes, 32);
        }
    }
    
    // Replace the incorrect implementation with proper Keccak-256
    io::UInt256 Hash::Keccak256Proper(const io::ByteSpan& data)
    {
        uint8_t hash[32];
        keccak_internal::keccak256(data.Data(), data.Size(), hash);
        return io::UInt256(io::ByteSpan(hash, 32));
    }
}