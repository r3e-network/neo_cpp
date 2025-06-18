#include <neo/smartcontract/manifest/contract_manifest.h>
#include <neo/smartcontract/manifest/contract_abi.h>
#include <neo/smartcontract/manifest/contract_permission.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/json/json.h>
#include <sstream>
#include <set>

namespace neo::smartcontract::manifest
{
    ContractManifest::ContractManifest()
        : abi_(new ContractAbi())
    {
    }

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
        // Implement validation matching C# ContractManifest.IsValid implementation
        try
        {
            // Check if the manifest can be serialized properly
            auto stackItem = ToStackItem();
            if (!stackItem)
                return false;

            // Try to serialize the stack item to ensure it's valid
            // This matches the C# BinarySerializer.Serialize check
            auto serialized = stackItem->Serialize();
            if (serialized.empty())
                return false;

            // Check that all groups are valid
            for (const auto& group : groups_)
            {
                if (!group.IsValid(hash))
                    return false;
            }

            // Validate name is not empty
            if (name_.empty())
                return false;

            // Validate supported standards don't contain empty strings
            for (const auto& standard : supportedStandards_)
            {
                if (standard.empty())
                    return false;
            }

            // Validate ABI if present
            if (abi_ && !abi_->IsValid())
                return false;

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

    bool ContractManifest::IsValid() const
    {
        // Implement validation matching C# ContractManifest.IsValid
        try
        {
            // Check that name is not empty
            if (name_.empty())
                return false;
                
            // Check that groups are valid (no duplicates)
            std::set<std::string> groupHashes;
            for (const auto& group : groups_)
            {
                auto groupStr = group.ToString();
                if (groupHashes.count(groupStr) > 0)
                    return false; // Duplicate group
                groupHashes.insert(groupStr);
            }
            
            // Check that supported standards are valid
            for (const auto& standard : supportedStandards_)
            {
                if (standard.empty())
                    return false; // Empty standard name
            }
            
            // Check that ABI is valid
            if (!abi_->IsValid())
                return false;
                
            // Check that permissions are valid
            for (const auto& permission : permissions_)
            {
                if (!permission.IsValid())
                    return false;
            }
            
            // Check that trusts are valid
            for (const auto& trust : trusts_)
            {
                if (!trust.IsValid())
                    return false;
            }
            
            // Check that extra data is valid JSON (if present)
            if (extra_.has_value())
            {
                try
                {
                    // Try to parse as JSON to validate
                    nlohmann::json::parse(extra_.value());
                }
                catch (...)
                {
                    return false; // Invalid JSON
                }
            }
            
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    ContractManifest ContractManifest::Parse(const std::string& json)
    {
        if (json.size() > MaxLength)
            throw std::runtime_error("Manifest is too large");

        // Parse JSON
        json::JSON jsonObj = json::JSON::Load(json);
        
        ContractManifest manifest;
        manifest.SetName(jsonObj["name"].ToString());
        
        // Parse supported standards
        auto& supportedStandards = jsonObj["supportedstandards"];
        std::vector<std::string> standards;
        for (size_t i = 0; i < supportedStandards.length(); i++)
        {
            standards.push_back(supportedStandards[i].ToString());
        }
        manifest.SetSupportedStandards(standards);
        
        // Parse ABI
        auto& abi = jsonObj["abi"];
        ContractAbi contractAbi;
        
        // Parse methods
        auto& methods = abi["methods"];
        std::vector<ContractMethodDescriptor> methodDescriptors;
        for (size_t i = 0; i < methods.length(); i++)
        {
            auto& method = methods[i];
            ContractMethodDescriptor descriptor;
            descriptor.SetName(method["name"].ToString());
            descriptor.SetReturnType(static_cast<ContractParameterType>(method["returntype"].ToInt()));
            descriptor.SetOffset(method["offset"].ToInt());
            descriptor.SetSafe(method["safe"].ToBool());
            
            // Parse parameters
            auto& parameters = method["parameters"];
            std::vector<ContractParameterDefinition> parameterDefinitions;
            for (size_t j = 0; j < parameters.length(); j++)
            {
                auto& parameter = parameters[j];
                ContractParameterDefinition definition;
                definition.SetName(parameter["name"].ToString());
                definition.SetType(static_cast<ContractParameterType>(parameter["type"].ToInt()));
                parameterDefinitions.push_back(definition);
            }
            descriptor.SetParameters(parameterDefinitions);
            
            methodDescriptors.push_back(descriptor);
        }
        contractAbi.SetMethods(methodDescriptors);
        
        // Parse events
        auto& events = abi["events"];
        std::vector<ContractEventDescriptor> eventDescriptors;
        for (size_t i = 0; i < events.length(); i++)
        {
            auto& event = events[i];
            ContractEventDescriptor descriptor;
            descriptor.SetName(event["name"].ToString());
            
            // Parse parameters
            auto& parameters = event["parameters"];
            std::vector<ContractParameterDefinition> parameterDefinitions;
            for (size_t j = 0; j < parameters.length(); j++)
            {
                auto& parameter = parameters[j];
                ContractParameterDefinition definition;
                definition.SetName(parameter["name"].ToString());
                definition.SetType(static_cast<ContractParameterType>(parameter["type"].ToInt()));
                parameterDefinitions.push_back(definition);
            }
            descriptor.SetParameters(parameterDefinitions);
            
            eventDescriptors.push_back(descriptor);
        }
        contractAbi.SetEvents(eventDescriptors);
        
        manifest.SetAbi(contractAbi);
        
        // Parse permissions
        auto& permissions = jsonObj["permissions"];
        std::vector<ContractPermission> contractPermissions;
        for (size_t i = 0; i < permissions.length(); i++)
        {
            auto& permission = permissions[i];
            ContractPermission contractPermission;
            
            // Parse contract descriptor properly
            auto& contract = permission["contract"];
            if (contract.ToString() == "*")
            {
                contractPermission.SetContract(ContractPermissionDescriptor::CreateWildcard());
            }
            else if (contract.IsString())
            {
                std::string contractStr = contract.ToString();
                if (contractStr.length() == 40) // Hash160 length in hex
                {
                    // Parse as contract hash
                    auto contractHash = io::UInt160::Parse(contractStr);
                    contractPermission.SetContract(ContractPermissionDescriptor::CreateByHash(contractHash));
                }
                else if (contractStr.length() == 66) // Public key length in hex (33 bytes * 2)
                {
                    // Parse as group public key
                    auto groupPubKey = cryptography::ecc::ECPoint::Parse(contractStr);
                    contractPermission.SetContract(ContractPermissionDescriptor::CreateByGroup(groupPubKey));
                }
                else
                {
                    // Invalid format, default to wildcard
                    contractPermission.SetContract(ContractPermissionDescriptor::CreateWildcard());
                }
            }
            else
            {
                // Invalid type, default to wildcard
                contractPermission.SetContract(ContractPermissionDescriptor::CreateWildcard());
            }
            
            // Parse methods
            auto& methods = permission["methods"];
            if (methods.IsArray() && methods.length() > 0)
            {
                std::vector<std::string> methodNames;
                for (size_t j = 0; j < methods.length(); j++)
                {
                    methodNames.push_back(methods[j].ToString());
                }
                contractPermission.SetMethods(methodNames);
            }
            else if (methods.ToString() == "*")
            {
                contractPermission.SetMethodsWildcard(true);
            }
            
            contractPermissions.push_back(contractPermission);
        }
        manifest.SetPermissions(contractPermissions);
        
        // Parse trusts
        auto& trusts = jsonObj["trusts"];
        std::vector<ContractPermissionDescriptor> trustDescriptors;
        if (trusts.IsArray())
        {
            for (size_t i = 0; i < trusts.length(); i++)
            {
                auto& trust = trusts[i];
                if (trust.ToString() == "*")
                {
                    trustDescriptors.push_back(ContractPermissionDescriptor::CreateWildcard());
                }
                else if (trust.IsString())
                {
                    std::string trustStr = trust.ToString();
                    if (trustStr.length() == 40) // Hash160 length in hex
                    {
                        // Parse as contract hash
                        auto trustHash = io::UInt160::Parse(trustStr);
                        trustDescriptors.push_back(ContractPermissionDescriptor::CreateByHash(trustHash));
                    }
                    else if (trustStr.length() == 66) // Public key length in hex (33 bytes * 2)
                    {
                        // Parse as group public key
                        auto groupPubKey = cryptography::ecc::ECPoint::Parse(trustStr);
                        trustDescriptors.push_back(ContractPermissionDescriptor::CreateByGroup(groupPubKey));
                    }
                    else
                    {
                        // Invalid format, default to wildcard
                        trustDescriptors.push_back(ContractPermissionDescriptor::CreateWildcard());
                    }
                }
                else
                {
                    // Invalid type, default to wildcard
                    trustDescriptors.push_back(ContractPermissionDescriptor::CreateWildcard());
                }
            }
        }
        else if (trusts.ToString() == "*")
        {
            trustDescriptors.push_back(ContractPermissionDescriptor::CreateWildcard());
        }
        manifest.SetTrusts(trustDescriptors);
        
        // Parse extra
        if (jsonObj.hasKey("extra") && !jsonObj["extra"].IsNull())
        {
            manifest.SetExtra(jsonObj["extra"].dump());
        }
        
        return manifest;
    }

    std::string ContractManifest::ToJson() const
    {
        json::JSON jsonObj;
        jsonObj["name"] = name_;
        
        // Serialize supported standards
        json::JSON supportedStandards = json::Array();
        for (const auto& standard : supportedStandards_)
        {
            supportedStandards.append(standard);
        }
        jsonObj["supportedstandards"] = supportedStandards;
        
        // Serialize ABI
        json::JSON abi;
        
        // Serialize methods
        json::JSON methods = json::Array();
        for (const auto& method : abi_->GetMethods())
        {
            json::JSON methodObj;
            methodObj["name"] = method.GetName();
            methodObj["returntype"] = static_cast<int>(method.GetReturnType());
            methodObj["offset"] = method.GetOffset();
            methodObj["safe"] = method.IsSafe();
            
            // Serialize parameters
            json::JSON parameters = json::Array();
            for (const auto& parameter : method.GetParameters())
            {
                json::JSON parameterObj;
                parameterObj["name"] = parameter.GetName();
                parameterObj["type"] = static_cast<int>(parameter.GetType());
                parameters.append(parameterObj);
            }
            methodObj["parameters"] = parameters;
            
            methods.append(methodObj);
        }
        abi["methods"] = methods;
        
        // Serialize events
        json::JSON events = json::Array();
        for (const auto& event : abi_->GetEvents())
        {
            json::JSON eventObj;
            eventObj["name"] = event.GetName();
            
            // Serialize parameters
            json::JSON parameters = json::Array();
            for (const auto& parameter : event.GetParameters())
            {
                json::JSON parameterObj;
                parameterObj["name"] = parameter.GetName();
                parameterObj["type"] = static_cast<int>(parameter.GetType());
                parameters.append(parameterObj);
            }
            eventObj["parameters"] = parameters;
            
            events.append(eventObj);
        }
        abi["events"] = events;
        
        jsonObj["abi"] = abi;
        
        // Serialize permissions
        json::JSON permissions = json::Array();
        for (const auto& permission : permissions_)
        {
            json::JSON permissionObj;
            
            // Serialize contract
            if (permission.GetContract().IsWildcard())
            {
                permissionObj["contract"] = "*";
            }
            else if (permission.GetContract().IsHash())
            {
                permissionObj["contract"] = permission.GetContract().GetHash().ToString();
            }
            else if (permission.GetContract().IsGroup())
            {
                permissionObj["contract"] = permission.GetContract().GetGroup().ToString();
            }
            
            // Serialize methods
            if (permission.IsMethodsWildcard())
            {
                permissionObj["methods"] = "*";
            }
            else
            {
                json::JSON methods = json::Array();
                for (const auto& method : permission.GetMethods())
                {
                    methods.append(method);
                }
                permissionObj["methods"] = methods;
            }
            
            permissions.append(permissionObj);
        }
        jsonObj["permissions"] = permissions;
        
        // Serialize trusts
        json::JSON trusts;
        if (trusts_.size() == 1 && trusts_[0].IsWildcard())
        {
            trusts = "*";
        }
        else
        {
            trusts = json::Array();
            for (const auto& trust : trusts_)
            {
                if (trust.IsWildcard())
                {
                    trusts.append("*");
                }
                else if (trust.IsHash())
                {
                    trusts.append(trust.GetHash().ToString());
                }
                else if (trust.IsGroup())
                {
                    trusts.append(trust.GetGroup().ToString());
                }
            }
        }
        jsonObj["trusts"] = trusts;
        
        // Serialize extra
        if (!extra_.empty())
        {
            jsonObj["extra"] = json::JSON::Load(extra_);
        }
        else
        {
            jsonObj["extra"] = nullptr;
        }
        
        // Serialize features (empty object for compatibility)
        jsonObj["features"] = json::Object();
        
        // Serialize groups (empty array for compatibility)
        jsonObj["groups"] = json::Array();
        
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
}
