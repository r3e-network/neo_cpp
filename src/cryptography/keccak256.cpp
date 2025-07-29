#include <array>
#include <cstring>
#include <neo/cryptography/hash.h>

namespace neo::cryptography
{
// Keccak-256 implementation (not SHA3-256!)
// Based on the original Keccak specification used by Ethereum
namespace keccak_internal
{
constexpr size_t KECCAK_ROUNDS = 24;
constexpr size_t KECCAK_STATE_SIZE = 25;
constexpr size_t KECCAK_RATE = 136;  // 1088 bits / 8 = 136 bytes for Keccak-256

// Keccak round constants
constexpr std::array<uint64_t, KECCAK_ROUNDS> RC = {
    0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL, 0x8000000080008000ULL, 0x000000000000808bULL,
    0x0000000080000001ULL, 0x8000000080008081ULL, 0x8000000000008009ULL, 0x000000000000008aULL, 0x0000000000000088ULL,
    0x0000000080008009ULL, 0x000000008000000aULL, 0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL,
    0x8000000000008003ULL, 0x8000000000008002ULL, 0x8000000000000080ULL, 0x000000000000800aULL, 0x800000008000000aULL,
    0x8000000080008081ULL, 0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL};

// Rotation offsets for rho step - corrected values
constexpr std::array<int, 25> rho_offsets = {0,  1,  62, 28, 27, 36, 44, 6,  55, 20, 3,  10, 43,
                                             25, 39, 41, 45, 15, 21, 8,  18, 2,  61, 56, 14};

inline uint64_t rotl64(uint64_t n, int c)
{
    return (n << c) | (n >> (64 - c));
}

void keccakf(uint64_t state[25])
{
    uint64_t C[5], D[5], B[25];

    for (int round = 0; round < KECCAK_ROUNDS; round++)
    {
        // Theta step
        for (int i = 0; i < 5; i++)
        {
            C[i] = state[i] ^ state[i + 5] ^ state[i + 10] ^ state[i + 15] ^ state[i + 20];
        }

        for (int i = 0; i < 5; i++)
        {
            D[i] = C[(i + 4) % 5] ^ rotl64(C[(i + 1) % 5], 1);
        }

        for (int i = 0; i < 25; i++)
        {
            state[i] ^= D[i % 5];
        }

        // Rho and Pi steps
        uint64_t current = state[1];
        int x = 1, y = 0;
        for (int t = 0; t < 24; t++)
        {
            int index = x + 5 * y;
            uint64_t rotatedValue = state[index];
            state[index] = rotl64(current, ((t + 1) * (t + 2) / 2) % 64);
            current = rotatedValue;
            int newX = y;
            int newY = (2 * x + 3 * y) % 5;
            x = newX;
            y = newY;
        }

        // Chi step
        for (int y = 0; y < 5; y++)
        {
            for (int x = 0; x < 5; x++)
            {
                B[x] = state[x + 5 * y];
            }
            for (int x = 0; x < 5; x++)
            {
                state[x + 5 * y] = B[x] ^ ((~B[(x + 1) % 5]) & B[(x + 2) % 5]);
            }
        }

        // Iota step
        state[0] ^= RC[round];
    }
}

void keccak256(const uint8_t* input, size_t input_len, uint8_t* output)
{
    uint64_t state[25] = {0};
    uint8_t* state_bytes = reinterpret_cast<uint8_t*>(state);

    // Absorbing phase
    size_t offset = 0;
    while (offset < input_len)
    {
        size_t block_size = std::min(KECCAK_RATE, input_len - offset);

        for (size_t i = 0; i < block_size; i++)
        {
            state_bytes[i] ^= input[offset + i];
        }

        offset += block_size;

        if (block_size == KECCAK_RATE)
        {
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
}  // namespace keccak_internal

// Replace the incorrect implementation with proper Keccak-256
io::UInt256 Hash::Keccak256Proper(const io::ByteSpan& data)
{
    uint8_t hash[32];
    keccak_internal::keccak256(data.Data(), data.Size(), hash);
    return io::UInt256(io::ByteSpan(hash, 32));
}
}  // namespace neo::cryptography