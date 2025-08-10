#pragma once

#include <neo/io/byte_vector.h>
#include <neo/io/iserializable.h>

#include <memory>

namespace neo::cryptography
{
/**
 * @brief Represents a witness for transaction validation
 */
class Witness : public io::ISerializable
{
   private:
    io::ByteVector invocation_script_;
    io::ByteVector verification_script_;

   public:
    /**
     * @brief Default constructor
     */
    Witness() = default;

    /**
     * @brief Constructor with scripts
     */
    Witness(const io::ByteVector& invocation, const io::ByteVector& verification)
        : invocation_script_(invocation), verification_script_(verification)
    {
    }

    /**
     * @brief Get invocation script
     */
    const io::ByteVector& GetInvocationScript() const { return invocation_script_; }

    /**
     * @brief Set invocation script
     */
    void SetInvocationScript(const io::ByteVector& script) { invocation_script_ = script; }

    /**
     * @brief Get verification script
     */
    const io::ByteVector& GetVerificationScript() const { return verification_script_; }

    /**
     * @brief Set verification script
     */
    void SetVerificationScript(const io::ByteVector& script) { verification_script_ = script; }

    /**
     * @brief Get size in bytes
     */
    size_t GetSize() const;

    // ISerializable implementation
    void Serialize(io::BinaryWriter& writer) const override;
    void Deserialize(io::BinaryReader& reader) override;

    // Comparison operators
    bool operator==(const Witness& other) const;
    bool operator!=(const Witness& other) const { return !(*this == other); }
};
}  // namespace neo::cryptography