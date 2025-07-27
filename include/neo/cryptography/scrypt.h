#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace neo::cryptography
{
/**
 * @brief Scrypt password-based key derivation function.
 *
 * Used by NEP6 wallets for secure password-based encryption.
 */
class Scrypt
{
  public:
    /**
     * @brief Derives a key using the scrypt algorithm.
     * @param password The password.
     * @param salt The salt.
     * @param n CPU/memory cost parameter.
     * @param r Block size parameter.
     * @param p Parallelization parameter.
     * @param dkLen Derived key length.
     * @return The derived key.
     */
    static std::vector<uint8_t> DeriveKey(const std::string& password, const std::vector<uint8_t>& salt,
                                          uint32_t n = 16384, uint32_t r = 8, uint32_t p = 8, uint32_t dkLen = 64);

    /**
     * @brief Derives a key using the scrypt algorithm with byte array password.
     * @param password The password as bytes.
     * @param salt The salt.
     * @param n CPU/memory cost parameter.
     * @param r Block size parameter.
     * @param p Parallelization parameter.
     * @param dkLen Derived key length.
     * @return The derived key.
     */
    static std::vector<uint8_t> DeriveKey(const std::vector<uint8_t>& password, const std::vector<uint8_t>& salt,
                                          uint32_t n = 16384, uint32_t r = 8, uint32_t p = 8, uint32_t dkLen = 64);

  private:
    static void BlockMix(std::vector<uint8_t>& B, uint32_t r);
    static void Salsa20_8(std::vector<uint8_t>& B);
    static void XorBytes(std::vector<uint8_t>& dest, const std::vector<uint8_t>& src);
};
}  // namespace neo::cryptography