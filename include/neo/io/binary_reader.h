#pragma once

#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/io/fixed8.h>
#include <istream>
#include <string>
#include <cstdint>
#include <vector>
#include <memory>

namespace neo::io
{
    class ISerializable;

    /**
     * @brief Reads primitive types and complex objects from a binary stream.
     */
    class BinaryReader
    {
    public:
        /**
         * @brief Constructs a BinaryReader that reads from the specified stream.
         * @param stream The stream to read from.
         */
        explicit BinaryReader(std::istream& stream);

        /**
         * @brief Reads a boolean value from the stream.
         * @return The value read.
         */
        bool ReadBool();

        /**
         * @brief Reads a boolean value from the stream (alias for ReadBool).
         * @return The value read.
         */
        bool ReadBoolean();

        /**
         * @brief Reads an 8-bit unsigned integer from the stream.
         * @return The value read.
         */
        uint8_t ReadUInt8();

        /**
         * @brief Reads a single byte from the stream.
         * @return The byte read.
         */
        uint8_t ReadByte();

        /**
         * @brief Peeks at the next 8-bit unsigned integer from the stream without advancing the position.
         * @return The value peeked.
         */
        uint8_t PeekUInt8();

        /**
         * @brief Reads a 16-bit unsigned integer from the stream.
         * @return The value read.
         */
        uint16_t ReadUInt16();

        /**
         * @brief Reads a 32-bit unsigned integer from the stream.
         * @return The value read.
         */
        uint32_t ReadUInt32();

        /**
         * @brief Reads a 64-bit unsigned integer from the stream.
         * @return The value read.
         */
        uint64_t ReadUInt64();

        /**
         * @brief Reads an 8-bit signed integer from the stream.
         * @return The value read.
         */
        int8_t ReadInt8();

        /**
         * @brief Reads a 16-bit signed integer from the stream.
         * @return The value read.
         */
        int16_t ReadInt16();

        /**
         * @brief Reads a 32-bit signed integer from the stream.
         * @return The value read.
         */
        int32_t ReadInt32();

        /**
         * @brief Reads a 64-bit signed integer from the stream.
         * @return The value read.
         */
        int64_t ReadInt64();

        /**
         * @brief Reads a byte array from the stream.
         * @param count The number of bytes to read.
         * @return The value read.
         */
        ByteVector ReadBytes(size_t count);

        /**
         * @brief Reads a UInt160 from the stream.
         * @return The value read.
         */
        UInt160 ReadUInt160();

        /**
         * @brief Reads a UInt256 from the stream.
         * @return The value read.
         */
        UInt256 ReadUInt256();

        /**
         * @brief Reads a Fixed8 from the stream.
         * @return The value read.
         */
        Fixed8 ReadFixed8();

        /**
         * @brief Reads a variable-length integer from the stream.
         * @return The value read.
         */
        int64_t ReadVarInt();

        /**
         * @brief Reads a variable-length integer from the stream with max value check.
         * @param max The maximum allowed value.
         * @return The value read.
         */
        int64_t ReadVarInt(int64_t max);

        /**
         * @brief Reads a variable-length byte array from the stream.
         * @return The value read.
         */
        ByteVector ReadVarBytes();

        /**
         * @brief Reads a variable-length byte array from the stream with max size check.
         * @param maxSize The maximum allowed size.
         * @return The value read.
         */
        ByteVector ReadVarBytes(size_t maxSize);

        /**
         * @brief Reads a string from the stream.
         * @return The value read.
         */
        std::string ReadString();

        /**
         * @brief Reads a variable-length string from the stream.
         * @return The value read.
         */
        std::string ReadVarString();

        /**
         * @brief Reads a variable-length string from the stream with max length check.
         * @param maxLength The maximum allowed length.
         * @return The value read.
         */
        std::string ReadVarString(size_t maxLength);

        /**
         * @brief Reads a fixed-length string from the stream.
         * @param length The length of the string.
         * @return The value read.
         */
        std::string ReadFixedString(size_t length);

        /**
         * @brief Reads a serializable object from the stream.
         * @tparam T The type of the object to read.
         * @return The value read.
         */
        template <typename T>
        T ReadSerializable()
        {
            static_assert(std::is_base_of<ISerializable, T>::value, "T must derive from ISerializable");
            static_assert(std::is_default_constructible<T>::value, "T must be default constructible");

            T obj;
            obj.Deserialize(*this);
            return obj;
        }

        /**
         * @brief Reads a vector of serializable objects from the stream.
         * @tparam T The type of the objects in the vector.
         * @return The value read.
         */
        template <typename T>
        std::vector<T> ReadVector()
        {
            static_assert(std::is_base_of<ISerializable, T>::value, "T must derive from ISerializable");
            static_assert(std::is_default_constructible<T>::value, "T must be default constructible");

            int64_t count = ReadVarInt();
            if (count < 0 || count > std::numeric_limits<size_t>::max())
                throw std::out_of_range("Invalid vector size");

            std::vector<T> result;
            result.reserve(static_cast<size_t>(count));

            for (int64_t i = 0; i < count; i++)
            {
                result.push_back(ReadSerializable<T>());
            }

            return result;
        }

        /**
         * @brief Reads raw bytes from the stream.
         * @param data Pointer to the buffer to read into.
         * @param size Number of bytes to read.
         */
        void ReadBytes(uint8_t* data, size_t size);

    private:
        std::istream& stream_;
    };
}
