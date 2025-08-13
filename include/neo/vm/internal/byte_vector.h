/**
 * @file byte_vector.h
 * @brief Byte Vector
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/vm/internal/byte_span.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace neo::vm::internal
{
/**
 * @brief A mutable byte array with efficient operations.
 */
class ByteVector
{
   public:
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
     * @brief Checks if the ByteVector is empty.
     * @return True if the ByteVector is empty, false otherwise.
     */
    bool IsEmpty() const noexcept { return data_.empty(); }

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
    uint8_t& operator[](size_t index)
    {
        if (index >= data_.size()) throw std::out_of_range("Index out of range");
        return data_[index];
    }

    /**
     * @brief Gets the byte at the specified index.
     * @param index The index.
     * @return The byte at the specified index.
     */
    uint8_t operator[](size_t index) const
    {
        if (index >= data_.size()) throw std::out_of_range("Index out of range");
        return data_[index];
    }

    /**
     * @brief Compares this ByteVector with another ByteVector.
     * @param other The other ByteVector.
     * @return True if the ByteVectors are equal, false otherwise.
     */
    bool operator==(const ByteVector& other) const noexcept { return data_ == other.data_; }

    /**
     * @brief Compares this ByteVector with another ByteVector.
     * @param other The other ByteVector.
     * @return True if the ByteVectors are not equal, false otherwise.
     */
    bool operator!=(const ByteVector& other) const noexcept { return data_ != other.data_; }

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
     * @brief Gets a ByteSpan view of the ByteVector.
     * @return ByteSpan view of the ByteVector.
     */
    ByteSpan AsSpan() const noexcept { return ByteSpan(data_); }

    /**
     * @brief Converts the ByteVector to a hexadecimal string.
     * @return The hexadecimal string representation of the ByteVector.
     */
    std::string ToHexString() const { return ByteSpan(data_).ToHexString(); }

    /**
     * @brief Parses a hexadecimal string into a ByteVector.
     * @param hex The hexadecimal string.
     * @return The parsed ByteVector.
     */
    static ByteVector Parse(const std::string& hex)
    {
        if (hex.empty()) return ByteVector();

        // Remove '0x' prefix if present
        std::string hexStr = hex;
        if (hexStr.size() >= 2 && hexStr[0] == '0' && (hexStr[1] == 'x' || hexStr[1] == 'X')) hexStr = hexStr.substr(2);

        // Ensure even length
        if (hexStr.length() % 2 != 0) throw std::invalid_argument("Invalid hex string length");

        // Validate hex characters
        for (char c : hexStr)
        {
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
                throw std::invalid_argument("Invalid hex character");
        }

        ByteVector result(hexStr.length() / 2);

        for (size_t i = 0; i < hexStr.length(); i += 2)
        {
            std::string byteStr = hexStr.substr(i, 2);
            try
            {
                result[i / 2] = static_cast<uint8_t>(std::stoi(byteStr, nullptr, 16));
            }
            catch (const std::exception&)
            {
                throw std::invalid_argument("Invalid hex string");
            }
        }

        return result;
    }

    /**
     * @brief Concatenates two ByteVectors.
     * @param a The first ByteVector.
     * @param b The second ByteVector.
     * @return The concatenated ByteVector.
     */
    static ByteVector Concat(const ByteSpan& a, const ByteSpan& b)
    {
        ByteVector result(a.Size() + b.Size());
        std::memcpy(result.Data(), a.Data(), a.Size());
        std::memcpy(result.Data() + a.Size(), b.Data(), b.Size());
        return result;
    }

   private:
    std::vector<uint8_t> data_;
};
}  // namespace neo::vm::internal
