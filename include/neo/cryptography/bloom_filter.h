#pragma once

#include <neo/io/byte_vector.h>
#include <neo/cryptography/hash.h>
#include <vector>
#include <cmath>
#include <functional>

namespace neo {
namespace cryptography {

/**
 * @brief Bloom filter implementation for probabilistic membership testing
 */
class BloomFilter {
public:
    /**
     * @brief Constructor
     * @param numElements Expected number of elements
     * @param falsePositiveRate Desired false positive rate (0 < rate < 1)
     */
    BloomFilter(size_t numElements, double falsePositiveRate = 0.01) {
        // Calculate optimal bit array size
        size_ = static_cast<size_t>(-numElements * std::log(falsePositiveRate) / (std::log(2) * std::log(2)));
        
        // Calculate optimal number of hash functions
        numHashes_ = static_cast<size_t>(size_ * std::log(2) / numElements);
        
        // Initialize bit array
        bits_.resize((size_ + 7) / 8, 0);
    }
    
    /**
     * @brief Add an element to the filter
     * @param data Element to add
     */
    void Add(const io::ByteVector& data) {
        auto hashes = GetHashes(data);
        for (size_t i = 0; i < numHashes_; i++) {
            size_t bitPos = hashes[i] % size_;
            bits_[bitPos / 8] |= (1 << (bitPos % 8));
        }
    }
    
    /**
     * @brief Check if an element might be in the filter
     * @param data Element to check
     * @return true if possibly in set, false if definitely not in set
     */
    bool Contains(const io::ByteVector& data) const {
        auto hashes = GetHashes(data);
        for (size_t i = 0; i < numHashes_; i++) {
            size_t bitPos = hashes[i] % size_;
            if ((bits_[bitPos / 8] & (1 << (bitPos % 8))) == 0) {
                return false;
            }
        }
        return true;
    }
    
    /**
     * @brief Clear the filter
     */
    void Clear() {
        std::fill(bits_.begin(), bits_.end(), 0);
    }
    
    /**
     * @brief Get the serialized filter data
     * @return Byte vector of the filter
     */
    io::ByteVector ToByteArray() const {
        return io::ByteVector(bits_.data(), bits_.size());
    }
    
    /**
     * @brief Get the size of the bit array
     * @return Size in bits
     */
    size_t GetSize() const {
        return size_;
    }
    
    /**
     * @brief Get the number of hash functions
     * @return Number of hash functions
     */
    size_t GetNumHashes() const {
        return numHashes_;
    }
    
    /**
     * @brief Calculate the false positive rate for current filter state
     * @param numElements Number of elements added
     * @return Estimated false positive rate
     */
    double GetFalsePositiveRate(size_t numElements) const {
        if (numElements == 0) return 0.0;
        double ratio = static_cast<double>(numElements) / size_;
        return std::pow(1 - std::exp(-numHashes_ * ratio), numHashes_);
    }

private:
    /**
     * @brief Generate hash values for data
     * @param data Input data
     * @return Vector of hash values
     */
    std::vector<size_t> GetHashes(const io::ByteVector& data) const {
        std::vector<size_t> hashes;
        hashes.reserve(numHashes_);
        
        // Use SHA256 as base hash and derive multiple hashes
        auto hash1 = Hash::Sha256(data.AsSpan());
        auto hash2 = Hash::Sha256(hash1.AsSpan());
        
        for (size_t i = 0; i < numHashes_; i++) {
            // Combine hashes to generate multiple hash values
            size_t h = 0;
            for (size_t j = 0; j < sizeof(size_t); j++) {
                size_t byte1 = hash1.ToArray()[(i * sizeof(size_t) + j) % 32];
                size_t byte2 = hash2.ToArray()[(i * sizeof(size_t) + j) % 32];
                h = (h << 8) | (byte1 ^ byte2);
            }
            hashes.push_back(h);
        }
        
        return hashes;
    }

private:
    size_t size_;                  // Size of bit array in bits
    size_t numHashes_;             // Number of hash functions
    std::vector<uint8_t> bits_;    // Bit array
};

} // namespace cryptography
} // namespace neo