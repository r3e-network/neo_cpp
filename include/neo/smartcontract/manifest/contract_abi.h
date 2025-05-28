#pragma once

#include <neo/io/serializable.h>
#include <string>
#include <vector>

namespace neo::smartcontract::manifest
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
     * @brief Represents a contract parameter definition.
     */
    class ContractParameterDefinition : public io::ISerializable
    {
    public:
        /**
         * @brief Constructs a ContractParameterDefinition.
         */
        ContractParameterDefinition();

        /**
         * @brief Gets the name.
         * @return The name.
         */
        const std::string& GetName() const;

        /**
         * @brief Sets the name.
         * @param name The name.
         */
        void SetName(const std::string& name);

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
        std::string name_;
        ContractParameterType type_;
    };

    /**
     * @brief Represents a contract method descriptor.
     */
    class ContractMethodDescriptor : public io::ISerializable
    {
    public:
        /**
         * @brief Constructs a ContractMethodDescriptor.
         */
        ContractMethodDescriptor();

        /**
         * @brief Gets the name.
         * @return The name.
         */
        const std::string& GetName() const;

        /**
         * @brief Sets the name.
         * @param name The name.
         */
        void SetName(const std::string& name);

        /**
         * @brief Gets the parameters.
         * @return The parameters.
         */
        const std::vector<ContractParameterDefinition>& GetParameters() const;

        /**
         * @brief Sets the parameters.
         * @param parameters The parameters.
         */
        void SetParameters(const std::vector<ContractParameterDefinition>& parameters);

        /**
         * @brief Gets the return type.
         * @return The return type.
         */
        ContractParameterType GetReturnType() const;

        /**
         * @brief Sets the return type.
         * @param returnType The return type.
         */
        void SetReturnType(ContractParameterType returnType);

        /**
         * @brief Gets the offset.
         * @return The offset.
         */
        uint32_t GetOffset() const;

        /**
         * @brief Sets the offset.
         * @param offset The offset.
         */
        void SetOffset(uint32_t offset);

        /**
         * @brief Gets whether the method is safe.
         * @return True if the method is safe, false otherwise.
         */
        bool IsSafe() const;

        /**
         * @brief Sets whether the method is safe.
         * @param safe True if the method is safe, false otherwise.
         */
        void SetSafe(bool safe);

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
        std::string name_;
        std::vector<ContractParameterDefinition> parameters_;
        ContractParameterType returnType_;
        uint32_t offset_;
        bool safe_;
    };

    /**
     * @brief Represents a contract event descriptor.
     */
    class ContractEventDescriptor : public io::ISerializable
    {
    public:
        /**
         * @brief Constructs a ContractEventDescriptor.
         */
        ContractEventDescriptor();

        /**
         * @brief Gets the name.
         * @return The name.
         */
        const std::string& GetName() const;

        /**
         * @brief Sets the name.
         * @param name The name.
         */
        void SetName(const std::string& name);

        /**
         * @brief Gets the parameters.
         * @return The parameters.
         */
        const std::vector<ContractParameterDefinition>& GetParameters() const;

        /**
         * @brief Sets the parameters.
         * @param parameters The parameters.
         */
        void SetParameters(const std::vector<ContractParameterDefinition>& parameters);

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
        std::string name_;
        std::vector<ContractParameterDefinition> parameters_;
    };

    /**
     * @brief Represents a contract ABI.
     */
    class ContractAbi : public io::ISerializable
    {
    public:
        /**
         * @brief Constructs a ContractAbi.
         */
        ContractAbi();

        /**
         * @brief Gets the methods.
         * @return The methods.
         */
        const std::vector<ContractMethodDescriptor>& GetMethods() const;

        /**
         * @brief Sets the methods.
         * @param methods The methods.
         */
        void SetMethods(const std::vector<ContractMethodDescriptor>& methods);

        /**
         * @brief Gets the events.
         * @return The events.
         */
        const std::vector<ContractEventDescriptor>& GetEvents() const;

        /**
         * @brief Sets the events.
         * @param events The events.
         */
        void SetEvents(const std::vector<ContractEventDescriptor>& events);

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
        std::vector<ContractMethodDescriptor> methods_;
        std::vector<ContractEventDescriptor> events_;
    };
}
