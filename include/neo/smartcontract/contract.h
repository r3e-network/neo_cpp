/**
 * @file contract.h
 * @brief Contract
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/io/byte_vector.h>
#include <neo/io/iserializable.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>

#include <optional>
#include <string>
#include <vector>

namespace neo::smartcontract
{
/**
 * @brief Represents a contract parameter type.
 */
enum class ContractParameterType : uint8_t
{
    Signature = 0x00,
    Boolean = 0x01,
    Integer = 0x02,
    Hash160 = 0x03,
    Hash256 = 0x04,
    ByteArray = 0x05,
    PublicKey = 0x06,
    String = 0x07,
    Array = 0x10,
    Map = 0x12,
    InteropInterface = 0x40,
    Void = 0xff
};

/**
 * @brief Represents a contract parameter.
 */
class ContractParameter
{
   public:
    /**
     * @brief Constructs an empty ContractParameter.
     */
    ContractParameter();

    /**
     * @brief Constructs a ContractParameter with the specified type.
     * @param type The type.
     */
    explicit ContractParameter(ContractParameterType type);

    /**
     * @brief Gets the type.
     * @return The type.
     */
    ContractParameterType GetType() const;

    /**
     * @brief Sets the type.
     * @param type The type.
     */
    void SetType(ContractParameterType type);

    /**
     * @brief Gets the value.
     * @return The value.
     */
    const std::optional<io::ByteVector>& GetValue() const;

    /**
     * @brief Sets the value.
     * @param value The value.
     */
    void SetValue(const io::ByteVector& value);

    /**
     * @brief Gets the array value.
     * @return The array value.
     */
    const std::vector<ContractParameter>& GetArray() const;

    /**
     * @brief Sets the array value.
     * @param value The array value.
     */
    void SetArray(const std::vector<ContractParameter>& value);

    /**
     * @brief Gets the map value.
     * @return The map value.
     */
    const std::vector<std::pair<ContractParameter, ContractParameter>>& GetMap() const;

    /**
     * @brief Sets the map value.
     * @param value The map value.
     */
    void SetMap(const std::vector<std::pair<ContractParameter, ContractParameter>>& value);

    /**
     * @brief Creates a signature parameter.
     * @param value The value.
     * @return The parameter.
     */
    static ContractParameter CreateSignature(const io::ByteVector& value);

    /**
     * @brief Creates a boolean parameter.
     * @param value The value.
     * @return The parameter.
     */
    static ContractParameter CreateBoolean(bool value);

    /**
     * @brief Creates an integer parameter.
     * @param value The value.
     * @return The parameter.
     */
    static ContractParameter CreateInteger(int64_t value);

    /**
     * @brief Creates a Hash160 parameter.
     * @param value The value.
     * @return The parameter.
     */
    static ContractParameter CreateHash160(const io::UInt160& value);

    /**
     * @brief Creates a Hash256 parameter.
     * @param value The value.
     * @return The parameter.
     */
    static ContractParameter CreateHash256(const io::UInt256& value);

    /**
     * @brief Creates a byte array parameter.
     * @param value The value.
     * @return The parameter.
     */
    static ContractParameter CreateByteArray(const io::ByteVector& value);

    /**
     * @brief Creates a public key parameter.
     * @param value The value.
     * @return The parameter.
     */
    static ContractParameter CreatePublicKey(const cryptography::ecc::ECPoint& value);

    /**
     * @brief Creates a string parameter.
     * @param value The value.
     * @return The parameter.
     */
    static ContractParameter CreateString(const std::string& value);

    /**
     * @brief Creates an array parameter.
     * @param value The value.
     * @return The parameter.
     */
    static ContractParameter CreateArray(const std::vector<ContractParameter>& value);

    /**
     * @brief Creates a map parameter.
     * @param value The value.
     * @return The parameter.
     */
    static ContractParameter CreateMap(const std::vector<std::pair<ContractParameter, ContractParameter>>& value);

    /**
     * @brief Creates a void parameter.
     * @return The parameter.
     */
    static ContractParameter CreateVoid();

   private:
    ContractParameterType type_;
    std::optional<io::ByteVector> value_;
    std::vector<ContractParameter> array_;
    std::vector<std::pair<ContractParameter, ContractParameter>> map_;
};

/**
 * @brief Represents a contract.
 */
class Contract : public io::ISerializable
{
   public:
    /**
     * @brief Constructs an empty Contract.
     */
    Contract();

    /**
     * @brief Constructs a Contract with the specified script and parameter list.
     * @param script The script.
     * @param parameterList The parameter list.
     */
    Contract(const io::ByteVector& script, const std::vector<ContractParameterType>& parameterList);

    /**
     * @brief Gets the script.
     * @return The script.
     */
    const io::ByteVector& GetScript() const;

    /**
     * @brief Sets the script.
     * @param script The script.
     */
    void SetScript(const io::ByteVector& script);

    /**
     * @brief Gets the parameter list.
     * @return The parameter list.
     */
    const std::vector<ContractParameterType>& GetParameterList() const;

    /**
     * @brief Sets the parameter list.
     * @param parameterList The parameter list.
     */
    void SetParameterList(const std::vector<ContractParameterType>& parameterList);

    /**
     * @brief Gets the script hash.
     * @return The script hash.
     */
    io::UInt160 GetScriptHash() const;

    /**
     * @brief Serializes the Contract to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the Contract from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Creates a contract from a public key.
     * @param pubKey The public key.
     * @return The contract.
     */
    static Contract CreateSignatureContract(const cryptography::ecc::ECPoint& pubKey);

    /**
     * @brief Creates a multi-signature contract.
     * @param m The minimum number of signatures required.
     * @param pubKeys The public keys.
     * @return The contract.
     */
    static Contract CreateMultiSigContract(int m, const std::vector<cryptography::ecc::ECPoint>& pubKeys);

   private:
    io::ByteVector script_;
    std::vector<ContractParameterType> parameterList_;
};

/**
 * @brief Represents a contract state.
 */
class ContractState : public io::ISerializable
{
   public:
    /**
     * @brief Constructs an empty ContractState.
     */
    ContractState();

    /**
     * @brief Gets the id.
     * @return The id.
     */
    int32_t GetId() const;

    /**
     * @brief Sets the id.
     * @param id The id.
     */
    void SetId(int32_t id);

    /**
     * @brief Gets the script hash.
     * @return The script hash.
     */
    const io::UInt160& GetScriptHash() const;

    /**
     * @brief Sets the script hash.
     * @param scriptHash The script hash.
     */
    void SetScriptHash(const io::UInt160& scriptHash);

    /**
     * @brief Gets the script.
     * @return The script.
     */
    const io::ByteVector& GetScript() const;

    /**
     * @brief Sets the script.
     * @param script The script.
     */
    void SetScript(const io::ByteVector& script);

    /**
     * @brief Gets the manifest.
     * @return The manifest.
     */
    const std::string& GetManifest() const;

    /**
     * @brief Sets the manifest.
     * @param manifest The manifest.
     */
    void SetManifest(const std::string& manifest);

    /**
     * @brief Gets the update counter.
     * @return The update counter.
     */
    uint16_t GetUpdateCounter() const;

    /**
     * @brief Sets the update counter.
     * @param updateCounter The update counter.
     */
    void SetUpdateCounter(uint16_t updateCounter);

    /**
     * @brief Serializes the ContractState to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the ContractState from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

   private:
    int32_t id_;
    uint16_t updateCounter_;
    io::UInt160 scriptHash_;
    io::ByteVector script_;
    std::string manifest_;
};
}  // namespace neo::smartcontract
