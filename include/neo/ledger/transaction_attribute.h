#pragma once

#include <neo/io/byte_vector.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/iserializable.h>

#include <cstdint>

namespace neo::ledger
{
/**
 * @brief Represents a transaction attribute.
 */
class TransactionAttribute : public io::ISerializable, public io::IJsonSerializable
{
   public:
    /**
     * @brief Enum for transaction attribute usage (Neo N3 TransactionAttributeType).
     */
    enum class Usage : uint8_t
    {
        // Neo N3 TransactionAttributeType values
        HighPriority = 0x01,
        OracleResponse = 0x11,
        NotValidBefore = 0x20,
        Conflicts = 0x21,
        NotaryAssisted = 0x22,

        // Legacy Neo 2.x values (kept for compatibility)
        ContractHash = 0x00,
        ECDH02 = 0x02,
        ECDH03 = 0x03,
        Script = 0x20,  // Note: conflicts with NotValidBefore, but kept for compatibility
        Vote = 0x30,
        DescriptionUrl = 0x81,
        Description = 0x90,
        Hash1 = 0xa1,
        Hash2 = 0xa2,
        Hash3 = 0xa3,
        Hash4 = 0xa4,
        Hash5 = 0xa5,
        Hash6 = 0xa6,
        Hash7 = 0xa7,
        Hash8 = 0xa8,
        Hash9 = 0xa9,
        Hash10 = 0xaa,
        Hash11 = 0xab,
        Hash12 = 0xac,
        Hash13 = 0xad,
        Hash14 = 0xae,
        Hash15 = 0xaf,
        Remark = 0xf0,
        Remark1 = 0xf1,
        Remark2 = 0xf2,
        Remark3 = 0xf3,
        Remark4 = 0xf4,
        Remark5 = 0xf5,
        Remark6 = 0xf6,
        Remark7 = 0xf7,
        Remark8 = 0xf8,
        Remark9 = 0xf9,
        Remark10 = 0xfa,
        Remark11 = 0xfb,
        Remark12 = 0xfc,
        Remark13 = 0xfd,
        Remark14 = 0xfe,
        Remark15 = 0xff
    };

    /**
     * @brief Constructs an empty TransactionAttribute.
     */
    TransactionAttribute();

    /**
     * @brief Constructs a TransactionAttribute with the specified usage and data.
     * @param usage The usage.
     * @param data The data.
     */
    TransactionAttribute(Usage usage, const io::ByteVector& data);

    /**
     * @brief Gets the usage.
     * @return The usage.
     */
    Usage GetUsage() const;

    /**
     * @brief Gets the type (alias for GetUsage for RPC compatibility).
     * @return The usage/type.
     */
    Usage GetType() const { return GetUsage(); }

    /**
     * @brief Sets the usage.
     * @param usage The usage.
     */
    void SetUsage(Usage usage);

    /**
     * @brief Gets the data.
     * @return The data.
     */
    const io::ByteVector& GetData() const;

    /**
     * @brief Sets the data.
     * @param data The data.
     */
    void SetData(const io::ByteVector& data);

    /**
     * @brief Serializes the TransactionAttribute to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the TransactionAttribute from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the TransactionAttribute to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const override;

    /**
     * @brief Deserializes the TransactionAttribute from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader) override;

    /**
     * @brief Checks if this TransactionAttribute is equal to another TransactionAttribute.
     * @param other The other TransactionAttribute.
     * @return True if the TransactionAttributes are equal, false otherwise.
     */
    bool operator==(const TransactionAttribute& other) const;

    /**
     * @brief Checks if this TransactionAttribute is not equal to another TransactionAttribute.
     * @param other The other TransactionAttribute.
     * @return True if the TransactionAttributes are not equal, false otherwise.
     */
    bool operator!=(const TransactionAttribute& other) const;

   private:
    Usage usage_;
    io::ByteVector data_;
};
}  // namespace neo::ledger
