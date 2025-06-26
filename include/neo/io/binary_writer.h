#pragma once

#include <neo/io/byte_span.h>
#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/io/fixed8.h>
#include <ostream>
#include <string>
#include <cstdint>
#include <vector>

namespace neo::io
{
    class ISerializable;

    /**
     * @brief Writes primitive types and complex objects to a binary stream.
     */
    class BinaryWriter
    {
    public:
        /**
         * @brief Constructs a BinaryWriter that writes to the specified stream.
         * @param stream The stream to write to.
         */
        explicit BinaryWriter(std::ostream& stream);

        /**
         * @brief Constructs a BinaryWriter that writes to the specified ByteVector.
         * @param buffer The ByteVector to write to.
         */
        explicit BinaryWriter(ByteVector& buffer);

        /**
         * @brief Writes a boolean value to the stream.
         * @param value The value to write.
         */
        void Write(bool value);

        /**
         * @brief Writes a boolean value to the stream (alias for Write(bool)).
         * @param value The value to write.
         */
        void WriteBoolean(bool value);

        /**
         * @brief Writes a boolean value to the stream (alias for Write(bool)).
         * @param value The value to write.
         */
        void WriteBool(bool value);

        /**
         * @brief Writes an 8-bit unsigned integer to the stream.
         * @param value The value to write.
         */
        void Write(uint8_t value);

        /**
         * @brief Writes a byte to the stream (alias for Write(uint8_t)).
         * @param value The value to write.
         */
        void WriteByte(uint8_t value);

        /**
         * @brief Writes a 16-bit unsigned integer to the stream.
         * @param value The value to write.
         */
        void Write(uint16_t value);

        /**
         * @brief Writes a 16-bit unsigned integer to the stream (alias for Write(uint16_t)).
         * @param value The value to write.
         */
        void WriteUInt16(uint16_t value);

        /**
         * @brief Writes a 32-bit unsigned integer to the stream.
         * @param value The value to write.
         */
        void Write(uint32_t value);

        /**
         * @brief Writes a 64-bit unsigned integer to the stream.
         * @param value The value to write.
         */
        void Write(uint64_t value);

        /**
         * @brief Writes a 64-bit unsigned integer to the stream (alias for Write(uint64_t)).
         * @param value The value to write.
         */
        void WriteUInt64(uint64_t value);

        /**
         * @brief Writes an 8-bit signed integer to the stream.
         * @param value The value to write.
         */
        void Write(int8_t value);

        /**
         * @brief Writes a 16-bit signed integer to the stream.
         * @param value The value to write.
         */
        void Write(int16_t value);

        /**
         * @brief Writes a 32-bit signed integer to the stream.
         * @param value The value to write.
         */
        void Write(int32_t value);

        /**
         * @brief Writes a 64-bit signed integer to the stream.
         * @param value The value to write.
         */
        void Write(int64_t value);

        /**
         * @brief Writes a 64-bit signed integer to the stream (alias for Write(int64_t)).
         * @param value The value to write.
         */
        void WriteInt64(int64_t value);

        /**
         * @brief Writes a byte span to the stream.
         * @param value The value to write.
         */
        void Write(const ByteSpan& value);

        /**
         * @brief Writes a string to the stream.
         * @param value The value to write.
         */
        void Write(const std::string& value);

        /**
         * @brief Writes a UInt160 to the stream.
         * @param value The value to write.
         */
        void Write(const UInt160& value);

        /**
         * @brief Writes a UInt256 to the stream.
         * @param value The value to write.
         */
        void Write(const UInt256& value);

        /**
         * @brief Writes a Fixed8 to the stream.
         * @param value The value to write.
         */
        void Write(const Fixed8& value);

        /**
         * @brief Writes a serializable object to the stream.
         * @param value The value to write.
         */
        void Write(const ISerializable& value);

        /**
         * @brief Writes a variable-length integer to the stream.
         * @param value The value to write.
         */
        void WriteVarInt(int64_t value);

        /**
         * @brief Writes a variable-length byte array to the stream.
         * @param value The value to write.
         */
        void WriteVarBytes(const ByteSpan& value);

        /**
         * @brief Writes a variable-length byte array to the stream.
         * @param value The value to write.
         */
        void WriteVarBytes(const std::vector<uint8_t>& value);

        /**
         * @brief Writes a string to the stream.
         * @param value The value to write.
         */
        void WriteString(const std::string& value);

        /**
         * @brief Writes a variable-length string to the stream.
         * @param value The value to write.
         */
        void WriteVarString(const std::string& value);

        /**
         * @brief Writes a fixed-length string to the stream.
         * @param value The string to write.
         * @param length The length of the string.
         */
        void WriteFixedString(const std::string& value, size_t length);

        /**
         * @brief Writes a vector of serializable objects to the stream.
         * @tparam T The type of the objects in the vector.
         * @param value The value to write.
         */
        template <typename T>
        void WriteVector(const std::vector<T>& value)
        {
            static_assert(std::is_base_of<ISerializable, T>::value, "T must derive from ISerializable");

            WriteVarInt(value.size());
            for (const auto& item : value)
            {
                Write(item);
            }
        }

        /**
         * @brief Writes a variable-length array to the stream.
         * @tparam T The type of the elements in the array.
         * @param value The value to write.
         */
        template <typename T>
        void WriteVarArray(const std::vector<T>& value)
        {
            WriteVarInt(value.size());
            for (const auto& item : value)
            {
                Write(item);
            }
        }

        /**
         * @brief Writes raw bytes to the stream.
         * @param data Pointer to the data to write.
         * @param size Number of bytes to write.
         */
        void WriteBytes(const uint8_t* data, size_t size);

        /**
         * @brief Writes a ByteVector to the stream.
         * @param data The ByteVector to write.
         */
        void WriteBytes(const ByteVector& data);

    private:
        std::ostream* stream_;
        ByteVector* buffer_;
        bool owns_stream_;

        /**
         * @brief Helper method to write raw bytes to either stream or buffer.
         * @param data Pointer to the data to write.
         * @param size Number of bytes to write.
         */
        void WriteRawBytes(const uint8_t* data, size_t size);
    };
}
