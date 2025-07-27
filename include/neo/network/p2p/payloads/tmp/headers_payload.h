#pragma once

#include <memory>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/iserializable.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/network/ipayload.h>
#include <vector>

namespace neo::blockchain
{
class Header;
}

namespace neo::network::payloads
{
/**
 * @brief Represents a headers payload.
 */
class HeadersPayload : public IPayload, public io::IJsonSerializable
{
  public:
    /**
     * @brief Constructs a HeadersPayload.
     */
    HeadersPayload();

    /**
     * @brief Constructs a HeadersPayload.
     * @param headers The block headers.
     */
    explicit HeadersPayload(const std::vector<std::shared_ptr<blockchain::Header>>& headers);

    /**
     * @brief Gets the headers.
     * @return The headers.
     */
    const std::vector<std::shared_ptr<blockchain::Header>>& GetHeaders() const;

    /**
     * @brief Sets the headers.
     * @param headers The headers.
     */
    void SetHeaders(const std::vector<std::shared_ptr<blockchain::Header>>& headers);

    /**
     * @brief Serializes the HeadersPayload to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the HeadersPayload from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the HeadersPayload to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const override;

    /**
     * @brief Deserializes the HeadersPayload from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader) override;

  private:
    std::vector<std::shared_ptr<blockchain::Header>> headers_;
};
}  // namespace neo::network::payloads
