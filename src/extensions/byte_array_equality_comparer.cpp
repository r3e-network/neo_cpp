#include <neo/extensions/byte_array_equality_comparer.h>
#include <cstring>
#include <algorithm>

namespace neo::extensions
{
    bool ByteArrayEqualityComparer::Equals(const io::ByteSpan& left, const io::ByteSpan& right)
    {
        // Fast size comparison first
        if (left.Size() != right.Size())
            return false;
            
        // Handle empty arrays
        if (left.Size() == 0)
            return true;
            
        // Use memcmp for fast byte-by-byte comparison
        return std::memcmp(left.Data(), right.Data(), left.Size()) == 0;
    }

    bool ByteArrayEqualityComparer::Equals(const io::ByteVector& left, const io::ByteVector& right)
    {
        return Equals(left.AsSpan(), right.AsSpan());
    }

    bool ByteArrayEqualityComparer::Equals(const std::vector<uint8_t>& left, const std::vector<uint8_t>& right)
    {
        // Use built-in vector equality which is already optimized
        return left == right;
    }

    size_t ByteArrayEqualityComparer::GetHashCode(const io::ByteSpan& data)
    {
        if (data.Size() == 0)
            return 0;
            
        // Use FNV-1a hash algorithm for consistent hashing
        constexpr size_t fnvOffsetBasis = 14695981039346656037ULL;
        constexpr size_t fnvPrime = 1099511628211ULL;
        
        size_t hash = fnvOffsetBasis;
        for (size_t i = 0; i < data.Size(); ++i)
        {
            hash ^= static_cast<size_t>(data[i]);
            hash *= fnvPrime;
        }
        
        return hash;
    }

    size_t ByteArrayEqualityComparer::GetHashCode(const io::ByteVector& data)
    {
        return GetHashCode(data.AsSpan());
    }

    size_t ByteArrayEqualityComparer::GetHashCode(const std::vector<uint8_t>& data)
    {
        if (data.empty())
            return 0;
            
        // Use FNV-1a hash algorithm
        constexpr size_t fnvOffsetBasis = 14695981039346656037ULL;
        constexpr size_t fnvPrime = 1099511628211ULL;
        
        size_t hash = fnvOffsetBasis;
        for (uint8_t byte : data)
        {
            hash ^= static_cast<size_t>(byte);
            hash *= fnvPrime;
        }
        
        return hash;
    }
}
