#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <stdexcept>

namespace neo::io
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
        ByteSpan(const std::vector<uint8_t, Allocator>& data) noexcept
            : data_(data.data()), size_(data.size()) {}

        /**
         * @brief Gets the size of the ByteSpan.
         * @return The size of the ByteSpan.
         */
        size_t Size() const noexcept { return size_; }

        /**
         * @brief Gets the size of the ByteSpan (STL compatibility).
         * @return The size of the ByteSpan.
         */
        size_t size() const noexcept { return size_; }

        /**
         * @brief Checks if the ByteSpan is empty.
         * @return True if the ByteSpan is empty, false otherwise.
         */
        bool IsEmpty() const noexcept { return size_ == 0; }

        /**
         * @brief Checks if the ByteSpan is empty (STL compatibility).
         * @return True if the ByteSpan is empty, false otherwise.
         */
        bool empty() const noexcept { return size_ == 0; }

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
        uint8_t operator[](size_t index) const { return data_[index]; }

        /**
         * @brief Gets an iterator to the beginning of the ByteSpan.
         * @return Iterator to the beginning.
         */
        const uint8_t* begin() const noexcept { return data_; }

        /**
         * @brief Gets an iterator to the end of the ByteSpan.
         * @return Iterator to the end.
         */
        const uint8_t* end() const noexcept { return data_ + size_; }

        /**
         * @brief Gets a subspan of this ByteSpan.
         * @param offset The offset of the subspan.
         * @param count The size of the subspan.
         * @return The subspan.
         */
        ByteSpan subspan(size_t offset, size_t count) const
        {
            if (offset > size_)
                throw std::out_of_range("Offset out of range");

            if (offset + count > size_)
                throw std::out_of_range("Count out of range");

            return ByteSpan(data_ + offset, count);
        }

        /**
         * @brief Gets a subspan of this ByteSpan from the specified offset to the end.
         * @param offset The offset of the subspan.
         * @return The subspan.
         */
        ByteSpan subspan(size_t offset) const
        {
            if (offset > size_)
                throw std::out_of_range("Offset out of range");

            return ByteSpan(data_ + offset, size_ - offset);
        }

        /**
         * @brief Creates a new ByteSpan that is a slice of this ByteSpan.
         * @param start The start index.
         * @param length The length of the slice.
         * @return A new ByteSpan that is a slice of this ByteSpan.
         */
        ByteSpan Slice(size_t start, size_t length) const
        {
            if (start + length > size_)
                throw std::out_of_range("Slice out of range");
            return ByteSpan(data_ + start, length);
        }

        /**
         * @brief Converts the ByteSpan to a hexadecimal string.
         * @return The hexadecimal string representation of the ByteSpan.
         */
        std::string ToHexString() const;

        /**
         * @brief Checks if this ByteSpan is equal to another ByteSpan.
         * @param other The other ByteSpan.
         * @return True if the ByteSpans are equal, false otherwise.
         */
        bool operator==(const ByteSpan& other) const
        {
            if (size_ != other.size_)
                return false;
            return std::memcmp(data_, other.data_, size_) == 0;
        }

        /**
         * @brief Checks if this ByteSpan is not equal to another ByteSpan.
         * @param other The other ByteSpan.
         * @return True if the ByteSpans are not equal, false otherwise.
         */
        bool operator!=(const ByteSpan& other) const
        {
            return !(*this == other);
        }

    private:
        const uint8_t* data_;
        size_t size_;
    };
}
