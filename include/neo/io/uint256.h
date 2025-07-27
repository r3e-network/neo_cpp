#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <neo/io/byte_span.h>
#include <neo/io/byte_vector.h>
#include <neo/io/iserializable.h>
#include <stdexcept>
#include <string>

namespace neo::io
{
/**
 * @brief Represents a 256-bit unsigned integer.
 */
class UInt256 : public ISerializable
{
  public:
    /**
     * @brief The size of the UInt256 in bytes.
     */
    static constexpr size_t Size = 32;
    static constexpr size_t SIZE = 32;  // Backward compatibility
    using value_type = std::array<uint8_t, Size>;

    /**
     * @brief Constructs a UInt256 initialized to zero.
     */
    UInt256() : data_() {}

    /**
     * @brief Virtual destructor.
     */
    virtual ~UInt256() = default;

    /**
     * @brief Constructs a UInt256 from a ByteSpan.
     * @param data The ByteSpan.
     * @throws std::invalid_argument if the ByteSpan size is not equal to UInt256::Size.
     */
    explicit UInt256(const ByteSpan& data)
    {
        if (data.Size() != Size)
            throw std::invalid_argument("Invalid UInt256 size");
        memcpy(data_.data(), data.Data(), Size);
    }

    /**
     * @brief Constructs a UInt256 from a byte array.
     * @param data The byte array.
     */
    explicit UInt256(const value_type& data) : data_(data) {}

    /**
     * @brief Constructs a UInt256 from a raw byte array.
     * @param data The raw byte array.
     */
    explicit UInt256(const uint8_t* data)
    {
        memcpy(data_.data(), data, Size);
    }

    /**
     * @brief Gets a pointer to the data.
     * @return Pointer to the data.
     */
    uint8_t* Data() noexcept
    {
        return data_.data();
    }

    /**
     * @brief Gets a const pointer to the data.
     * @return Const pointer to the data.
     */
    const uint8_t* Data() const noexcept
    {
        return data_.data();
    }

    /**
     * @brief Converts the UInt256 to a ByteSpan.
     * @return A ByteSpan view of the UInt256.
     */
    ByteSpan AsSpan() const
    {
        return ByteSpan(data_.data(), Size);
    }

    /**
     * @brief Converts the UInt256 to a ByteVector.
     * @return A ByteVector copy of the UInt256.
     */
    ByteVector ToArray() const
    {
        return ByteVector(data_.data(), Size);
    }

    /**
     * @brief Converts the UInt256 to a hexadecimal string.
     * @return The hexadecimal string representation of the UInt256.
     */
    std::string ToHexString() const
    {
        return AsSpan().ToHexString();
    }

    /**
     * @brief Checks if this UInt256 is equal to another UInt256.
     * @param other The other UInt256.
     * @return True if the UInt256s are equal, false otherwise.
     */
    bool operator==(const UInt256& other) const
    {
        return std::equal(data_.begin(), data_.end(), other.data_.begin());
    }

    /**
     * @brief Checks if this UInt256 is not equal to another UInt256.
     * @param other The other UInt256.
     * @return True if the UInt256s are not equal, false otherwise.
     */
    bool operator!=(const UInt256& other) const
    {
        return !(*this == other);
    }

    /**
     * @brief Checks if this UInt256 is less than another UInt256.
     * @param other The other UInt256.
     * @return True if this UInt256 is less than the other UInt256, false otherwise.
     */
    bool operator<(const UInt256& other) const
    {
        return std::lexicographical_compare(data_.begin(), data_.end(), other.data_.begin(), other.data_.end());
    }

    /**
     * @brief Checks if this UInt256 is greater than another UInt256.
     * @param other The other UInt256.
     * @return True if this UInt256 is greater than the other UInt256, false otherwise.
     */
    bool operator>(const UInt256& other) const
    {
        return other < *this;
    }

    /**
     * @brief Converts this UInt256 to a string.
     * @return The string representation of this UInt256.
     */
    std::string ToString() const
    {
        return ToHexString();
    }

    /**
     * @brief Converts this UInt256 to a string with optional byte order reversal.
     * @param reverse If true, output in little-endian order; if false, big-endian order.
     * @return The string representation of this UInt256.
     */
    std::string ToString(bool reverse) const;

    /**
     * @brief Parses a hexadecimal string into a UInt256.
     * @param hex The hexadecimal string.
     * @return The parsed UInt256.
     * @throws std::invalid_argument if the hexadecimal string is invalid.
     */
    static UInt256 Parse(const std::string& hex);

    /**
     * @brief Tries to parse a hexadecimal string into a UInt256.
     * @param hex The hexadecimal string.
     * @param result The parsed UInt256.
     * @return True if the parsing was successful, false otherwise.
     */
    static bool TryParse(const std::string& hex, UInt256& result);

    /**
     * @brief Gets a UInt256 with all bits set to zero.
     * @return A UInt256 with all bits set to zero.
     */
    static UInt256 Zero()
    {
        return UInt256();
    }

    /**
     * @brief Checks if this UInt256 is zero.
     * @return True if this UInt256 is zero, false otherwise.
     */
    bool IsZero() const;

    /**
     * @brief Serializes the UInt256 to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the UInt256 from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(BinaryReader& reader) override;

    /**
     * @brief Creates a UInt256 from a hex string.
     * @param hex_string The hex string (with or without 0x prefix).
     * @return The UInt256 value.
     */
    static UInt256 FromString(const std::string& hex_string);

    /**
     * @brief Creates a UInt256 from a little-endian hex string.
     * @param hex_string The hex string in little-endian format.
     * @return The UInt256 value.
     */
    static UInt256 FromLittleEndianString(const std::string& hex_string);

    /**
     * @brief Gets the raw data.
     * @return The raw data.
     */
    const value_type& GetData() const
    {
        return data_;
    }

    /**
     * @brief Gets the size in bytes.
     * @return The size in bytes.
     */
    constexpr size_t size() const
    {
        return Size;
    }

    /**
     * @brief Converts to little-endian hex string.
     * @return The little-endian hex string.
     */
    std::string ToLittleEndianString() const;

    /**
     * @brief Array subscript operator.
     * @param index The index.
     * @return Reference to the byte at the index.
     */
    uint8_t& operator[](size_t index)
    {
        return data_[index];
    }

    /**
     * @brief Array subscript operator (const).
     * @param index The index.
     * @return Const reference to the byte at the index.
     */
    const uint8_t& operator[](size_t index) const
    {
        return data_[index];
    }

  private:
    value_type data_;
};
}  // namespace neo::io

namespace std
{
/**
 * @brief Hash function for neo::io::UInt256.
 */
template <>
struct hash<neo::io::UInt256>
{
    /**
     * @brief Calculates the hash of a UInt256.
     * @param value The UInt256.
     * @return The hash.
     */
    size_t operator()(const neo::io::UInt256& value) const noexcept
    {
        // Use the first 8 bytes as a hash
        const uint8_t* data = value.Data();
        size_t result = 0;
        for (size_t i = 0; i < sizeof(size_t); i++)
        {
            result = (result << 8) | data[i];
        }
        return result;
    }
};
}  // namespace std
