#include <neo/smartcontract/native/name_service.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/storage_item.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <sstream>
#include <regex>

namespace neo::smartcontract::native
{
    // ValidateRecordType is implemented in name_service.cpp

    std::string NameService::GetRecord(std::shared_ptr<persistence::StoreView> snapshot, const std::string& name, const std::string& type) const
    {
        // Check if name is valid
        if (!ValidateName(name))
            throw std::runtime_error("Invalid name");

        // Check if type is valid
        if (!ValidateRecordType(type))
            throw std::runtime_error("Invalid record type");

        // Check if name is registered and not expired
        auto [owner, expiration] = GetName(snapshot, name);
        if (expiration <= snapshot->GetCurrentBlockIndex())
            throw std::runtime_error("Name expired");

        // Get record
        std::string recordKey = name + ":" + type;
        auto key = GetStorageKey(PREFIX_RECORD, recordKey);
        auto value = GetStorageValue(snapshot, key);
        if (value.IsEmpty())
            throw std::runtime_error("Record not found");

        return std::string(reinterpret_cast<const char*>(value.Data()), value.Size());
    }

    void NameService::SetRecord(std::shared_ptr<persistence::StoreView> snapshot, const std::string& name, const std::string& type, const std::string& value)
    {
        // Check if name is valid
        if (!ValidateName(name))
            throw std::runtime_error("Invalid name");

        // Check if type is valid
        if (!ValidateRecordType(type))
            throw std::runtime_error("Invalid record type");

        // Check if value is valid
        if (value.length() > MAX_RECORD_VALUE_LENGTH)
            throw std::runtime_error("Record value too long");

        // Check if name is registered and not expired
        auto [owner, expiration] = GetName(snapshot, name);
        if (expiration <= snapshot->GetCurrentBlockIndex())
            throw std::runtime_error("Name expired");

        // Set record
        std::string recordKey = name + ":" + type;
        auto key = GetStorageKey(PREFIX_RECORD, recordKey);
        io::ByteVector valueBytes(io::ByteSpan(reinterpret_cast<const uint8_t*>(value.data()), value.size()));
        PutStorageValue(snapshot, key, valueBytes);
    }

    void NameService::DeleteRecord(std::shared_ptr<persistence::StoreView> snapshot, const std::string& name, const std::string& type)
    {
        // Check if name is valid
        if (!ValidateName(name))
            throw std::runtime_error("Invalid name");

        // Check if type is valid
        if (!ValidateRecordType(type))
            throw std::runtime_error("Invalid record type");

        // Check if name is registered and not expired
        auto [owner, expiration] = GetName(snapshot, name);
        if (expiration <= snapshot->GetCurrentBlockIndex())
            throw std::runtime_error("Name expired");

        // Delete record
        std::string recordKey = name + ":" + type;
        auto key = GetStorageKey(PREFIX_RECORD, recordKey);
        DeleteStorageValue(snapshot, key);
    }

    std::shared_ptr<vm::StackItem> NameService::OnGetRecord(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.size() < 2)
            throw std::runtime_error("Invalid arguments");

        auto nameItem = args[0];
        auto typeItem = args[1];

        auto name = nameItem->GetString();
        auto type = typeItem->GetString();

        // Check if name and type are valid
        if (!ValidateName(name) || !ValidateRecordType(type))
            return vm::StackItem::Create(nullptr);

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

    std::shared_ptr<vm::StackItem> NameService::OnSetRecord(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.size() < 3)
            throw std::runtime_error("Invalid arguments");

        auto nameItem = args[0];
        auto typeItem = args[1];
        auto valueItem = args[2];

        auto name = nameItem->GetString();
        auto type = typeItem->GetString();
        auto value = valueItem->GetString();

        // Check if name is valid
        if (!ValidateName(name))
            throw std::runtime_error("Invalid name");

        // Check if type is valid
        if (!ValidateRecordType(type))
            throw std::runtime_error("Invalid record type");

        // Check if value is valid
        if (value.length() > MAX_RECORD_VALUE_LENGTH)
            throw std::runtime_error("Record value too long");

        // Get name
        auto [owner, expiration] = GetName(engine.GetSnapshot(), name);

        // Check if name is expired
        if (expiration <= engine.GetSnapshot()->GetCurrentBlockIndex())
            throw std::runtime_error("Name expired");

        // Check if the caller is the owner
        auto caller = engine.GetCurrentScriptHash();
        if (caller != owner)
            throw std::runtime_error("Not the owner");

        // Set record
        SetRecord(engine.GetSnapshot(), name, type, value);

        // Send notification
        std::vector<std::shared_ptr<vm::StackItem>> notificationArgs;
        notificationArgs.push_back(vm::StackItem::Create(name));
        notificationArgs.push_back(vm::StackItem::Create(type));
        notificationArgs.push_back(vm::StackItem::Create(value));
        engine.SendNotification(GetScriptHash(), "SetRecord", vm::StackItem::Create(notificationArgs));

        return vm::StackItem::Create(true);
    }

    std::shared_ptr<vm::StackItem> NameService::OnDeleteRecord(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.size() < 2)
            throw std::runtime_error("Invalid arguments");

        auto nameItem = args[0];
        auto typeItem = args[1];

        auto name = nameItem->GetString();
        auto type = typeItem->GetString();

        // Check if name is valid
        if (!ValidateName(name))
            throw std::runtime_error("Invalid name");

        // Check if type is valid
        if (!ValidateRecordType(type))
            throw std::runtime_error("Invalid record type");

        // Get name
        auto [owner, expiration] = GetName(engine.GetSnapshot(), name);

        // Check if name is expired
        if (expiration <= engine.GetSnapshot()->GetCurrentBlockIndex())
            throw std::runtime_error("Name expired");

        // Check if the caller is the owner
        auto caller = engine.GetCurrentScriptHash();
        if (caller != owner)
            throw std::runtime_error("Not the owner");

        try
        {
            // Delete record
            DeleteRecord(engine.GetSnapshot(), name, type);

            // Send notification
            std::vector<std::shared_ptr<vm::StackItem>> notificationArgs;
            notificationArgs.push_back(vm::StackItem::Create(name));
            notificationArgs.push_back(vm::StackItem::Create(type));
            engine.SendNotification(GetScriptHash(), "DeleteRecord", vm::StackItem::Create(notificationArgs));

            return vm::StackItem::Create(true);
        }
        catch (const std::exception&)
        {
            return vm::StackItem::Create(false);
        }
    }
}
