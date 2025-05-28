#pragma once

#include <neo/io/byte_span.h>
#include <neo/io/byte_vector.h>
#include <neo/io/iserializable.h>
#include <array>
#include <string>
#include <cstdint>
#include <stdexcept>

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

        /**
         * @brief Constructs a UInt256 initialized to zero.
         */
        UInt256() : data_() {}

        /**
         * @brief Constructs a UInt256 from a ByteSpan.
         * @param data The ByteSpan.
         * @throws std::invalid_argument if the ByteSpan size is not equal to UInt256::Size.
         */
        explicit UInt256(const ByteSpan& data)
        {
            if (data.Size() != Size)
                throw std::invalid_argument("Invalid UInt256 size");
            std::memcpy(data_.data(), data.Data(), Size);
        }

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
         * @brief Converts the UInt256 to a ByteSpan.
         * @return A ByteSpan view of the UInt256.
         */
        ByteSpan AsSpan() const { return ByteSpan(data_.data(), Size); }

        /**
         * @brief Converts the UInt256 to a ByteVector.
         * @return A ByteVector copy of the UInt256.
         */
        ByteVector ToArray() const { return ByteVector(AsSpan()); }

        /**
         * @brief Converts the UInt256 to a hexadecimal string.
         * @return The hexadecimal string representation of the UInt256.
         */
        std::string ToHexString() const { return AsSpan().ToHexString(); }

        /**
         * @brief Checks if this UInt256 is equal to another UInt256.
         * @param other The other UInt256.
         * @return True if the UInt256s are equal, false otherwise.
         */
        bool operator==(const UInt256& other) const { return data_ == other.data_; }

        /**
         * @brief Checks if this UInt256 is not equal to another UInt256.
         * @param other The other UInt256.
         * @return True if the UInt256s are not equal, false otherwise.
         */
        bool operator!=(const UInt256& other) const { return data_ != other.data_; }

        /**
         * @brief Checks if this UInt256 is less than another UInt256.
         * @param other The other UInt256.
         * @return True if this UInt256 is less than the other UInt256, false otherwise.
         */
        bool operator<(const UInt256& other) const { return data_ < other.data_; }

        /**
         * @brief Checks if this UInt256 is greater than another UInt256.
         * @param other The other UInt256.
         * @return True if this UInt256 is greater than the other UInt256, false otherwise.
         */
        bool operator>(const UInt256& other) const { return data_ > other.data_; }

        /**
         * @brief Converts this UInt256 to a string.
         * @return The string representation of this UInt256.
         */
        std::string ToString() const { return ToHexString(); }

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
        static UInt256 Zero() { return UInt256(); }

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

    private:
        std::array<uint8_t, Size> data_;
    };
}

namespace std
{
    /**
     * @brief Hash function for neo::io::UInt256.
     */
    template<>
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
}
