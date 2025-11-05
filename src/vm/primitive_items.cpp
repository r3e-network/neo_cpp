/**
 * @file primitive_items.cpp
 * @brief Primitive Items
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/vm/exceptions.h>
#include <neo/vm/primitive_items.h>

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace neo::vm
{
// BooleanItem implementation
BooleanItem::BooleanItem(bool value) : value_(value) {}

StackItemType BooleanItem::GetType() const { return StackItemType::Boolean; }

bool BooleanItem::GetBoolean() const { return value_; }

int64_t BooleanItem::GetInteger() const { return value_ ? 1 : 0; }

io::ByteVector BooleanItem::GetByteArray() const
{
    return io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(&value_), sizeof(value_)));
}

bool BooleanItem::Equals(const StackItem& other) const
{
    if (other.GetType() == StackItemType::Boolean) return value_ == other.GetBoolean();

    if (other.GetType() == StackItemType::Integer) return GetInteger() == other.GetInteger();

    if (other.GetType() == StackItemType::ByteString || other.GetType() == StackItemType::Buffer)
    {
        auto bytes = other.GetByteArray();
        if (bytes.Size() != 1) return false;

        return value_ == (bytes[0] != 0);
    }

    return false;
}

// IntegerItem implementation
IntegerItem::IntegerItem(int64_t value) : value_(value) {}

StackItemType IntegerItem::GetType() const { return StackItemType::Integer; }

bool IntegerItem::GetBoolean() const { return value_ != 0; }

int64_t IntegerItem::GetInteger() const { return value_; }

io::ByteVector IntegerItem::GetByteArray() const
{
    // Convert to little-endian byte array
    uint8_t bytes[sizeof(value_)];
    for (size_t i = 0; i < sizeof(value_); i++) bytes[i] = static_cast<uint8_t>((value_ >> (i * 8)) & 0xFF);

    // Remove trailing zeros
    size_t length = sizeof(value_);
    while (length > 0 && bytes[length - 1] == 0) length--;

    // If the number is negative, we need to keep the sign bit
    if (value_ < 0 && length > 0 && (bytes[length - 1] & 0x80) == 0) length++;

    // If the number is positive and the sign bit is set, we need to add a zero byte
    if (value_ > 0 && length > 0 && (bytes[length - 1] & 0x80) != 0) length++;

    // If the number is zero, return a single zero byte
    if (length == 0) length = 1;

    return io::ByteVector(io::ByteSpan(bytes, length));
}

bool IntegerItem::Equals(const StackItem& other) const
{
    if (other.GetType() == StackItemType::Integer) return value_ == other.GetInteger();

    if (other.GetType() == StackItemType::Boolean) return GetBoolean() == other.GetBoolean();

    if (other.GetType() == StackItemType::ByteString || other.GetType() == StackItemType::Buffer)
    {
        auto bytes = other.GetByteArray();
        if (bytes.Size() > sizeof(value_)) return false;

        int64_t otherValue = 0;
        for (size_t i = 0; i < bytes.Size(); i++) otherValue |= static_cast<int64_t>(bytes[i]) << (i * 8);

        // Sign extension
        if (bytes.Size() > 0 && (bytes[bytes.Size() - 1] & 0x80) != 0) otherValue |= ~((1LL << (bytes.Size() * 8)) - 1);

        return value_ == otherValue;
    }

    return false;
}

// ByteStringItem implementation
ByteStringItem::ByteStringItem(const io::ByteVector& value) : value_(value) {}

ByteStringItem::ByteStringItem(const io::ByteSpan& value) : value_(value) {}

StackItemType ByteStringItem::GetType() const { return StackItemType::ByteString; }

bool ByteStringItem::GetBoolean() const
{
    for (size_t i = 0; i < value_.Size(); i++)
    {
        if (value_.Data()[i] != 0) return true;
    }
    return false;  // Empty or all zeros
}

int64_t ByteStringItem::GetInteger() const
{
    if (value_.Size() > sizeof(int64_t)) throw std::runtime_error("ByteString too large to convert to integer");

    int64_t result = 0;
    for (size_t i = 0; i < value_.Size(); i++) result |= static_cast<int64_t>(value_[i]) << (i * 8);

    // Sign extension
    if (value_.Size() > 0 && (value_[value_.Size() - 1] & 0x80) != 0) result |= ~((1LL << (value_.Size() * 8)) - 1);

    return result;
}

io::ByteVector ByteStringItem::GetByteArray() const { return value_; }

io::ByteSpan ByteStringItem::GetByteSpan() const { return value_.AsSpan(); }

std::string ByteStringItem::GetString() const
{
    return std::string(reinterpret_cast<const char*>(value_.Data()), value_.Size());
}

bool ByteStringItem::Equals(const StackItem& other) const
{
    if (other.GetType() == StackItemType::ByteString || other.GetType() == StackItemType::Buffer)
        return value_ == other.GetByteArray();

    if (other.GetType() == StackItemType::Integer)
    {
        try
        {
            return GetInteger() == other.GetInteger();
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

    if (other.GetType() == StackItemType::Boolean) return GetBoolean() == other.GetBoolean();

    return false;
}

// BufferItem implementation
BufferItem::BufferItem(const io::ByteVector& value) : value_(value) {}

BufferItem::BufferItem(const io::ByteSpan& value) : value_(value) {}

StackItemType BufferItem::GetType() const { return StackItemType::Buffer; }

bool BufferItem::GetBoolean() const { return value_.Size() > 0; }

io::ByteVector BufferItem::GetByteArray() const { return value_; }

io::ByteVector& BufferItem::GetSpan() { return value_; }

const io::ByteVector& BufferItem::GetSpan() const { return value_; }

std::string BufferItem::GetString() const
{
    return std::string(reinterpret_cast<const char*>(value_.Data()), value_.Size());
}

bool BufferItem::Equals(const StackItem& other) const
{
    if (other.GetType() == StackItemType::Buffer || other.GetType() == StackItemType::ByteString)
        return value_ == other.GetByteArray();

    if (other.GetType() == StackItemType::Integer)
    {
        try
        {
            int64_t otherValue = other.GetInteger();

            // Convert to little-endian byte array
            uint8_t bytes[sizeof(otherValue)];
            for (size_t i = 0; i < sizeof(otherValue); i++)
                bytes[i] = static_cast<uint8_t>((otherValue >> (i * 8)) & 0xFF);

            // Remove trailing zeros
            size_t length = sizeof(otherValue);
            while (length > 0 && bytes[length - 1] == 0) length--;

            // If the number is negative, we need to keep the sign bit
            if (otherValue < 0 && length > 0 && (bytes[length - 1] & 0x80) == 0) length++;

            // If the number is positive and the sign bit is set, we need to add a zero byte
            if (otherValue > 0 && length > 0 && (bytes[length - 1] & 0x80) != 0) length++;

            // If the number is zero, return a single zero byte
            if (length == 0) length = 1;

            return value_ == io::ByteVector(io::ByteSpan(bytes, length));
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

    if (other.GetType() == StackItemType::Boolean) return GetBoolean() == other.GetBoolean();

    return false;
}
}  // namespace neo::vm
