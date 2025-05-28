#pragma once

#include <neo/io/byte_span.h>
#include <neo/io/byte_vector.h>
#include <string>
#include <cstdint>
#include <memory>

namespace neo::io
{
    /**
     * @brief An immutable byte array with efficient operations.
     */
    class ByteString
    {
    public:
        /**
         * @brief Constructs an empty ByteString.
         */
        ByteString() : data_(std::make_shared<ByteVector>()) {}

        /**
         * @brief Constructs a ByteString from a ByteSpan.
         * @param data The ByteSpan.
         */
        explicit ByteString(const ByteSpan& data) : data_(std::make_shared<ByteVector>(data)) {}

        /**
         * @brief Constructs a ByteString from a ByteVector.
         * @param data The ByteVector.
         */
        explicit ByteString(const ByteVector& data) : data_(std::make_shared<ByteVector>(data)) {}

        /**
         * @brief Constructs a ByteString from an initializer list.
         * @param data The initializer list.
         */
        ByteString(std::initializer_list<uint8_t> data) : data_(std::make_shared<ByteVector>(data)) {}

        /**
         * @brief Gets the size of the ByteString.
         * @return The size of the ByteString.
         */
        size_t Size() const noexcept { return data_->Size(); }

        /**
         * @brief Gets a const pointer to the data.
         * @return Const pointer to the data.
         */
        const uint8_t* Data() const noexcept { return data_->Data(); }

        /**
         * @brief Gets the byte at the specified index.
         * @param index The index.
         * @return The byte at the specified index.
         */
        uint8_t operator[](size_t index) const { return (*data_)[index]; }

        /**
         * @brief Converts the ByteString to a ByteSpan.
         * @return A ByteSpan view of the ByteString.
         */
        ByteSpan AsSpan() const { return data_->AsSpan(); }

        /**
         * @brief Converts the ByteString to a hexadecimal string.
         * @return The hexadecimal string representation of the ByteString.
         */
        std::string ToHexString() const { return data_->ToHexString(); }

        /**
         * @brief Checks if this ByteString is equal to another ByteString.
         * @param other The other ByteString.
         * @return True if the ByteStrings are equal, false otherwise.
         */
        bool operator==(const ByteString& other) const { return *data_ == *other.data_; }

        /**
         * @brief Checks if this ByteString is not equal to another ByteString.
         * @param other The other ByteString.
         * @return True if the ByteStrings are not equal, false otherwise.
         */
        bool operator!=(const ByteString& other) const { return *data_ != *other.data_; }

        /**
         * @brief Parses a hexadecimal string into a ByteString.
         * @param hex The hexadecimal string.
         * @return The parsed ByteString.
         */
        static ByteString Parse(const std::string& hex)
        {
            return ByteString(ByteVector::Parse(hex));
        }

    private:
        std::shared_ptr<ByteVector> data_;
    };
}
