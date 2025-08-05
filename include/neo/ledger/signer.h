#pragma once

#include <cstdint>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/iserializable.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/uint160.h>
#include <vector>

namespace neo::ledger
{
/**
 * @brief Represents witness scopes for a signer.
 */
enum class WitnessScope : uint8_t
{
    None = 0x00,
    CalledByEntry = 0x01,
    CustomContracts = 0x10,
    CustomGroups = 0x20,
    WitnessRules = 0x40,
    Global = 0x80
};

// Bitwise operators for WitnessScope
inline WitnessScope operator&(WitnessScope lhs, WitnessScope rhs)
{
    return static_cast<WitnessScope>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

inline WitnessScope operator|(WitnessScope lhs, WitnessScope rhs)
{
    return static_cast<WitnessScope>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

inline WitnessScope operator^(WitnessScope lhs, WitnessScope rhs)
{
    return static_cast<WitnessScope>(static_cast<uint8_t>(lhs) ^ static_cast<uint8_t>(rhs));
}

inline WitnessScope operator~(WitnessScope scope)
{
    return static_cast<WitnessScope>(~static_cast<uint8_t>(scope));
}

inline WitnessScope& operator&=(WitnessScope& lhs, WitnessScope rhs)
{
    lhs = lhs & rhs;
    return lhs;
}

inline WitnessScope& operator|=(WitnessScope& lhs, WitnessScope rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

inline WitnessScope& operator^=(WitnessScope& lhs, WitnessScope rhs)
{
    lhs = lhs ^ rhs;
    return lhs;
}

/**
 * @brief Represents a transaction signer.
 */
class Signer : public io::ISerializable, public io::IJsonSerializable
{
  public:
    /**
     * @brief Constructs an empty Signer.
     */
    Signer();
    
    /**
     * @brief Virtual destructor.
     */
    virtual ~Signer() = default;

    /**
     * @brief Constructs a Signer with the specified account and scopes.
     * @param account The account.
     * @param scopes The witness scopes.
     */
    Signer(const io::UInt160& account, WitnessScope scopes);

    /**
     * @brief Gets the account.
     * @return The account.
     */
    const io::UInt160& GetAccount() const;

    /**
     * @brief Sets the account.
     * @param account The account.
     */
    void SetAccount(const io::UInt160& account);

    /**
     * @brief Gets the witness scopes.
     * @return The witness scopes.
     */
    WitnessScope GetScopes() const;

    /**
     * @brief Sets the witness scopes.
     * @param scopes The witness scopes.
     */
    void SetScopes(WitnessScope scopes);

    /**
     * @brief Gets the allowed contracts.
     * @return The allowed contracts.
     */
    const std::vector<io::UInt160>& GetAllowedContracts() const;

    /**
     * @brief Sets the allowed contracts.
     * @param allowedContracts The allowed contracts.
     */
    void SetAllowedContracts(const std::vector<io::UInt160>& allowedContracts);

    /**
     * @brief Gets the allowed groups.
     * @return The allowed groups.
     */
    const std::vector<cryptography::ecc::ECPoint>& GetAllowedGroups() const;

    /**
     * @brief Sets the allowed groups.
     * @param allowedGroups The allowed groups.
     */
    void SetAllowedGroups(const std::vector<cryptography::ecc::ECPoint>& allowedGroups);

    /**
     * @brief Serializes the Signer to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the Signer from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the Signer to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const override;

    /**
     * @brief Deserializes the Signer from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader) override;

    /**
     * @brief Checks if this Signer is equal to another Signer.
     * @param other The other Signer.
     * @return True if the Signers are equal, false otherwise.
     */
    bool operator==(const Signer& other) const;

    /**
     * @brief Checks if this Signer is not equal to another Signer.
     * @param other The other Signer.
     * @return True if the Signers are not equal, false otherwise.
     */
    bool operator!=(const Signer& other) const;

  private:
    io::UInt160 account_;
    WitnessScope scopes_;
    std::vector<io::UInt160> allowedContracts_;
    std::vector<cryptography::ecc::ECPoint> allowedGroups_;
};
}  // namespace neo::ledger