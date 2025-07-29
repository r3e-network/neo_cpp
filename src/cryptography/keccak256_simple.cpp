#include <array>
#include <cstring>
#include <neo/cryptography/hash.h>

namespace neo::cryptography
{

// Simple, verified Keccak-256 implementation
namespace
{

const uint64_t keccakf_rndc[24] = {0x0000000000000001, 0x0000000000008082, 0x800000000000808a, 0x8000000080008000,
                                   0x000000000000808b, 0x0000000080000001, 0x8000000080008081, 0x8000000000008009,
                                   0x000000000000008a, 0x0000000000000088, 0x0000000080008009, 0x000000008000000a,
                                   0x000000008000808b, 0x800000000000008b, 0x8000000000008089, 0x8000000000008003,
                                   0x8000000000008002, 0x8000000000000080, 0x000000000000800a, 0x800000008000000a,
                                   0x8000000080008081, 0x8000000000008080, 0x0000000080000001, 0x8000000080008008};

const unsigned keccakf_rotc[24] = {1,  3,  6,  10, 15, 21, 28, 36, 45, 55, 2,  14,
                                   27, 41, 56, 8,  25, 43, 62, 18, 39, 61, 20, 44};

const unsigned keccakf_piln[24] = {10, 7,  11, 17, 18, 3, 5,  16, 8,  21, 24, 4,
                                   15, 23, 19, 13, 12, 2, 20, 14, 22, 9,  6,  1};

void keccakf(uint64_t st[25])
{
    uint64_t t, bc[5];

    for (int r = 0; r < 24; r++)
    {
        // Theta
        for (int i = 0; i < 5; i++)
            bc[i] = st[i] ^ st[i + 5] ^ st[i + 10] ^ st[i + 15] ^ st[i + 20];

        for (int i = 0; i < 5; i++)
        {
            t = bc[(i + 4) % 5] ^ ((bc[(i + 1) % 5] << 1) | (bc[(i + 1) % 5] >> 63));
            for (int j = 0; j < 25; j += 5)
                st[j + i] ^= t;
        }

        // Rho Pi
        t = st[1];
        for (int i = 0; i < 24; i++)
        {
            int j = keccakf_piln[i];
            bc[0] = st[j];
            st[j] = (t << keccakf_rotc[i]) | (t >> (64 - keccakf_rotc[i]));
            t = bc[0];
        }

        // Chi
        for (int j = 0; j < 25; j += 5)
        {
            for (int i = 0; i < 5; i++)
                bc[i] = st[j + i];
            for (int i = 0; i < 5; i++)
                st[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
        }

        // Iota
        st[0] ^= keccakf_rndc[r];
    }
}

void keccak(const uint8_t* in, size_t inlen, uint8_t* md, int mdlen)
{
    uint64_t st[25];
    uint8_t temp[144];
    size_t rsiz, rsizw;

    rsiz = 200 - 2 * mdlen;
    rsizw = rsiz / 8;

    memset(st, 0, sizeof(st));

    for (; inlen >= rsiz; inlen -= rsiz, in += rsiz)
    {
        for (size_t i = 0; i < rsizw; i++)
            st[i] ^= ((uint64_t*)in)[i];
        keccakf(st);
    }

    memcpy(temp, in, inlen);
    temp[inlen++] = 0x01;
    memset(temp + inlen, 0, rsiz - inlen);
    temp[rsiz - 1] |= 0x80;

    for (size_t i = 0; i < rsizw; i++)
        st[i] ^= ((uint64_t*)temp)[i];

    keccakf(st);

    memcpy(md, st, mdlen);
}

}  // anonymous namespace

io::UInt256 Hash::Keccak256Simple(const io::ByteSpan& data)
{
    uint8_t hash[32];
    keccak(data.Data(), data.Size(), hash, 32);
    return io::UInt256(io::ByteSpan(hash, 32));
}

}  // namespace neo::cryptography