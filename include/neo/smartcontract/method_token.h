#pragma once

#include <neo/io/iserializable.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/uint160.h>
#include <neo/smartcontract/call_flags.h>
#include <string>
#include <cstdint>

namespace neo::smartcontract
{
    /**
     * @brief Represents the methods that a contract will call statically.
     */
    class MethodToken : public io::ISerializable, public io::IJsonSerializable
    {
    public:
        /**
         * @brief Constructs an empty MethodToken.
         */
        MethodToken();

        /**
         * @brief Gets the hash of the contract to be called.
         * @return The hash of the contract to be called.
         */
        const io::UInt160& GetHash() const;

        /**
         * @brief Sets the hash of the contract to be called.
         * @param hash The hash of the contract to be called.
         */
        void SetHash(const io::UInt160& hash);

        /**
         * @brief Gets the name of the method to be called.
         * @return The name of the method to be called.
         */
        const std::string& GetMethod() const;

        /**
         * @brief Sets the name of the method to be called.
         * @param method The name of the method to be called.
         */
        void SetMethod(const std::string& method);

        /**
         * @brief Gets the number of parameters of the method to be called.
         * @return The number of parameters of the method to be called.
         */
        uint16_t GetParametersCount() const;

        /**
         * @brief Sets the number of parameters of the method to be called.
         * @param parametersCount The number of parameters of the method to be called.
         */
        void SetParametersCount(uint16_t parametersCount);

        /**
         * @brief Gets whether the method to be called has a return value.
         * @return Whether the method to be called has a return value.
         */
        bool GetHasReturnValue() const;

        /**
         * @brief Sets whether the method to be called has a return value.
         * @param hasReturnValue Whether the method to be called has a return value.
         */
        void SetHasReturnValue(bool hasReturnValue);

        /**
         * @brief Gets the call flags to be used to call the contract.
         * @return The call flags to be used to call the contract.
         */
        CallFlags GetCallFlags() const;

        /**
         * @brief Sets the call flags to be used to call the contract.
         * @param callFlags The call flags to be used to call the contract.
         */
        void SetCallFlags(CallFlags callFlags);

        /**
         * @brief Serializes the MethodToken to a binary writer.
         * @param writer The binary writer.
         */
        void Serialize(io::BinaryWriter& writer) const override;

        /**
         * @brief Deserializes the MethodToken from a binary reader.
         * @param reader The binary reader.
         */
        void Deserialize(io::BinaryReader& reader) override;

        /**
         * @brief Serializes the MethodToken to a JSON writer.
         * @param writer The JSON writer.
         */
        void SerializeJson(io::JsonWriter& writer) const override;

        /**
         * @brief Deserializes the MethodToken from a JSON reader.
         * @param reader The JSON reader.
         */
        void DeserializeJson(const io::JsonReader& reader) override;

    private:
        io::UInt160 hash_;
        std::string method_;
        uint16_t parametersCount_;
        bool hasReturnValue_;
        CallFlags callFlags_;
    };
}
