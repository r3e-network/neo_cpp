/**
 * @file name_service_records.cpp
 * @brief Service implementations
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/name_service.h>

#include <regex>
#include <sstream>

namespace neo::smartcontract::native
{
// ValidateRecordType is implemented in name_service.cpp

std::string NameService::GetRecord(std::shared_ptr<persistence::DataCache> snapshot, const std::string& name,
                                   const std::string& type) const
{
    if (!ValidateName(name)) throw std::runtime_error("Invalid name");

    if (!ValidateRecordType(type)) throw std::runtime_error("Invalid record type");

    auto [owner, expiration] = GetName(snapshot, name);
    uint32_t currentHeight = snapshot->GetCurrentBlockIndex();
    if (expiration <= currentHeight) throw std::runtime_error("Name expired");

    auto key = CreateStorageKey(
        PREFIX_RECORD,
        io::ByteVector(reinterpret_cast<const uint8_t*>((name + "." + type).data()), (name + "." + type).size()));
    auto value = GetStorageValue(snapshot, key.GetKey());
    if (value.IsEmpty()) return "";

    return std::string(reinterpret_cast<const char*>(value.Data()), value.Size());
}

void NameService::SetRecord(std::shared_ptr<persistence::DataCache> snapshot, const std::string& name,
                            const std::string& type, const std::string& data)
{
    if (!ValidateName(name)) throw std::runtime_error("Invalid name");

    if (!ValidateRecordType(type)) throw std::runtime_error("Invalid record type");

    if (data.size() > MAX_RECORD_SIZE) throw std::runtime_error("Record data too large");

    auto [owner, expiration] = GetName(snapshot, name);
    uint32_t currentHeight = snapshot->GetCurrentBlockIndex();
    if (expiration <= currentHeight) throw std::runtime_error("Name expired");

    auto key = CreateStorageKey(
        PREFIX_RECORD,
        io::ByteVector(reinterpret_cast<const uint8_t*>((name + "." + type).data()), (name + "." + type).size()));
    io::ByteVector value(reinterpret_cast<const uint8_t*>(data.data()), data.size());
    PutStorageValue(snapshot, key.GetKey(), value);
}

void NameService::DeleteRecord(std::shared_ptr<persistence::DataCache> snapshot, const std::string& name,
                               const std::string& type)
{
    if (!ValidateName(name)) throw std::runtime_error("Invalid name");

    if (!ValidateRecordType(type)) throw std::runtime_error("Invalid record type");

    auto [owner, expiration] = GetName(snapshot, name);
    uint32_t currentHeight = snapshot->GetCurrentBlockIndex();
    if (expiration <= currentHeight) throw std::runtime_error("Name expired");

    auto key = CreateStorageKey(
        PREFIX_RECORD,
        io::ByteVector(reinterpret_cast<const uint8_t*>((name + "." + type).data()), (name + "." + type).size()));
    DeleteStorageValue(snapshot, key.GetKey());
}

std::shared_ptr<vm::StackItem> NameService::OnGetRecord(ApplicationEngine& engine,
                                                        const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.size() < 2) throw std::runtime_error("Invalid arguments");

    auto nameItem = args[0];
    auto typeItem = args[1];

    auto name = nameItem->GetString();
    auto type = typeItem->GetString();

    // Check if name and type are valid
    if (!ValidateName(name) || !ValidateRecordType(type)) return vm::StackItem::Create(nullptr);

    try
    {
        // Get record
        auto value = GetRecord(engine.GetSnapshot(), name, type);
        return vm::StackItem::Create(value);
    }
    catch (const std::exception&)
    {
        return vm::StackItem::Create(nullptr);
    }
}

std::shared_ptr<vm::StackItem> NameService::OnSetRecord(ApplicationEngine& engine,
                                                        const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.size() < 3) throw std::runtime_error("Invalid arguments");

    auto nameItem = args[0];
    auto typeItem = args[1];
    auto valueItem = args[2];

    auto name = nameItem->GetString();
    auto type = typeItem->GetString();
    auto value = valueItem->GetString();

    // Check if name is valid
    if (!ValidateName(name)) throw std::runtime_error("Invalid name");

    // Check if type is valid
    if (!ValidateRecordType(type)) throw std::runtime_error("Invalid record type");

    // Check if value is valid
    if (value.length() > MAX_RECORD_SIZE) throw std::runtime_error("Record value too long");

    // Get name
    auto [owner, expiration] = GetName(engine.GetSnapshot(), name);

    // Check if name is expired
    if (expiration <= engine.GetSnapshot()->GetCurrentBlockIndex()) throw std::runtime_error("Name expired");

    // Check if the caller is the owner
    auto caller = engine.GetCurrentScriptHash();
    if (caller != owner) throw std::runtime_error("Not the owner");

    // Set record
    SetRecord(engine.GetSnapshot(), name, type, value);

    // Send notification
    std::vector<std::shared_ptr<vm::StackItem>> state = {vm::StackItem::Create(name), vm::StackItem::Create(type),
                                                         vm::StackItem::Create(value)};
    engine.Notify(GetScriptHash(), "SetRecord", state);

    return vm::StackItem::Create(true);
}

std::shared_ptr<vm::StackItem> NameService::OnDeleteRecord(ApplicationEngine& engine,
                                                           const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.size() < 2) throw std::runtime_error("Invalid arguments");

    auto nameItem = args[0];
    auto typeItem = args[1];

    auto name = nameItem->GetString();
    auto type = typeItem->GetString();

    // Check if name is valid
    if (!ValidateName(name)) throw std::runtime_error("Invalid name");

    // Check if type is valid
    if (!ValidateRecordType(type)) throw std::runtime_error("Invalid record type");

    // Get name
    auto [owner, expiration] = GetName(engine.GetSnapshot(), name);

    // Check if name is expired
    if (expiration <= engine.GetSnapshot()->GetCurrentBlockIndex()) throw std::runtime_error("Name expired");

    // Check if the caller is the owner
    auto caller = engine.GetCurrentScriptHash();
    if (caller != owner) throw std::runtime_error("Not the owner");

    try
    {
        // Delete record
        DeleteRecord(engine.GetSnapshot(), name, type);

        // Send notification
        std::vector<std::shared_ptr<vm::StackItem>> state = {vm::StackItem::Create(name), vm::StackItem::Create(type)};
        engine.Notify(GetScriptHash(), "DeleteRecord", state);

        return vm::StackItem::Create(true);
    }
    catch (const std::exception&)
    {
        return vm::StackItem::Create(false);
    }
}
}  // namespace neo::smartcontract::native
