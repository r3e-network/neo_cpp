/**
 * @file contract_abi.cpp
 * @brief Contract Abi
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/smartcontract/manifest/contract_abi.h>

namespace neo::smartcontract::manifest
{
// ContractParameterDefinition implementation
ContractParameterDefinition::ContractParameterDefinition() : type_(ContractParameterType::Void) {}

const std::string& ContractParameterDefinition::GetName() const { return name_; }

void ContractParameterDefinition::SetName(const std::string& name) { name_ = name; }

ContractParameterType ContractParameterDefinition::GetType() const { return type_; }

void ContractParameterDefinition::SetType(ContractParameterType type) { type_ = type; }

void ContractParameterDefinition::Serialize(io::BinaryWriter& writer) const
{
    writer.WriteVarString(name_);
    writer.Write(static_cast<uint8_t>(type_));
}

void ContractParameterDefinition::Deserialize(io::BinaryReader& reader)
{
    name_ = reader.ReadVarString();
    type_ = static_cast<ContractParameterType>(reader.ReadUInt8());
}

// ContractMethodDescriptor implementation
ContractMethodDescriptor::ContractMethodDescriptor()
    : returnType_(ContractParameterType::Void), offset_(0), safe_(false)
{
}

const std::string& ContractMethodDescriptor::GetName() const { return name_; }

void ContractMethodDescriptor::SetName(const std::string& name) { name_ = name; }

const std::vector<ContractParameterDefinition>& ContractMethodDescriptor::GetParameters() const { return parameters_; }

void ContractMethodDescriptor::SetParameters(const std::vector<ContractParameterDefinition>& parameters)
{
    parameters_ = parameters;
}

ContractParameterType ContractMethodDescriptor::GetReturnType() const { return returnType_; }

void ContractMethodDescriptor::SetReturnType(ContractParameterType returnType) { returnType_ = returnType; }

uint32_t ContractMethodDescriptor::GetOffset() const { return offset_; }

void ContractMethodDescriptor::SetOffset(uint32_t offset) { offset_ = offset; }

bool ContractMethodDescriptor::IsSafe() const { return safe_; }

void ContractMethodDescriptor::SetSafe(bool safe) { safe_ = safe; }

void ContractMethodDescriptor::Serialize(io::BinaryWriter& writer) const
{
    writer.WriteVarString(name_);
    writer.WriteVarInt(parameters_.size());
    for (const auto& parameter : parameters_)
    {
        parameter.Serialize(writer);
    }
    writer.Write(static_cast<uint8_t>(returnType_));
    writer.WriteVarInt(offset_);
    writer.Write(safe_);
}

void ContractMethodDescriptor::Deserialize(io::BinaryReader& reader)
{
    name_ = reader.ReadVarString();
    uint64_t count = reader.ReadVarInt();
    parameters_.resize(count);
    for (uint64_t i = 0; i < count; i++)
    {
        parameters_[i].Deserialize(reader);
    }
    returnType_ = static_cast<ContractParameterType>(reader.ReadUInt8());
    offset_ = static_cast<uint32_t>(reader.ReadVarInt());
    safe_ = reader.ReadBool();
}

// ContractEventDescriptor implementation
ContractEventDescriptor::ContractEventDescriptor() {}

const std::string& ContractEventDescriptor::GetName() const { return name_; }

void ContractEventDescriptor::SetName(const std::string& name) { name_ = name; }

const std::vector<ContractParameterDefinition>& ContractEventDescriptor::GetParameters() const { return parameters_; }

void ContractEventDescriptor::SetParameters(const std::vector<ContractParameterDefinition>& parameters)
{
    parameters_ = parameters;
}

void ContractEventDescriptor::Serialize(io::BinaryWriter& writer) const
{
    writer.WriteVarString(name_);
    writer.WriteVarInt(parameters_.size());
    for (const auto& parameter : parameters_)
    {
        parameter.Serialize(writer);
    }
}

void ContractEventDescriptor::Deserialize(io::BinaryReader& reader)
{
    name_ = reader.ReadVarString();
    uint64_t count = reader.ReadVarInt();
    parameters_.resize(count);
    for (uint64_t i = 0; i < count; i++)
    {
        parameters_[i].Deserialize(reader);
    }
}

// ContractAbi implementation
ContractAbi::ContractAbi() {}

const std::vector<ContractMethodDescriptor>& ContractAbi::GetMethods() const { return methods_; }

void ContractAbi::SetMethods(const std::vector<ContractMethodDescriptor>& methods) { methods_ = methods; }

const std::vector<ContractEventDescriptor>& ContractAbi::GetEvents() const { return events_; }

void ContractAbi::SetEvents(const std::vector<ContractEventDescriptor>& events) { events_ = events; }

void ContractAbi::Serialize(io::BinaryWriter& writer) const
{
    writer.WriteVarInt(methods_.size());
    for (const auto& method : methods_)
    {
        method.Serialize(writer);
    }
    writer.WriteVarInt(events_.size());
    for (const auto& event : events_)
    {
        event.Serialize(writer);
    }
}

void ContractAbi::Deserialize(io::BinaryReader& reader)
{
    uint64_t count = reader.ReadVarInt();
    methods_.resize(count);
    for (uint64_t i = 0; i < count; i++)
    {
        methods_[i].Deserialize(reader);
    }
    count = reader.ReadVarInt();
    events_.resize(count);
    for (uint64_t i = 0; i < count; i++)
    {
        events_[i].Deserialize(reader);
    }
}
}  // namespace neo::smartcontract::manifest
