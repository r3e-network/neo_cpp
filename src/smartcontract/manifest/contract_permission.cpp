#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/smartcontract/manifest/contract_permission.h>

namespace neo::smartcontract::manifest
{
// ContractPermissionDescriptor implementation
ContractPermissionDescriptor::ContractPermissionDescriptor() : isWildcard_(true) {}

ContractPermissionDescriptor::ContractPermissionDescriptor(const io::UInt160& hash) : hash_(hash), isWildcard_(false) {}

ContractPermissionDescriptor::ContractPermissionDescriptor(const cryptography::ecc::ECPoint& group)
    : group_(group), isWildcard_(false)
{
}

const io::UInt160& ContractPermissionDescriptor::GetHash() const { return hash_; }

const cryptography::ecc::ECPoint& ContractPermissionDescriptor::GetGroup() const { return group_; }

bool ContractPermissionDescriptor::IsHash() const { return !isWildcard_ && !hash_.IsZero(); }

bool ContractPermissionDescriptor::IsGroup() const { return !isWildcard_ && !group_.IsInfinity(); }

bool ContractPermissionDescriptor::IsWildcard() const { return isWildcard_; }

ContractPermissionDescriptor ContractPermissionDescriptor::CreateWildcard() { return ContractPermissionDescriptor(); }

ContractPermissionDescriptor ContractPermissionDescriptor::Create(const io::UInt160& hash)
{
    return ContractPermissionDescriptor(hash);
}

ContractPermissionDescriptor ContractPermissionDescriptor::Create(const cryptography::ecc::ECPoint& group)
{
    return ContractPermissionDescriptor(group);
}

void ContractPermissionDescriptor::Serialize(io::BinaryWriter& writer) const
{
    if (isWildcard_)
    {
        writer.Write(static_cast<uint8_t>(0));
    }
    else if (IsHash())
    {
        writer.Write(static_cast<uint8_t>(1));
        writer.Write(hash_);
    }
    else if (IsGroup())
    {
        writer.Write(static_cast<uint8_t>(2));
        writer.Write(group_);
    }
    else
    {
        writer.Write(static_cast<uint8_t>(0));
    }
}

void ContractPermissionDescriptor::Deserialize(io::BinaryReader& reader)
{
    uint8_t type = reader.ReadUInt8();
    switch (type)
    {
        case 0:
            isWildcard_ = true;
            hash_ = io::UInt160();
            group_ = cryptography::ecc::ECPoint::Infinity();
            break;
        case 1:
            isWildcard_ = false;
            hash_ = reader.ReadSerializable<io::UInt160>();
            group_ = cryptography::ecc::ECPoint::Infinity();
            break;
        case 2:
            isWildcard_ = false;
            hash_ = io::UInt160();
            group_ = reader.ReadSerializable<cryptography::ecc::ECPoint>();
            break;
        default:
            throw std::runtime_error("Invalid contract permission descriptor type");
    }
}

// ContractPermission implementation
ContractPermission::ContractPermission() : isMethodsWildcard_(true) {}

const ContractPermissionDescriptor& ContractPermission::GetContract() const { return contract_; }

void ContractPermission::SetContract(const ContractPermissionDescriptor& contract) { contract_ = contract; }

const std::vector<std::string>& ContractPermission::GetMethods() const { return methods_; }

void ContractPermission::SetMethods(const std::vector<std::string>& methods)
{
    methods_ = methods;
    isMethodsWildcard_ = false;
}

bool ContractPermission::IsMethodsWildcard() const { return isMethodsWildcard_; }

void ContractPermission::SetMethodsWildcard(bool isWildcard)
{
    isMethodsWildcard_ = isWildcard;
    if (isWildcard)
    {
        methods_.clear();
    }
}

ContractPermission ContractPermission::CreateDefault()
{
    ContractPermission permission;
    permission.SetContract(ContractPermissionDescriptor::CreateWildcard());
    permission.SetMethodsWildcard(true);
    return permission;
}

void ContractPermission::Serialize(io::BinaryWriter& writer) const
{
    contract_.Serialize(writer);
    if (isMethodsWildcard_)
    {
        writer.Write(static_cast<uint8_t>(0));
    }
    else
    {
        writer.Write(static_cast<uint8_t>(1));
        writer.WriteVarInt(methods_.size());
        for (const auto& method : methods_)
        {
            writer.WriteVarString(method);
        }
    }
}

void ContractPermission::Deserialize(io::BinaryReader& reader)
{
    contract_.Deserialize(reader);
    uint8_t type = reader.ReadUInt8();
    switch (type)
    {
        case 0:
            isMethodsWildcard_ = true;
            methods_.clear();
            break;
        case 1:
        {
            isMethodsWildcard_ = false;
            uint64_t count = reader.ReadVarInt();
            methods_.resize(count);
            for (uint64_t i = 0; i < count; i++)
            {
                methods_[i] = reader.ReadVarString();
            }
            break;
        }
        default:
            throw std::runtime_error("Invalid contract permission methods type");
    }
}
}  // namespace neo::smartcontract::manifest
