#pragma once

#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/io/byte_vector.h>
#include <neo/io/caching/lru_cache.h>

#include <functional>

namespace neo::io::caching
{
/**
 * @brief A hash function for ByteVector.
 */
struct ByteVectorHash
{
    /**
     * @brief Computes the hash of a ByteVector.
     * @param bytes The ByteVector to hash.
     * @return The hash value.
     */
    size_t operator()(const ByteVector& bytes) const
    {
        size_t hash = 0;
        for (size_t i = 0; i < bytes.Size(); ++i)
        {
            hash = hash * 31 + bytes[i];
        }
        return hash;
    }
};

/**
 * @brief An equality function for ByteVector.
 */
struct ByteVectorEqual
{
    /**
     * @brief Checks if two ByteVectors are equal.
     * @param lhs The first ByteVector.
     * @param rhs The second ByteVector.
     * @return True if the ByteVectors are equal, false otherwise.
     */
    bool operator()(const ByteVector& lhs, const ByteVector& rhs) const
    {
        if (lhs.Size() != rhs.Size()) return false;

        for (size_t i = 0; i < lhs.Size(); ++i)
        {
            if (lhs[i] != rhs[i]) return false;
        }

        return true;
    }
};

/**
 * @brief A cache for ECPoint objects.
 */
class ECPointCache
{
   public:
    /**
     * @brief Constructs an ECPointCache with the specified capacity.
     * @param capacity The maximum number of items the cache can hold.
     */
    explicit ECPointCache(size_t capacity) : cache_(capacity) {}

    /**
     * @brief Gets an ECPoint from the cache or creates a new one.
     * @param bytes The encoded ECPoint.
     * @param curve The elliptic curve.
     * @return The ECPoint.
     */
    std::shared_ptr<cryptography::ecc::ECPoint> GetOrCreate(const ByteVector& bytes,
                                                            std::shared_ptr<cryptography::ecc::ECCurve> curve)
    {
        // Try to get from cache
        auto ecpoint = cache_.Get(bytes);
        if (ecpoint) return ecpoint.value();

        // Create new ECPoint
        auto newECPoint = cryptography::ecc::ECPoint::DecodePoint(bytes.AsSpan(), curve);

        // Add to cache
        cache_.Add(bytes, newECPoint);

        return newECPoint;
    }

    /**
     * @brief Gets an ECPoint from the cache.
     * @param bytes The encoded ECPoint.
     * @return The ECPoint if found, std::nullopt otherwise.
     */
    std::optional<std::shared_ptr<cryptography::ecc::ECPoint>> Get(const ByteVector& bytes)
    {
        return cache_.Get(bytes);
    }

    /**
     * @brief Adds an ECPoint to the cache.
     * @param ecpoint The ECPoint to add.
     */
    void Add(std::shared_ptr<cryptography::ecc::ECPoint> ecpoint)
    {
        if (!ecpoint) return;

        // Encode ECPoint
        auto bytes = ecpoint->EncodePoint(true);

        // Add to cache
        cache_.Add(bytes, ecpoint);
    }

    /**
     * @brief Clears the cache.
     */
    void Clear() { cache_.Clear(); }

    /**
     * @brief Gets the number of items in the cache.
     * @return The number of items in the cache.
     */
    size_t Size() const { return cache_.Size(); }

    /**
     * @brief Gets the capacity of the cache.
     * @return The capacity of the cache.
     */
    size_t Capacity() const { return cache_.Capacity(); }

   private:
    LRUCache<ByteVector, std::shared_ptr<cryptography::ecc::ECPoint>, ByteVectorHash> cache_;
};
}  // namespace neo::io::caching
