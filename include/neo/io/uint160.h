/**
 * @file uint160.h
 * @brief 160-bit unsigned integer type
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/byte_span.h>
#include <neo/io/byte_vector.h>
#include <neo/io/iserializable.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <functional>
#include <stdexcept>
#include <string>

namespace neo::io
{
/**
 * @brief Represents a 160-bit unsigned integer.
 */
class UInt160 : public ISerializable
{
   public:
    /**
     * @brief The size of the UInt160 in bytes.
     */
    static constexpr size_t Size = 20;
    static constexpr size_t SIZE = 20;  // Backward compatibility
    using value_type = std::array<uint8_t, Size>;

    /**
     * @brief Constructs a UInt160 initialized to zero.
     */
    UInt160() : data_() {}

    /**
     * @brief Constructs a UInt160 from a ByteSpan.
     * @param data The ByteSpan.
     * @throws std::invalid_argument if the ByteSpan size is not equal to UInt160::Size.
     */
    explicit UInt160(const ByteSpan& data)
    {
        if (data.Size() != Size)
        {
            throw std::invalid_argument("Invalid UInt160 size: expected " + std::to_string(Size) + " but got " +
                                        std::to_string(data.Size()));
        }
        std::memcpy(data_.data(), data.Data(), Size);
    }

    /**
     * @brief Constructs a UInt160 from a byte array.
     * @param data The byte array.
     */
    explicit UInt160(const value_type& data) : data_(data) {}

    /**
     * @brief Constructs a UInt160 from a raw byte array.
     * @param data The raw byte array.
     */
    explicit UInt160(const uint8_t* data) { std::memcpy(data_.data(), data, Size); }

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
     * @brief Converts the UInt160 to a ByteSpan.
     * @return A ByteSpan view of the UInt160.
     */
    ByteSpan AsSpan() const { return ByteSpan(data_.data(), Size); }

    /**
     * @brief Converts the UInt160 to a ByteVector.
     * @return A ByteVector copy of the UInt160.
     */
    ByteVector ToArray() const { return ByteVector(AsSpan()); }

    /**
     * @brief Converts the UInt160 to a hexadecimal string.
     * @return The hexadecimal string representation of the UInt160.
     */
    std::string ToHexString() const { return AsSpan().ToHexString(); }

    /**
     * @brief Converts the UInt160 to a string.
     * @return The string representation of the UInt160.
     */
    std::string ToString() const { return ToHexString(); }

    /**
     * @brief Checks if this UInt160 is equal to another UInt160.
     * @param other The other UInt160.
     * @return True if the UInt160s are equal, false otherwise.
     */
    bool operator==(const UInt160& other) const { return data_ == other.data_; }

    /**
     * @brief Checks if this UInt160 is not equal to another UInt160.
     * @param other The other UInt160.
     * @return True if the UInt160s are not equal, false otherwise.
     */
    bool operator!=(const UInt160& other) const { return data_ != other.data_; }

    /**
     * @brief Checks if this UInt160 is less than another UInt160.
     * @param other The other UInt160.
     * @return True if this UInt160 is less than the other UInt160, false otherwise.
     */
    bool operator<(const UInt160& other) const { return data_ < other.data_; }

    /**
     * @brief Checks if this UInt160 is greater than another UInt160.
     * @param other The other UInt160.
     * @return True if this UInt160 is greater than the other UInt160, false otherwise.
     */
    bool operator>(const UInt160& other) const { return data_ > other.data_; }

    /**
     * @brief Checks if this UInt160 is less than or equal to another UInt160.
     * @param other The other UInt160.
     * @return True if this UInt160 is less than or equal to the other UInt160, false otherwise.
     */
    bool operator<=(const UInt160& other) const { return !(*this > other); }

    /**
     * @brief Checks if this UInt160 is greater than or equal to another UInt160.
     * @param other The other UInt160.
     * @return True if this UInt160 is greater than or equal to the other UInt160, false otherwise.
     */
    bool operator>=(const UInt160& other) const { return !(*this < other); }

    /**
     * @brief Parses a hexadecimal string into a UInt160.
     * @param hex The hexadecimal string.
     * @return The parsed UInt160.
     * @throws std::invalid_argument if the hexadecimal string is invalid.
     */
    static UInt160 Parse(const std::string& hex);

    /**
     * @brief Tries to parse a hexadecimal string into a UInt160.
     * @param hex The hexadecimal string.
     * @param result The parsed UInt160.
     * @return True if the parsing was successful, false otherwise.
     */
    static bool TryParse(const std::string& hex, UInt160& result);

    /**
     * @brief Creates a UInt160 from a hex string.
     * @param hex_string The hex string (with or without 0x prefix).
     * @return The UInt160 value.
     */
    static UInt160 FromString(const std::string& hex_string);

    /**
     * @brief Checks if this UInt160 is zero.
     * @return True if this UInt160 is zero, false otherwise.
     */
    bool IsZero() const;

    /**
     * @brief Gets a UInt160 with all bits set to zero.
     * @return A UInt160 with all bits set to zero.
     */
    static UInt160 Zero() { return UInt160(); }

    /**
     * @brief Creates a UInt160 from a ByteSpan.
     * @param data The ByteSpan containing the data.
     * @return The UInt160 created from the data.
     * @throws std::invalid_argument if the ByteSpan size is not equal to UInt160::Size.
     */
    static UInt160 FromBytes(const ByteSpan& data) { return UInt160(data); }

    /**
     * @brief Creates a UInt160 from a Neo address.
     * @param address The Neo address string.
     * @return The UInt160 script hash.
     * @throws std::invalid_argument if the address is invalid.
     */
    static UInt160 FromAddress(const std::string& address);

    /**
     * @brief Converts the UInt160 to a Neo address.
     * @param version The address version (default is 0x17 for Neo mainnet).
     * @return The Neo address string.
     */
    std::string ToAddress(uint8_t version = 0x17) const;

    /**
     * @brief Serializes the UInt160 to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the UInt160 from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(BinaryReader& reader) override;

    /**
     * @brief Array subscript operator.
     * @param index The index.
     * @return Reference to the byte at the index.
     */
    uint8_t& operator[](size_t index) { return data_[index]; }

    /**
     * @brief Array subscript operator (const).
     * @param index The index.
     * @return Const reference to the byte at the index.
     */
    const uint8_t& operator[](size_t index) const { return data_[index]; }

   private:
    value_type data_;
};
}  // namespace neo::io

// Add hash function for UInt160 to be used with std::unordered_map
namespace std
{
/**
 * @brief Hash function for neo::io::UInt160.
 */
template <>
struct hash<neo::io::UInt160>
{
    /**
     * @brief Calculates the hash of a UInt160.
     * @param value The UInt160.
     * @return The hash.
     */
    size_t operator()(const neo::io::UInt160& value) const noexcept
    {
        // Use the first 8 bytes as a hash
        const uint8_t* data = value.Data();
        size_t result = 0;
        for (size_t i = 0; i < sizeof(size_t) && i < neo::io::UInt160::Size; i++)
        {
            result = (result << 8) | data[i];
        }
        return result;
    }
};
}  // namespace std
