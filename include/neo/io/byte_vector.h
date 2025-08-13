/**
 * @file byte_vector.h
 * @brief Byte Vector
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/byte_span.h>

#include <cstdint>
#include <string>
#include <vector>
#if __cplusplus >= 202002L
#include <span>
#endif

namespace neo::io
{
/**
 * @brief A mutable byte array with efficient operations.
 */
class ByteVector
{
   public:
    using iterator = std::vector<uint8_t>::iterator;
    using const_iterator = std::vector<uint8_t>::const_iterator;

    /**
     * @brief Constructs an empty ByteVector.
     */
    ByteVector() = default;

    /**
     * @brief Constructs a ByteVector with the specified size.
     * @param size The size of the ByteVector.
     */
    explicit ByteVector(size_t size) : data_(size) {}

    /**
     * @brief Constructs a ByteVector from a ByteSpan.
     * @param data The ByteSpan.
     */
    explicit ByteVector(const ByteSpan& data) : data_(data.Data(), data.Data() + data.Size()) {}

    /**
     * @brief Constructs a ByteVector from a pointer and size.
     * @param data Pointer to the data.
     * @param size Size of the data.
     */
    ByteVector(const uint8_t* data, size_t size) : data_(data, data + size) {}

#if __cplusplus >= 202002L
    /**
     * @brief Constructs a ByteVector from a span.
     * @param data The span.
     */
    explicit ByteVector(std::span<const uint8_t> data) : data_(data.begin(), data.end()) {}
#endif

    /**
     * @brief Constructs a ByteVector from a vector.
     * @param data The vector.
     */
    template <typename Allocator>
    explicit ByteVector(const std::vector<uint8_t, Allocator>& data) : data_(data)
    {
    }

    /**
     * @brief Constructs a ByteVector from an initializer list.
     * @param data The initializer list.
     */
    ByteVector(std::initializer_list<uint8_t> data) : data_(data) {}

    /**
     * @brief Gets the size of the ByteVector.
     * @return The size of the ByteVector.
     */
    size_t Size() const noexcept { return data_.size(); }

    /**
     * @brief Gets the size of the ByteVector (STL compatibility).
     * @return The size of the ByteVector.
     */
    size_t size() const noexcept { return data_.size(); }

    /**
     * @brief Checks if the ByteVector is empty.
     * @return True if the ByteVector is empty, false otherwise.
     */
    bool IsEmpty() const noexcept { return data_.empty(); }

    /**
     * @brief Checks if the ByteVector is empty (STL compatibility).
     * @return True if the ByteVector is empty, false otherwise.
     */
    bool empty() const noexcept { return data_.empty(); }

    /**
     * @brief Gets a pointer to the data.
     * @return Pointer to the data.
     */
    uint8_t* Data() noexcept { return data_.data(); }

    /**
     * @brief Gets a const pointer to the data.
     * @return Const pointer to the data.
     */
    const uint8_t* Data() const noexcept { return data_.data(); }

    /**
     * @brief Gets a reference to the byte at the specified index.
     * @param index The index.
     * @return Reference to the byte at the specified index.
     */
    uint8_t& operator[](size_t index) { return data_[index]; }

    /**
     * @brief Gets the byte at the specified index.
     * @param index The index.
     * @return The byte at the specified index.
     */
    uint8_t operator[](size_t index) const { return data_[index]; }

    /**
     * @brief Resizes the ByteVector.
     * @param size The new size.
     */
    void Resize(size_t size) { data_.resize(size); }

    /**
     * @brief Reserves capacity for the ByteVector.
     * @param capacity The capacity to reserve.
     */
    void Reserve(size_t capacity) { data_.reserve(capacity); }

    /**
     * @brief Appends data to the ByteVector.
     * @param data The data to append.
     */
    void Append(const ByteSpan& data) { data_.insert(data_.end(), data.Data(), data.Data() + data.Size()); }

    /**
     * @brief Pushes a byte to the end of the ByteVector.
     * @param value The byte to push.
     */
    void Push(uint8_t value) { data_.push_back(value); }

    /**
     * @brief Clears the ByteVector.
     */
    void Clear() { data_.clear(); }

    /**
     * @brief Inserts data at the specified position.
     * @param pos Iterator to the position where to insert.
     * @param data Pointer to the data to insert.
     * @param size Size of the data to insert.
     * @return Iterator to the first inserted element.
     */
    auto insert(iterator pos, const uint8_t* data, size_t size) { return data_.insert(pos, data, data + size); }

    /**
     * @brief Inserts data at the specified position.
     * @param pos Iterator to the position where to insert.
     * @param first Iterator to the beginning of the data to insert.
     * @param last Iterator to the end of the data to insert.
     * @return Iterator to the first inserted element.
     */
    template <typename InputIt>
    auto insert(iterator pos, InputIt first, InputIt last)
    {
        return data_.insert(pos, first, last);
    }

    /**
     * @brief Clears the ByteVector (STL compatibility).
     */
    void clear() { data_.clear(); }

    /**
     * @brief Gets an iterator to the beginning of the ByteVector.
     * @return Iterator to the beginning.
     */
    auto begin() noexcept { return data_.begin(); }

    /**
     * @brief Gets an iterator to the end of the ByteVector.
     * @return Iterator to the end.
     */
    auto end() noexcept { return data_.end(); }

    /**
     * @brief Gets a const iterator to the beginning of the ByteVector.
     * @return Const iterator to the beginning.
     */
    auto begin() const noexcept { return data_.begin(); }

    /**
     * @brief Gets a const iterator to the end of the ByteVector.
     * @return Const iterator to the end.
     */
    auto end() const noexcept { return data_.end(); }

    /**
     * @brief Converts the ByteVector to a ByteSpan.
     * @return A ByteSpan view of the ByteVector.
     */
    ByteSpan AsSpan() const { return ByteSpan(data_.data(), data_.size()); }

    /**
     * @brief Converts the ByteVector to a hexadecimal string.
     * @return The hexadecimal string representation of the ByteVector.
     */
    std::string ToHexString() const { return AsSpan().ToHexString(); }

    /**
     * @brief Checks if this ByteVector is equal to another ByteVector.
     * @param other The other ByteVector.
     * @return True if the ByteVectors are equal, false otherwise.
     */
    bool operator==(const ByteVector& other) const { return data_ == other.data_; }

    /**
     * @brief Checks if this ByteVector is equal to a std::vector<uint8_t>.
     * @param other The other vector.
     * @return True if the vectors are equal, false otherwise.
     */
    bool operator==(const std::vector<uint8_t>& other) const { return data_ == other; }

    /**
     * @brief Checks if this ByteVector is not equal to another ByteVector.
     * @param other The other ByteVector.
     * @return True if the ByteVectors are not equal, false otherwise.
     */
    bool operator!=(const ByteVector& other) const { return data_ != other.data_; }

    /**
     * @brief Checks if this ByteVector is not equal to a std::vector<uint8_t>.
     * @param other The other vector.
     * @return True if the vectors are not equal, false otherwise.
     */
    bool operator!=(const std::vector<uint8_t>& other) const { return data_ != other; }

    /**
     * @brief Gets the data as a const reference to std::vector.
     * @return Const reference to the underlying vector.
     */
    const std::vector<uint8_t>& GetVector() const { return data_; }

    /**
     * @brief Implicit conversion to std::vector<uint8_t>.
     * @return Copy of the underlying vector.
     */
    operator std::vector<uint8_t>() const { return data_; }

    /**
     * @brief Constructor from std::vector<uint8_t>.
     * @param vec The vector to copy from.
     */
    ByteVector(const std::vector<uint8_t>& vec) : data_(vec) {}

    /**
     * @brief Assignment from std::vector<uint8_t>.
     * @param vec The vector to assign from.
     * @return Reference to this ByteVector.
     */
    ByteVector& operator=(const std::vector<uint8_t>& vec)
    {
        data_ = vec;
        return *this;
    }

    /**
     * @brief Parses a hexadecimal string into a ByteVector.
     * @param hex The hexadecimal string.
     * @return The parsed ByteVector.
     */
    static ByteVector Parse(const std::string& hex);

    /**
     * @brief Concatenates two ByteVectors.
     * @param a The first ByteVector.
     * @param b The second ByteVector.
     * @return The concatenated ByteVector.
     */
    static ByteVector Concat(const ByteSpan& a, const ByteSpan& b);

    /**
     * @brief Creates a ByteVector from a uint16_t value.
     * @param value The uint16_t value.
     * @return The ByteVector.
     */
    static ByteVector FromUInt16(uint16_t value);

    /**
     * @brief Creates a ByteVector from a uint32_t value.
     * @param value The uint32_t value.
     * @return The ByteVector.
     */
    static ByteVector FromUInt32(uint32_t value);

    /**
     * @brief Converts a ByteSpan to a hex string.
     * @param span The ByteSpan to convert.
     * @return The hex string.
     */
    static std::string ToHexString(const ByteSpan& span);

    /**
     * @brief Creates a ByteVector from a hex string.
     * @param hex The hex string.
     * @return The ByteVector.
     */
    static ByteVector FromHexString(const std::string& hex);

    /**
     * @brief Parses a hex string into a ByteVector (alias for FromHexString).
     * @param hex The hex string.
     * @return The ByteVector.
     */
    static ByteVector ParseHex(const std::string& hex) { return FromHexString(hex); }

    /**
     * @brief Converts the ByteVector to a base64 string.
     * @return The base64 string representation.
     */
    std::string ToBase64String() const;

    /**
     * @brief Creates a ByteVector from a base64 string.
     * @param base64 The base64 string.
     * @return The ByteVector.
     */
    static ByteVector FromBase64String(const std::string& base64);

    /**
     * @brief Checks if this ByteVector is less than another ByteVector.
     * @param other The other ByteVector.
     * @return True if this ByteVector is less than the other ByteVector, false otherwise.
     */
    bool operator<(const ByteVector& other) const { return data_ < other.data_; }

    /**
     * @brief Checks if this ByteVector is greater than another ByteVector.
     * @param other The other ByteVector.
     * @return True if this ByteVector is greater than the other ByteVector, false otherwise.
     */
    bool operator>(const ByteVector& other) const { return data_ > other.data_; }

    /**
     * @brief Gets the variable-length size of the vector.
     * @return The variable-length size.
     */
    size_t GetVarSize() const;

   private:
    std::vector<uint8_t> data_;
};

/**
 * @brief Checks if a std::vector<uint8_t> is equal to a ByteVector.
 * @param lhs The std::vector<uint8_t>.
 * @param rhs The ByteVector.
 * @return True if the vectors are equal, false otherwise.
 */
inline bool operator==(const std::vector<uint8_t>& lhs, const ByteVector& rhs) { return rhs == lhs; }

/**
 * @brief Checks if a std::vector<uint8_t> is not equal to a ByteVector.
 * @param lhs The std::vector<uint8_t>.
 * @param rhs The ByteVector.
 * @return True if the vectors are not equal, false otherwise.
 */
inline bool operator!=(const std::vector<uint8_t>& lhs, const ByteVector& rhs) { return rhs != lhs; }
}  // namespace neo::io

// Add hash function for ByteVector to be used with std::unordered_map
namespace std
{
template <>
struct hash<neo::io::ByteVector>
{
    size_t operator()(const neo::io::ByteVector& value) const noexcept
    {
        // Use the first 8 bytes as a hash, or fewer if the vector is smaller
        size_t result = 0;
        size_t size = std::min(value.Size(), sizeof(size_t));

        if (size > 0)
        {
            const uint8_t* data = value.Data();
            for (size_t i = 0; i < size; i++)
            {
                result = (result << 8) | data[i];
            }
        }

        return result;
    }
};
}  // namespace std
