#pragma once

#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/iserializable.h>
#include <neo/io/uint160.h>

namespace neo::ledger
{
/**
 * @brief Represents a witness.
 */
class Witness : public io::ISerializable, public io::IJsonSerializable
{
   public:
    /**
     * @brief Constructs an empty Witness.
     */
    Witness();

    /**
     * @brief Virtual destructor.
     */
    virtual ~Witness() = default;

    /**
     * @brief Constructs a Witness with the specified invocation and verification scripts.
     * @param invocationScript The invocation script.
     * @param verificationScript The verification script.
     */
    Witness(const io::ByteVector& invocationScript, const io::ByteVector& verificationScript);

    /**
     * @brief Gets the invocation script.
     * @return The invocation script.
     */
    const io::ByteVector& GetInvocationScript() const;

    /**
     * @brief Sets the invocation script.
     * @param invocationScript The invocation script.
     */
    void SetInvocationScript(const io::ByteVector& invocationScript);

    /**
     * @brief Gets the verification script.
     * @return The verification script.
     */
    const io::ByteVector& GetVerificationScript() const;

    /**
     * @brief Sets the verification script.
     * @param verificationScript The verification script.
     */
    void SetVerificationScript(const io::ByteVector& verificationScript);

    /**
     * @brief Gets the script hash.
     * @return The script hash.
     */
    io::UInt160 GetScriptHash() const;

    /**
     * @brief Gets the size of the witness.
     * @return The size in bytes.
     */
    int GetSize() const;

    /**
     * @brief Serializes the witness to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the witness from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the witness to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const override;

    /**
     * @brief Deserializes the witness from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader) override;

    /**
     * @brief Checks if this witness is equal to another witness.
     * @param other The other witness.
     * @return True if the witnesses are equal, false otherwise.
     */
    bool operator==(const Witness& other) const;

    /**
     * @brief Checks if this witness is not equal to another witness.
     * @param other The other witness.
     * @return True if the witnesses are not equal, false otherwise.
     */
    bool operator!=(const Witness& other) const;

   private:
    io::ByteVector invocationScript_;
    io::ByteVector verificationScript_;
};
}  // namespace neo::ledger
