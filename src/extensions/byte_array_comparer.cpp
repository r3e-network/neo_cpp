#include <neo/extensions/byte_array_comparer.h>

#include <algorithm>
#include <cstring>

namespace neo::extensions
{
int ByteArrayComparer::Compare(const io::ByteSpan& left, const io::ByteSpan& right)
{
    size_t minSize = std::min(left.Size(), right.Size());

    if (minSize > 0)
    {
        int result = std::memcmp(left.Data(), right.Data(), minSize);
        if (result != 0) return (result < 0) ? -1 : 1;
    }

    // If common portion is equal, compare sizes
    if (left.Size() < right.Size())
        return -1;
    else if (left.Size() > right.Size())
        return 1;
    else
        return 0;
}

int ByteArrayComparer::Compare(const io::ByteVector& left, const io::ByteVector& right)
{
    return Compare(left.AsSpan(), right.AsSpan());
}

int ByteArrayComparer::Compare(const std::vector<uint8_t>& left, const std::vector<uint8_t>& right)
{
    auto result = std::lexicographical_compare_three_way(left.begin(), left.end(), right.begin(), right.end());

    if (result < 0) return -1;
    if (result > 0) return 1;
    return 0;
}

bool ByteArrayComparer::Equals(const io::ByteSpan& left, const io::ByteSpan& right)
{
    if (left.Size() != right.Size()) return false;

    if (left.Size() == 0) return true;

    return std::memcmp(left.Data(), right.Data(), left.Size()) == 0;
}

bool ByteArrayComparer::Equals(const io::ByteVector& left, const io::ByteVector& right)
{
    return Equals(left.AsSpan(), right.AsSpan());
}

bool ByteArrayComparer::Equals(const std::vector<uint8_t>& left, const std::vector<uint8_t>& right)
{
    return left == right;
}

bool ByteArrayComparer::StartsWith(const io::ByteSpan& left, const io::ByteSpan& right)
{
    if (right.Size() > left.Size()) return false;

    if (right.Size() == 0) return true;

    return std::memcmp(left.Data(), right.Data(), right.Size()) == 0;
}

bool ByteArrayComparer::EndsWith(const io::ByteSpan& left, const io::ByteSpan& right)
{
    if (right.Size() > left.Size()) return false;

    if (right.Size() == 0) return true;

    size_t offset = left.Size() - right.Size();
    return std::memcmp(left.Data() + offset, right.Data(), right.Size()) == 0;
}

size_t ByteArrayComparer::GetHashCode(const io::ByteSpan& data)
{
    // FNV-1a hash algorithm
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

size_t ByteArrayComparer::GetHashCode(const io::ByteVector& data) { return GetHashCode(data.AsSpan()); }

size_t ByteArrayComparer::FindFirst(const io::ByteSpan& data, const io::ByteSpan& pattern)
{
    if (pattern.Size() == 0 || pattern.Size() > data.Size()) return SIZE_MAX;

    for (size_t i = 0; i <= data.Size() - pattern.Size(); ++i)
    {
        if (std::memcmp(data.Data() + i, pattern.Data(), pattern.Size()) == 0) return i;
    }

    return SIZE_MAX;
}

size_t ByteArrayComparer::FindLast(const io::ByteSpan& data, const io::ByteSpan& pattern)
{
    if (pattern.Size() == 0 || pattern.Size() > data.Size()) return SIZE_MAX;

    for (size_t i = data.Size() - pattern.Size(); i != SIZE_MAX; --i)
    {
        if (std::memcmp(data.Data() + i, pattern.Data(), pattern.Size()) == 0) return i;
    }

    return SIZE_MAX;
}

const io::ByteVector& ByteArrayComparer::Min(const io::ByteVector& left, const io::ByteVector& right)
{
    return (Compare(left, right) <= 0) ? left : right;
}

const io::ByteVector& ByteArrayComparer::Max(const io::ByteVector& left, const io::ByteVector& right)
{
    return (Compare(left, right) >= 0) ? left : right;
}
}  // namespace neo::extensions
