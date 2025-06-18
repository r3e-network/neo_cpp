#pragma once

#include <neo/io/byte_vector.h>
#include <vector>
#include <cstdint>

namespace neo::io
{
    /**
     * @brief A stream that operates on memory.
     */
    class MemoryStream
    {
    public:
        /**
         * @brief Constructs an empty memory stream.
         */
        MemoryStream();

        /**
         * @brief Constructs a memory stream from a byte vector.
         * @param data The initial data.
         */
        explicit MemoryStream(const ByteVector& data);

        /**
         * @brief Constructs a memory stream from a byte array.
         * @param data The initial data.
         * @param size The size of the data.
         */
        MemoryStream(const uint8_t* data, size_t size);

        /**
         * @brief Destructor.
         */
        ~MemoryStream() = default;

        /**
         * @brief Gets the current position in the stream.
         * @return The current position.
         */
        size_t GetPosition() const { return position_; }

        /**
         * @brief Sets the current position in the stream.
         * @param position The new position.
         */
        void SetPosition(size_t position);

        /**
         * @brief Gets the length of the stream.
         * @return The length of the stream.
         */
        size_t GetLength() const { return data_.size(); }

        /**
         * @brief Reads data from the stream.
         * @param buffer The buffer to read into.
         * @param count The number of bytes to read.
         * @return The number of bytes actually read.
         */
        size_t Read(uint8_t* buffer, size_t count);

        /**
         * @brief Writes data to the stream.
         * @param buffer The buffer to write from.
         * @param count The number of bytes to write.
         */
        void Write(const uint8_t* buffer, size_t count);

        /**
         * @brief Gets the underlying data.
         * @return The data.
         */
        const std::vector<uint8_t>& GetData() const { return data_; }

        /**
         * @brief Gets the underlying data as a ByteVector.
         * @return The data as ByteVector.
         */
        ByteVector ToByteVector() const;

    private:
        std::vector<uint8_t> data_;
        size_t position_;
    };

} // namespace neo::io 