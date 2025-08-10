#pragma once

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace neo::vm::internal
{
/**
 * @brief A non-owning view of a byte array.
 */
class ByteSpan
{
   public:
    /**
     * @brief Constructs an empty ByteSpan.
     */
    ByteSpan() noexcept : data_(nullptr), size_(0) {}

    /**
     * @brief Constructs a ByteSpan from a pointer and size.
     * @param data Pointer to the data.
     * @param size Size of the data.
     */
    ByteSpan(const uint8_t* data, size_t size) noexcept : data_(data), size_(size) {}

    /**
     * @brief Constructs a ByteSpan from a vector.
     * @param data Vector containing the data.
     */
    template <typename Allocator>
    ByteSpan(const std::vector<uint8_t, Allocator>& data) noexcept : data_(data.data()), size_(data.size())
    {
    }

    /**
     * @brief Gets the size of the ByteSpan.
     * @return The size of the ByteSpan.
     */
    size_t Size() const noexcept { return size_; }

    /**
     * @brief Checks if the ByteSpan is empty.
     * @return True if the ByteSpan is empty, false otherwise.
     */
    bool IsEmpty() const noexcept { return size_ == 0; }

    /**
     * @brief Gets a pointer to the data.
     * @return Pointer to the data.
     */
    const uint8_t* Data() const noexcept { return data_; }

    /**
     * @brief Gets the byte at the specified index.
     * @param index The index.
     * @return The byte at the specified index.
     */
    uint8_t operator[](size_t index) const
    {
        if (index >= size_) throw std::out_of_range("Index out of range");
        return data_[index];
    }

    /**
     * @brief Compares this ByteSpan with another ByteSpan.
     * @param other The other ByteSpan.
     * @return True if the ByteSpans are equal, false otherwise.
     */
    bool operator==(const ByteSpan& other) const noexcept
    {
        if (size_ != other.size_) return false;
        return std::memcmp(data_, other.data_, size_) == 0;
    }

    /**
     * @brief Compares this ByteSpan with another ByteSpan.
     * @param other The other ByteSpan.
     * @return True if the ByteSpans are not equal, false otherwise.
     */
    bool operator!=(const ByteSpan& other) const noexcept { return !(*this == other); }

    /**
     * @brief Creates a new ByteSpan that is a slice of this ByteSpan.
     * @param start The start index.
     * @param length The length of the slice.
     * @return A new ByteSpan that is a slice of this ByteSpan.
     */
    ByteSpan Slice(size_t start, size_t length) const
    {
        if (start + length > size_) throw std::out_of_range("Slice out of range");
        return ByteSpan(data_ + start, length);
    }

    /**
     * @brief Converts the ByteSpan to a hexadecimal string.
     * @return The hexadecimal string representation of the ByteSpan.
     */
    std::string ToHexString() const
    {
        std::string result;
        result.reserve(size_ * 2);

        for (size_t i = 0; i < size_; i++)
        {
            static const char* hex = "0123456789abcdef";
            result.push_back(hex[(data_[i] >> 4) & 0xF]);
            result.push_back(hex[data_[i] & 0xF]);
        }

        return result;
    }

   private:
    const uint8_t* data_;
    size_t size_;
};
}  // namespace neo::vm::internal
