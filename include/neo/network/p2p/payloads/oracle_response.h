#pragma once

#include <cstdint>
#include <neo/io/byte_vector.h>
#include <neo/network/p2p/ipayload.h>
#include <neo/network/p2p/payloads/oracle_response_code.h>

namespace neo::network::p2p::payloads
{
/**
 * @brief Indicates that the transaction is an oracle response.
 */
class OracleResponse : public IPayload
{
  public:
    /**
     * @brief Indicates the maximum size of the Result field.
     */
    static constexpr uint32_t MaxResultSize = UINT16_MAX;

    /**
     * @brief Default constructor.
     */
    OracleResponse();

    /**
     * @brief Constructs an OracleResponse with the specified parameters.
     * @param id The ID of the oracle request.
     * @param code The response code for the oracle request.
     * @param result The result for the oracle request.
     */
    OracleResponse(uint64_t id, OracleResponseCode code, const io::ByteVector& result);

    /**
     * @brief Gets the ID of the oracle request.
     * @return The ID of the oracle request.
     */
    uint64_t GetId() const;

    /**
     * @brief Sets the ID of the oracle request.
     * @param id The ID of the oracle request.
     */
    void SetId(uint64_t id);

    /**
     * @brief Gets the response code for the oracle request.
     * @return The response code for the oracle request.
     */
    OracleResponseCode GetCode() const;

    /**
     * @brief Sets the response code for the oracle request.
     * @param code The response code for the oracle request.
     */
    void SetCode(OracleResponseCode code);

    /**
     * @brief Gets the result for the oracle request.
     * @return The result for the oracle request.
     */
    const io::ByteVector& GetResult() const;

    /**
     * @brief Sets the result for the oracle request.
     * @param result The result for the oracle request.
     */
    void SetResult(const io::ByteVector& result);

    /**
     * @brief Serializes the payload to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the payload from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the payload to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const override;

    /**
     * @brief Deserializes the payload from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader) override;

  private:
    uint64_t id_;
    OracleResponseCode code_;
    io::ByteVector result_;
};
}  // namespace neo::network::p2p::payloads
