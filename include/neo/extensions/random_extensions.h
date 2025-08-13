/**
 * @file random_extensions.h
 * @brief Random Extensions
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>

#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace neo::extensions
{
/**
 * @brief Extensions for random number generation.
 *
 * ## Overview
 * Provides cryptographically secure random number generation methods
 * for blockchain operations, including random bytes, numbers, and identifiers.
 *
 * ## API Reference
 * - **Secure Random**: Cryptographically secure random generation
 * - **Basic Types**: Random integers, floats, booleans
 * - **Blockchain Types**: Random hashes, addresses, nonces
 * - **Collections**: Random elements from containers
 *
 * ## Usage Examples
 * ```cpp
 * // Generate random bytes
 * auto randomBytes = RandomExtensions::GenerateRandomBytes(32);
 *
 * // Generate random integer
 * auto randomInt = RandomExtensions::NextInt(1, 100);
 *
 * // Generate random UInt256
 * auto randomHash = RandomExtensions::GenerateRandomUInt256();
 * ```
 */
class RandomExtensions
{
   public:
    /**
     * @brief Generate cryptographically secure random bytes
     * @param length Number of bytes to generate
     * @return Vector of random bytes
     */
    static io::ByteVector GenerateRandomBytes(size_t length);

    /**
     * @brief Generate random integer in range [min, max]
     * @param min Minimum value (inclusive)
     * @param max Maximum value (inclusive)
     * @return Random integer in range
     */
    static int32_t NextInt(int32_t min, int32_t max);

    /**
     * @brief Generate random integer in range [0, max]
     * @param max Maximum value (inclusive)
     * @return Random integer in range
     */
    static int32_t NextInt(int32_t max) { return NextInt(0, max); }

    /**
     * @brief Generate random integer (full range)
     * @return Random 32-bit integer
     */
    static int32_t NextInt()
    {
        return NextInt(std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max());
    }

    /**
     * @brief Generate random unsigned integer in range [min, max]
     * @param min Minimum value (inclusive)
     * @param max Maximum value (inclusive)
     * @return Random unsigned integer in range
     */
    static uint32_t NextUInt(uint32_t min, uint32_t max);

    /**
     * @brief Generate random unsigned integer in range [0, max]
     * @param max Maximum value (inclusive)
     * @return Random unsigned integer in range
     */
    static uint32_t NextUInt(uint32_t max) { return NextUInt(0, max); }

    /**
     * @brief Generate random unsigned integer (full range)
     * @return Random 32-bit unsigned integer
     */
    static uint32_t NextUInt() { return NextUInt(0, std::numeric_limits<uint32_t>::max()); }

    /**
     * @brief Generate random 64-bit integer in range [min, max]
     * @param min Minimum value (inclusive)
     * @param max Maximum value (inclusive)
     * @return Random 64-bit integer in range
     */
    static int64_t NextLong(int64_t min, int64_t max);

    /**
     * @brief Generate random 64-bit integer (full range)
     * @return Random 64-bit integer
     */
    static int64_t NextLong()
    {
        return NextLong(std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max());
    }

    /**
     * @brief Generate random 64-bit unsigned integer in range [min, max]
     * @param min Minimum value (inclusive)
     * @param max Maximum value (inclusive)
     * @return Random 64-bit unsigned integer in range
     */
    static uint64_t NextULong(uint64_t min, uint64_t max);

    /**
     * @brief Generate random 64-bit unsigned integer (full range)
     * @return Random 64-bit unsigned integer
     */
    static uint64_t NextULong() { return NextULong(0, std::numeric_limits<uint64_t>::max()); }

    /**
     * @brief Generate random float in range [0.0, 1.0)
     * @return Random float
     */
    static float NextFloat();

    /**
     * @brief Generate random float in range [min, max)
     * @param min Minimum value (inclusive)
     * @param max Maximum value (exclusive)
     * @return Random float in range
     */
    static float NextFloat(float min, float max);

    /**
     * @brief Generate random double in range [0.0, 1.0)
     * @return Random double
     */
    static double NextDouble();

    /**
     * @brief Generate random double in range [min, max)
     * @param min Minimum value (inclusive)
     * @param max Maximum value (exclusive)
     * @return Random double in range
     */
    static double NextDouble(double min, double max);

    /**
     * @brief Generate random boolean
     * @return Random boolean value
     */
    static bool NextBool() { return NextInt(0, 1) == 1; }

    /**
     * @brief Generate random boolean with specified probability
     * @param probability Probability of returning true (0.0 to 1.0)
     * @return Random boolean based on probability
     */
    static bool NextBool(double probability) { return NextDouble() < probability; }

    /**
     * @brief Generate random UInt160 hash
     * @return Random UInt160
     */
    static io::UInt160 GenerateRandomUInt160();

    /**
     * @brief Generate random UInt256 hash
     * @return Random UInt256
     */
    static io::UInt256 GenerateRandomUInt256();

    /**
     * @brief Generate random alphanumeric string
     * @param length Length of the string
     * @return Random alphanumeric string
     */
    static std::string GenerateRandomString(size_t length);

    /**
     * @brief Generate random hex string
     * @param length Length of the string (in characters)
     * @return Random hex string
     */
    static std::string GenerateRandomHexString(size_t length);

    /**
     * @brief Shuffle a vector in place
     * @tparam T Element type
     * @param vec Vector to shuffle
     */
    template <typename T>
    static void Shuffle(std::vector<T>& vec)
    {
        auto& rng = GetSecureRNG();
        std::shuffle(vec.begin(), vec.end(), rng);
    }

    /**
     * @brief Select random element from vector
     * @tparam T Element type
     * @param vec Vector to select from
     * @return Random element
     */
    template <typename T>
    static const T& SelectRandom(const std::vector<T>& vec)
    {
        if (vec.empty()) throw std::invalid_argument("Cannot select from empty vector");

        auto index = NextUInt(0, static_cast<uint32_t>(vec.size() - 1));
        return vec[index];
    }

    /**
     * @brief Select multiple random elements from vector (without replacement)
     * @tparam T Element type
     * @param vec Vector to select from
     * @param count Number of elements to select
     * @return Vector of selected elements
     */
    template <typename T>
    static std::vector<T> SelectRandomMultiple(const std::vector<T>& vec, size_t count)
    {
        if (count > vec.size()) throw std::invalid_argument("Cannot select more elements than available");

        std::vector<T> result = vec;
        Shuffle(result);
        result.resize(count);
        return result;
    }

    /**
     * @brief Generate random nonce for blockchain operations
     * @return Random 32-bit nonce
     */
    static uint32_t GenerateNonce() { return NextUInt(); }

    /**
     * @brief Generate random timestamp within reasonable bounds
     * @param baseTime Base timestamp (default: current time)
     * @param maxVariation Maximum variation in seconds (default: 1 hour)
     * @return Random timestamp
     */
    static uint64_t GenerateRandomTimestamp(uint64_t baseTime = 0, uint64_t maxVariation = 3600);

   private:
    /**
     * @brief Get secure random number generator
     * @return Reference to thread-local secure RNG
     */
    static std::mt19937& GetSecureRNG();

    /**
     * @brief Seed the RNG with secure random data
     */
    static void SeedSecureRNG(std::mt19937& rng);
};
}  // namespace neo::extensions
