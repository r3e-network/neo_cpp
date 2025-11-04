#include <neo/cryptography/scrypt.h>

#include <neo/cryptography/crypto.h>
#include <neo/io/byte_span.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>

namespace neo::cryptography
{
namespace
{
constexpr size_t kBlockBytes = 64;   // 16 * 4
constexpr size_t kBlockWords = 16;   // 16 uint32_t words per block

inline uint32_t rotl32(uint32_t value, int shift)
{
    return static_cast<uint32_t>((value << shift) | (value >> (32 - shift)));
}

inline uint32_t load_le32(const uint8_t* src)
{
    uint32_t value;
    std::memcpy(&value, src, sizeof(uint32_t));
    return value;
}

inline void store_le32(uint8_t* dest, uint32_t value) { std::memcpy(dest, &value, sizeof(uint32_t)); }

inline void bytes_to_words(const uint8_t* src, uint32_t* dest, size_t wordCount)
{
    for (size_t i = 0; i < wordCount; ++i)
    {
        dest[i] = load_le32(src + i * sizeof(uint32_t));
    }
}

inline void words_to_bytes(const uint32_t* src, uint8_t* dest, size_t wordCount)
{
    for (size_t i = 0; i < wordCount; ++i)
    {
        store_le32(dest + i * sizeof(uint32_t), src[i]);
    }
}

void salsa20_8(std::array<uint32_t, kBlockWords>& block)
{
    std::array<uint32_t, kBlockWords> x = block;

    for (int i = 0; i < 8; i += 2)
    {
        // column rounds
        x[4] ^= rotl32(x[0] + x[12], 7);
        x[8] ^= rotl32(x[4] + x[0], 9);
        x[12] ^= rotl32(x[8] + x[4], 13);
        x[0] ^= rotl32(x[12] + x[8], 18);

        x[9] ^= rotl32(x[5] + x[1], 7);
        x[13] ^= rotl32(x[9] + x[5], 9);
        x[1] ^= rotl32(x[13] + x[9], 13);
        x[5] ^= rotl32(x[1] + x[13], 18);

        x[14] ^= rotl32(x[10] + x[6], 7);
        x[2] ^= rotl32(x[14] + x[10], 9);
        x[6] ^= rotl32(x[2] + x[14], 13);
        x[10] ^= rotl32(x[6] + x[2], 18);

        x[3] ^= rotl32(x[15] + x[11], 7);
        x[7] ^= rotl32(x[3] + x[15], 9);
        x[11] ^= rotl32(x[7] + x[3], 13);
        x[15] ^= rotl32(x[11] + x[7], 18);

        // row rounds
        x[1] ^= rotl32(x[0] + x[3], 7);
        x[2] ^= rotl32(x[1] + x[0], 9);
        x[3] ^= rotl32(x[2] + x[1], 13);
        x[0] ^= rotl32(x[3] + x[2], 18);

        x[6] ^= rotl32(x[5] + x[4], 7);
        x[7] ^= rotl32(x[6] + x[5], 9);
        x[4] ^= rotl32(x[7] + x[6], 13);
        x[5] ^= rotl32(x[4] + x[7], 18);

        x[11] ^= rotl32(x[10] + x[9], 7);
        x[8] ^= rotl32(x[11] + x[10], 9);
        x[9] ^= rotl32(x[8] + x[11], 13);
        x[10] ^= rotl32(x[9] + x[8], 18);

        x[12] ^= rotl32(x[15] + x[14], 7);
        x[13] ^= rotl32(x[12] + x[15], 9);
        x[14] ^= rotl32(x[13] + x[12], 13);
        x[15] ^= rotl32(x[14] + x[13], 18);
    }

    for (size_t i = 0; i < kBlockWords; ++i)
    {
        block[i] += x[i];
    }
}

uint64_t integerify(const uint32_t* block, uint32_t r)
{
    const size_t offset = (static_cast<size_t>(2 * r) - 1) * kBlockWords;
    return static_cast<uint64_t>(block[offset]) | (static_cast<uint64_t>(block[offset + 1]) << 32);
}

void block_mix(uint32_t* B, uint32_t* Y, uint32_t r)
{
    std::array<uint32_t, kBlockWords> X{};
    std::copy_n(B + static_cast<size_t>(2 * r - 1) * kBlockWords, kBlockWords, X.begin());

    for (uint32_t i = 0; i < 2 * r; ++i)
    {
        uint32_t* chunk = B + static_cast<size_t>(i) * kBlockWords;
        for (size_t j = 0; j < kBlockWords; ++j)
        {
            X[j] ^= chunk[j];
        }

        salsa20_8(X);

        if ((i & 1) == 0)
        {
            std::copy_n(X.begin(), kBlockWords, Y + static_cast<size_t>(i / 2) * kBlockWords);
        }
        else
        {
            std::copy_n(X.begin(), kBlockWords, Y + static_cast<size_t>(r + (i - 1) / 2) * kBlockWords);
        }
    }

    std::copy_n(Y, static_cast<size_t>(2 * r) * kBlockWords, B);
}

void smix(uint8_t* block, uint32_t r, uint32_t N, uint32_t* V, uint32_t* XY)
{
    const size_t blockWords = static_cast<size_t>(32) * r;
    uint32_t* X = XY;
    uint32_t* Y = XY + blockWords;

    bytes_to_words(block, X, blockWords);

    for (uint32_t i = 0; i < N; ++i)
    {
        std::copy_n(X, blockWords, V + static_cast<size_t>(i) * blockWords);
        block_mix(X, Y, r);
    }

    for (uint32_t i = 0; i < N; ++i)
    {
        uint64_t j = integerify(X, r) & (static_cast<uint64_t>(N) - 1);
        uint32_t* vBlock = V + static_cast<size_t>(j) * blockWords;
        for (size_t k = 0; k < blockWords; ++k)
        {
            X[k] ^= vBlock[k];
        }
        block_mix(X, Y, r);
    }

    words_to_bytes(X, block, blockWords);
}

bool is_power_of_two(uint32_t value) { return value != 0 && (value & (value - 1)) == 0; }

void check_parameters(uint32_t N, uint32_t r, uint32_t p, uint32_t dkLen)
{
    if (!is_power_of_two(N) || N < 2)
    {
        throw std::invalid_argument("Scrypt: N must be > 1 and a power of two");
    }

    if (r == 0 || p == 0)
    {
        throw std::invalid_argument("Scrypt: r and p must be greater than zero");
    }

    if (dkLen == 0)
    {
        throw std::invalid_argument("Scrypt: derived key length must be greater than zero");
    }

    // Overflow checks based on RFC 7914 reference implementation.
    const uint64_t r64 = r;
    const uint64_t p64 = p;

    if (r64 > (std::numeric_limits<uint64_t>::max() / 128 / p64))
    {
        throw std::overflow_error("Scrypt: r * p overflow");
    }

    if (N > std::numeric_limits<uint32_t>::max() / 2)
    {
        throw std::overflow_error("Scrypt: N is too large");
    }

    if (r64 > (std::numeric_limits<uint64_t>::max() / 256))
    {
        throw std::overflow_error("Scrypt: r is too large");
    }

    if (static_cast<uint64_t>(dkLen) > static_cast<uint64_t>(std::numeric_limits<int>::max()))
    {
        throw std::overflow_error("Scrypt: derived key length too large");
    }
}
}  // namespace

std::vector<uint8_t> Scrypt::DeriveKey(const std::string& password, const std::vector<uint8_t>& salt, uint32_t N,
                                       uint32_t r, uint32_t p, uint32_t dkLen)
{
    std::vector<uint8_t> passwordBytes(password.begin(), password.end());
    return DeriveKey(passwordBytes, salt, N, r, p, dkLen);
}

std::vector<uint8_t> Scrypt::DeriveKey(const std::vector<uint8_t>& password, const std::vector<uint8_t>& salt,
                                       uint32_t N, uint32_t r, uint32_t p, uint32_t dkLen)
{
    check_parameters(N, r, p, dkLen);

    const size_t blockSize = static_cast<size_t>(128) * r;
    const size_t BSize = blockSize * static_cast<size_t>(p);

    io::ByteSpan passwordSpan(password.data(), password.size());
    io::ByteSpan saltSpan(salt.data(), salt.size());

    auto BByteVector = Crypto::PBKDF2(passwordSpan, saltSpan, 1, static_cast<int>(BSize));
    std::vector<uint8_t> B(BByteVector.begin(), BByteVector.end());

    const size_t blockWords = static_cast<size_t>(32) * r;
    std::vector<uint32_t> V(static_cast<size_t>(N) * blockWords);
    std::vector<uint32_t> XY(blockWords * 2);

    for (uint32_t i = 0; i < p; ++i)
    {
        smix(B.data() + static_cast<size_t>(i) * blockSize, r, N, V.data(), XY.data());
    }

    io::ByteSpan mixedSpan(B.data(), B.size());
    auto derived = Crypto::PBKDF2(passwordSpan, mixedSpan, 1, static_cast<int>(dkLen));

    std::fill(B.begin(), B.end(), 0);
    std::fill(V.begin(), V.end(), 0);
    std::fill(XY.begin(), XY.end(), 0);

    return std::vector<uint8_t>(derived.begin(), derived.end());
}
}  // namespace neo::cryptography
