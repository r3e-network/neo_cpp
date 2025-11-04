/**
 * @file memory_stream.h
 * @brief Stream processing
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/byte_vector.h>

#include <cstdint>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <vector>

namespace neo::io
{
/**
 * @brief A stream buffer for MemoryStream.
 */
class MemoryStreamBuf : public std::streambuf
{
   public:
    MemoryStreamBuf(std::vector<uint8_t>& buffer) : buffer_(buffer)
    {
        // Set up the buffer for both reading and writing
        Reset();
    }

    void Reset()
    {
        // Set up for writing
        char* begin = reinterpret_cast<char*>(buffer_.data());
        char* end = begin + buffer_.size();
        setp(begin, end);

        // Set up for reading
        setg(begin, begin, end);
    }

   protected:
    int_type overflow(int_type ch) override
    {
        if (ch != EOF)
        {
            buffer_.push_back(static_cast<uint8_t>(ch));
            Reset();
            // Move to the end for writing
            setp(reinterpret_cast<char*>(buffer_.data()) + buffer_.size() - 1,
                 reinterpret_cast<char*>(buffer_.data()) + buffer_.size());
        }
        return ch;
    }

    std::streamsize xsputn(const char* s, std::streamsize n) override
    {
        const size_t old_size = buffer_.size();
        buffer_.resize(old_size + static_cast<size_t>(n));
        std::memcpy(buffer_.data() + old_size, s, static_cast<size_t>(n));
        Reset();
        return n;
    }

   private:
    std::vector<uint8_t>& buffer_;
};

/**
 * @brief Indicates the reference point used to obtain a new position within a stream.
 */
enum class SeekOrigin
{
    Begin,
    Current,
    End
};

/**
 * @brief A stream that operates on memory.
 */
class MemoryStream : public std::iostream
{
   public:
    /**
     * @brief Constructs an empty memory stream.
     */
    MemoryStream() : std::iostream(&streamBuf_), data_(), streamBuf_(data_), position_(0) {}

    /**
     * @brief Constructs a memory stream from a byte vector.
     * @param data The initial data.
     */
    explicit MemoryStream(const ByteVector& data)
        : std::iostream(&streamBuf_), data_(data.Data(), data.Data() + data.Size()), streamBuf_(data_), position_(0)
    {
        streamBuf_.Reset();
    }

    /**
     * @brief Constructs a memory stream from a byte array.
     * @param data The initial data.
     * @param size The size of the data.
     */
    MemoryStream(const uint8_t* data, size_t size)
        : std::iostream(&streamBuf_), data_(data, data + size), streamBuf_(data_), position_(0)
    {
        streamBuf_.Reset();
    }

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
     * @brief Seeks to a specific position in the stream.
     * @param position The absolute position to seek to.
     */
    void Seek(size_t position) { SetPosition(position); }

    /**
     * @brief Seeks to a specific position using the provided origin.
     * @param offset The offset relative to the origin.
     * @param origin The reference point used to calculate the new position.
     */
    void Seek(int64_t offset, SeekOrigin origin);

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
    MemoryStreamBuf streamBuf_;
    size_t position_;
};

}  // namespace neo::io
