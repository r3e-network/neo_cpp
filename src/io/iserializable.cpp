/**
 * @file iserializable.cpp
 * @brief Serialization interfaces
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/iserializable.h>

#include <sstream>

namespace neo::io
{
ByteVector ISerializable::ToArray() const
{
    std::ostringstream stream;
    BinaryWriter writer(stream);
    Serialize(writer);

    std::string str = stream.str();
    return ByteVector(ByteSpan(reinterpret_cast<const uint8_t*>(str.data()), str.size()));
}

void ISerializable::DeserializeFromArray(const ByteSpan& data)
{
    std::istringstream stream(std::string(reinterpret_cast<const char*>(data.Data()), data.Size()));
    BinaryReader reader(stream);
    Deserialize(reader);
}
}  // namespace neo::io
