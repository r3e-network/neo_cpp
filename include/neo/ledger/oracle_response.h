#pragma once

#include <neo/ledger/transaction_attribute.h>
#include <neo/io/byte_vector.h>
#include <cstdint>

namespace neo::ledger
{
    /**
     * @brief Oracle response codes.
     */
    enum class OracleResponseCode : uint8_t
    {
        Success = 0x00,
        ProtocolNotSupported = 0x10,
        ConsensusUnreachable = 0x12,
        NotFound = 0x14,
        Timeout = 0x16,
        Forbidden = 0x18,
        ResponseTooLarge = 0x1a,
        InsufficientFunds = 0x1c,
        ContentTypeNotSupported = 0x1f,
        Error = 0xff
    };

    /**
     * @brief Represents an oracle response transaction attribute.
     */
    class OracleResponse : public TransactionAttribute
    {
    public:
        /**
         * @brief Constructs an empty OracleResponse.
         */
        OracleResponse();

        /**
         * @brief Constructs an OracleResponse with the specified parameters.
         * @param id The oracle request ID.
         * @param code The response code.
         * @param result The response result.
         */
        OracleResponse(uint64_t id, OracleResponseCode code, const io::ByteVector& result);

        /**
         * @brief Gets the oracle request ID.
         * @return The oracle request ID.
         */
        uint64_t GetId() const;

        /**
         * @brief Sets the oracle request ID.
         * @param id The oracle request ID.
         */
        void SetId(uint64_t id);

        /**
         * @brief Gets the response code.
         * @return The response code.
         */
        OracleResponseCode GetCode() const;

        /**
         * @brief Sets the response code.
         * @param code The response code.
         */
        void SetCode(OracleResponseCode code);

        /**
         * @brief Gets the response result.
         * @return The response result.
         */
        const io::ByteVector& GetResult() const;

        /**
         * @brief Sets the response result.
         * @param result The response result.
         */
        void SetResult(const io::ByteVector& result);

        /**
         * @brief Serializes the OracleResponse to a binary writer.
         * @param writer The binary writer.
         */
        void Serialize(io::BinaryWriter& writer) const override;

        /**
         * @brief Deserializes the OracleResponse from a binary reader.
         * @param reader The binary reader.
         */
        void Deserialize(io::BinaryReader& reader) override;

        /**
         * @brief Gets the size of the OracleResponse.
         * @return The size in bytes.
         */
        size_t GetSize() const;

        /**
         * @brief Checks if this OracleResponse is equal to another OracleResponse.
         * @param other The other OracleResponse.
         * @return True if the OracleResponses are equal, false otherwise.
         */
        bool operator==(const OracleResponse& other) const;

        /**
         * @brief Checks if this OracleResponse is not equal to another OracleResponse.
         * @param other The other OracleResponse.
         * @return True if the OracleResponses are not equal, false otherwise.
         */
        bool operator!=(const OracleResponse& other) const;

    private:
        uint64_t id_;
        OracleResponseCode code_;
        io::ByteVector result_;
    };
}
