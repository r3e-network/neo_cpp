#include <iostream>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json.h>
#include <neo/smartcontract/manifest/contract_abi.h>
#include <neo/smartcontract/manifest/contract_manifest.h>
#include <neo/smartcontract/manifest/contract_permission.h>
#include <set>
#include <sstream>

namespace neo::smartcontract::manifest
{
ContractManifest::ContractManifest() : abi_(new ContractAbi()) {}

ContractManifest::~ContractManifest()
{
    delete abi_;
}

const std::string& ContractManifest::GetName() const
{
    return name_;
}

void ContractManifest::SetName(const std::string& name)
{
    name_ = name;
}

const std::vector<std::string>& ContractManifest::GetSupportedStandards() const
{
    return supportedStandards_;
}

void ContractManifest::SetSupportedStandards(const std::vector<std::string>& supportedStandards)
{
    supportedStandards_ = supportedStandards;
}

const ContractAbi& ContractManifest::GetAbi() const
{
    return *abi_;
}

void ContractManifest::SetAbi(const ContractAbi& abi)
{
    *abi_ = abi;
}

const std::vector<ContractPermission>& ContractManifest::GetPermissions() const
{
    return permissions_;
}

void ContractManifest::SetPermissions(const std::vector<ContractPermission>& permissions)
{
    permissions_ = permissions;
}

const std::vector<ContractPermissionDescriptor>& ContractManifest::GetTrusts() const
{
    return trusts_;
}

void ContractManifest::SetTrusts(const std::vector<ContractPermissionDescriptor>& trusts)
{
    trusts_ = trusts;
}

const std::string& ContractManifest::GetExtra() const
{
    return extra_;
}

void ContractManifest::SetExtra(const std::string& extra)
{
    extra_ = extra;
}

bool ContractManifest::IsValid(const vm::ExecutionEngineLimits& limits, const io::UInt160& hash) const
{
    // Basic validation implementation
    try
    {
        // TODO: Implement ToStackItem method when StackItem is available
        // For now, just do basic validation

        // Groups validation would go here when groups are implemented
        // TODO: Add groups member and validation

        // Validate name is not empty
        if (name_.empty())
            return false;

        // Validate supported standards don't contain empty strings
        for (const auto& standard : supportedStandards_)
        {
            if (standard.empty())
                return false;
        }

        // TODO: Validate ABI when IsValid method is implemented in ContractAbi
        // if (abi_ && !abi_->IsValid())
        //     return false;

        // All validations passed
        return true;
    }
    catch (const std::exception& e)
    {
        // Any exception during validation means the manifest is invalid
        std::cerr << "Contract manifest validation failed: " << e.what() << std::endl;
        return false;
    }
}

ContractManifest ContractManifest::Parse(const std::string& jsonStr)
{
    if (jsonStr.size() > MaxLength)
        throw std::runtime_error("Manifest is too large");

    // Basic JSON parsing implementation
    auto jsonObj = nlohmann::json::parse(jsonStr);

    ContractManifest manifest;

    if (jsonObj.contains("name") && jsonObj["name"].is_string())
    {
        manifest.SetName(jsonObj["name"].get<std::string>());
    }

    // Parse supported standards
    std::vector<std::string> standards;
    if (jsonObj.contains("supportedstandards") && jsonObj["supportedstandards"].is_array())
    {
        for (const auto& standard : jsonObj["supportedstandards"])
        {
            if (standard.is_string())
            {
                standards.push_back(standard.get<std::string>());
            }
        }
    }
    manifest.SetSupportedStandards(standards);

    // TODO: Parse other fields when their classes are properly implemented

    return manifest;
}

std::string ContractManifest::ToJson() const
{
    nlohmann::json jsonObj;
    jsonObj["name"] = name_;

    // Serialize supported standards
    nlohmann::json supportedStandards = nlohmann::json::array();
    for (const auto& standard : supportedStandards_)
    {
        supportedStandards.push_back(standard);
    }
    jsonObj["supportedstandards"] = supportedStandards;

    // TODO: Serialize other fields when their classes are properly implemented
    jsonObj["abi"] = nlohmann::json::object();
    jsonObj["permissions"] = nlohmann::json::array();
    jsonObj["trusts"] = nlohmann::json::array();
    jsonObj["groups"] = nlohmann::json::array();
    jsonObj["features"] = nlohmann::json::object();

    if (!extra_.empty())
    {
        try
        {
            jsonObj["extra"] = nlohmann::json::parse(extra_);
        }
        catch (...)
        {
            jsonObj["extra"] = extra_;
        }
    }
    else
    {
        jsonObj["extra"] = nullptr;
    }

    return jsonObj.dump();
}

void ContractManifest::Serialize(io::BinaryWriter& writer) const
{
    writer.WriteVarString(name_);
    writer.WriteVarInt(supportedStandards_.size());
    for (const auto& standard : supportedStandards_)
    {
        writer.WriteVarString(standard);
    }
    abi_->Serialize(writer);
    writer.WriteVarInt(permissions_.size());
    for (const auto& permission : permissions_)
    {
        permission.Serialize(writer);
    }
    writer.WriteVarInt(trusts_.size());
    for (const auto& trust : trusts_)
    {
        trust.Serialize(writer);
    }
    writer.WriteVarString(extra_);
}

void ContractManifest::Deserialize(io::BinaryReader& reader)
{
    name_ = reader.ReadVarString();
    uint64_t count = reader.ReadVarInt();
    supportedStandards_.resize(count);
    for (uint64_t i = 0; i < count; i++)
    {
        supportedStandards_[i] = reader.ReadVarString();
    }
    abi_->Deserialize(reader);
    count = reader.ReadVarInt();
    permissions_.resize(count);
    for (uint64_t i = 0; i < count; i++)
    {
        permissions_[i].Deserialize(reader);
    }
    count = reader.ReadVarInt();
    trusts_.resize(count);
    for (uint64_t i = 0; i < count; i++)
    {
        trusts_[i].Deserialize(reader);
    }
    extra_ = reader.ReadVarString();
}
}  // namespace neo::smartcontract::manifest