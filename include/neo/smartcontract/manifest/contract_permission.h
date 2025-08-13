/**
 * @file contract_permission.h
 * @brief Contract Permission
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/io/serializable.h>
#include <neo/io/uint160.h>

#include <string>
#include <vector>

namespace neo::smartcontract::manifest
{
/**
 * @brief Represents a contract permission descriptor.
 *
 * Indicates which contracts are authorized to be called.
 */
class ContractPermissionDescriptor : public io::ISerializable
{
   public:
    /**
     * @brief Constructs a ContractPermissionDescriptor.
     */
    ContractPermissionDescriptor();

    /**
     * @brief Constructs a ContractPermissionDescriptor with a hash.
     * @param hash The hash.
     */
    explicit ContractPermissionDescriptor(const io::UInt160& hash);

    /**
     * @brief Constructs a ContractPermissionDescriptor with a group.
     * @param group The group.
     */
    explicit ContractPermissionDescriptor(const cryptography::ecc::ECPoint& group);

    /**
     * @brief Gets the hash.
     * @return The hash.
     */
    const io::UInt160& GetHash() const;

    /**
     * @brief Gets the group.
     * @return The group.
     */
    const cryptography::ecc::ECPoint& GetGroup() const;

    /**
     * @brief Determines whether the descriptor is a hash.
     * @return True if the descriptor is a hash, false otherwise.
     */
    bool IsHash() const;

    /**
     * @brief Determines whether the descriptor is a group.
     * @return True if the descriptor is a group, false otherwise.
     */
    bool IsGroup() const;

    /**
     * @brief Determines whether the descriptor is a wildcard.
     * @return True if the descriptor is a wildcard, false otherwise.
     */
    bool IsWildcard() const;

    /**
     * @brief Creates a wildcard descriptor.
     * @return The wildcard descriptor.
     */
    static ContractPermissionDescriptor CreateWildcard();

    /**
     * @brief Creates a descriptor with a hash.
     * @param hash The hash.
     * @return The descriptor.
     */
    static ContractPermissionDescriptor Create(const io::UInt160& hash);

    /**
     * @brief Creates a descriptor with a group.
     * @param group The group.
     * @return The descriptor.
     */
    static ContractPermissionDescriptor Create(const cryptography::ecc::ECPoint& group);

    /**
     * @brief Serializes the object.
     * @param writer The writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the object.
     * @param reader The reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

   private:
    io::UInt160 hash_;
    cryptography::ecc::ECPoint group_;
    bool isWildcard_;
};

/**
 * @brief Represents a contract permission.
 *
 * It describes which contracts may be invoked and which methods are called.
 * If a contract invokes a contract or method that is not declared in the manifest
 * at runtime, the invocation will fail.
 */
class ContractPermission : public io::ISerializable
{
   public:
    /**
     * @brief Constructs a ContractPermission.
     */
    ContractPermission();

    /**
     * @brief Gets the contract.
     * @return The contract.
     */
    const ContractPermissionDescriptor& GetContract() const;

    /**
     * @brief Sets the contract.
     * @param contract The contract.
     */
    void SetContract(const ContractPermissionDescriptor& contract);

    /**
     * @brief Gets the methods.
     * @return The methods.
     */
    const std::vector<std::string>& GetMethods() const;

    /**
     * @brief Sets the methods.
     * @param methods The methods.
     */
    void SetMethods(const std::vector<std::string>& methods);

    /**
     * @brief Determines whether the methods are a wildcard.
     * @return True if the methods are a wildcard, false otherwise.
     */
    bool IsMethodsWildcard() const;

    /**
     * @brief Sets whether the methods are a wildcard.
     * @param isWildcard True if the methods are a wildcard, false otherwise.
     */
    void SetMethodsWildcard(bool isWildcard);

    /**
     * @brief Creates a default permission.
     * @return The default permission.
     */
    static ContractPermission CreateDefault();

    /**
     * @brief Serializes the object.
     * @param writer The writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the object.
     * @param reader The reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

   private:
    ContractPermissionDescriptor contract_;
    std::vector<std::string> methods_;
    bool isMethodsWildcard_;
};
}  // namespace neo::smartcontract::manifest
