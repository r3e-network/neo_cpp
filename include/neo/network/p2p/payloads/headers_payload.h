#pragma once

#include <memory>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/ledger/block_header.h>
#include <neo/network/p2p/ipayload.h>
#include <vector>

namespace neo::network::p2p::payloads
{
/**
 * @brief Represents a headers payload.
 *
 * This message is sent to respond to GetHeaders messages.
 */
class HeadersPayload : public IPayload
{
  public:
    /**
     * @brief Indicates the maximum number of headers sent each time.
     */
    static constexpr int MaxHeadersCount = 2000;

    /**
     * @brief Constructs a HeadersPayload.
     */
    HeadersPayload();

    /**
     * @brief Constructs a HeadersPayload.
     * @param headers The block headers.
     */
    explicit HeadersPayload(const std::vector<std::shared_ptr<ledger::BlockHeader>>& headers);

    /**
     * @brief Gets the headers.
     * @return The headers.
     */
    const std::vector<std::shared_ptr<ledger::BlockHeader>>& GetHeaders() const;

    /**
     * @brief Sets the headers.
     * @param headers The headers.
     */
    void SetHeaders(const std::vector<std::shared_ptr<ledger::BlockHeader>>& headers);

    /**
     * @brief Gets the size of the payload.
     * @return The size of the payload.
     */
    size_t GetSize() const;

    /**
     * @brief Creates a new instance of the HeadersPayload class.
     * @param headers The list of headers.
     * @return The created payload.
     */
    static HeadersPayload Create(const std::vector<std::shared_ptr<ledger::BlockHeader>>& headers);

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
    void SerializeJson(io::JsonWriter& writer) const;

    /**
     * @brief Deserializes the HeadersPayload from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader);

  private:
    std::vector<std::shared_ptr<ledger::BlockHeader>> headers_;
};
}  // namespace neo::network::p2p::payloads
