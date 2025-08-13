/**
 * @file name_service_operations.cpp
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
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/name_service.h>

#include <algorithm>
#include <sstream>

namespace neo::smartcontract::native
{
std::shared_ptr<vm::StackItem> NameService::OnRegister(ApplicationEngine& engine,
                                                       const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.size() < 2) throw std::runtime_error("Invalid arguments");

    auto nameItem = args[0];
    auto ownerItem = args[1];

    auto name = nameItem->GetString();
    auto ownerBytes = ownerItem->GetByteArray();

    if (ownerBytes.Size() != 20) throw std::runtime_error("Invalid owner");

    io::UInt160 owner;
    std::memcpy(owner.Data(), ownerBytes.Data(), 20);

    // Check if name is available
    if (!IsAvailable(engine.GetSnapshot(), name)) throw std::runtime_error("Name is not available");

    // Check if the caller has enough GAS
    auto price = GetPrice(engine.GetSnapshot());
    auto gasToken = GasToken::GetInstance();
    auto caller = engine.GetCurrentScriptHash();
    auto gasBalance = gasToken->GetBalance(engine.GetSnapshot(), caller);
    if (gasBalance < price) throw std::runtime_error("Insufficient GAS");

    // Transfer GAS to the name service contract
    if (!gasToken->Transfer(engine.GetSnapshot(), caller, GetScriptHash(), price))
        throw std::runtime_error("Failed to transfer GAS");

    // Register name
    auto expiration = engine.GetSnapshot()->GetCurrentBlockIndex() + REGISTRATION_DURATION;
    std::ostringstream stream;
    io::BinaryWriter writer(stream);
    writer.Write(owner);
    writer.Write(expiration);
    std::string data = stream.str();

    auto key = GetStorageKey(PREFIX_NAME, name);
    io::ByteVector value(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    PutStorageValue(engine.GetSnapshot(), key, value);

    // Send notification
    std::vector<std::shared_ptr<vm::StackItem>> notificationArgs;
    notificationArgs.push_back(vm::StackItem::Create(name));
    notificationArgs.push_back(vm::StackItem::Create(io::ByteVector(io::ByteSpan(owner.Data(), io::UInt160::Size))));
    notificationArgs.push_back(vm::StackItem::Create(static_cast<int64_t>(expiration)));
    engine.Notify(GetScriptHash(), "Register", notificationArgs);

    return vm::StackItem::Create(true);
}

std::shared_ptr<vm::StackItem> NameService::OnRenew(ApplicationEngine& engine,
                                                    const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.empty()) throw std::runtime_error("Invalid arguments");

    auto nameItem = args[0];
    auto name = nameItem->GetString();

    // Check if name is valid
    if (!ValidateName(name)) throw std::runtime_error("Invalid name");

    // Get name
    auto [owner, expiration] = GetName(engine.GetSnapshot(), name);

    // Check if the caller has enough GAS
    auto price = GetPrice(engine.GetSnapshot());
    auto gasToken = GasToken::GetInstance();
    auto caller = engine.GetCurrentScriptHash();
    auto gasBalance = gasToken->GetBalance(engine.GetSnapshot(), caller);
    if (gasBalance < price) throw std::runtime_error("Insufficient GAS");

    // Transfer GAS to the name service contract
    if (!gasToken->Transfer(engine.GetSnapshot(), caller, GetScriptHash(), price))
        throw std::runtime_error("Failed to transfer GAS");

    // Renew name
    auto newExpiration = std::max(expiration, static_cast<uint64_t>(engine.GetSnapshot()->GetCurrentBlockIndex())) +
                         REGISTRATION_DURATION;
    std::ostringstream stream;
    io::BinaryWriter writer(stream);
    writer.Write(owner);
    writer.Write(newExpiration);
    std::string data = stream.str();

    auto key = GetStorageKey(PREFIX_NAME, name);
    io::ByteVector value(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    PutStorageValue(engine.GetSnapshot(), key, value);

    // Send notification
    std::vector<std::shared_ptr<vm::StackItem>> notificationArgs;
    notificationArgs.push_back(vm::StackItem::Create(name));
    notificationArgs.push_back(vm::StackItem::Create(io::ByteVector(owner.Data(), io::UInt160::Size)));
    notificationArgs.push_back(vm::StackItem::Create(static_cast<int64_t>(newExpiration)));
    engine.Notify(GetScriptHash(), "Renew", notificationArgs);

    return vm::StackItem::Create(true);
}

std::shared_ptr<vm::StackItem> NameService::OnTransfer(ApplicationEngine& engine,
                                                       const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.size() < 2) throw std::runtime_error("Invalid arguments");

    auto nameItem = args[0];
    auto toItem = args[1];

    auto name = nameItem->GetString();
    auto toBytes = toItem->GetByteArray();

    // Check if name is valid
    if (!ValidateName(name)) throw std::runtime_error("Invalid name");

    if (toBytes.Size() != 20) throw std::runtime_error("Invalid to");

    io::UInt160 to;
    std::memcpy(to.Data(), toBytes.Data(), 20);

    // Get name
    auto [owner, expiration] = GetName(engine.GetSnapshot(), name);

    // Check if name is expired
    if (expiration <= engine.GetSnapshot()->GetCurrentBlockIndex()) throw std::runtime_error("Name expired");

    // Check if the caller is the owner
    auto caller = engine.GetCurrentScriptHash();
    if (caller != owner) throw std::runtime_error("Not the owner");

    // Check if the owner is different from the new owner
    if (owner == to) throw std::runtime_error("Owner is already the new owner");

    // Transfer name
    std::ostringstream stream;
    io::BinaryWriter writer(stream);
    writer.Write(to);
    writer.Write(expiration);
    std::string data = stream.str();

    auto key = GetStorageKey(PREFIX_NAME, name);
    io::ByteVector value(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    PutStorageValue(engine.GetSnapshot(), key, value);

    // Send notification
    std::vector<std::shared_ptr<vm::StackItem>> notificationArgs;
    notificationArgs.push_back(vm::StackItem::Create(name));
    notificationArgs.push_back(vm::StackItem::Create(io::ByteVector(owner.Data(), 20)));
    notificationArgs.push_back(vm::StackItem::Create(io::ByteVector(to.Data(), 20)));
    engine.Notify(GetScriptHash(), "Transfer", notificationArgs);

    return vm::StackItem::Create(true);
}

std::shared_ptr<vm::StackItem> NameService::OnDelete(ApplicationEngine& engine,
                                                     const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.empty()) throw std::runtime_error("Invalid arguments");

    auto nameItem = args[0];
    auto name = nameItem->GetString();

    // Check if name is valid
    if (!ValidateName(name)) throw std::runtime_error("Invalid name");

    // Get name
    auto [owner, expiration] = GetName(engine.GetSnapshot(), name);

    // Check if the caller is the owner
    auto caller = engine.GetCurrentScriptHash();
    if (caller != owner) throw std::runtime_error("Not the owner");

    // Delete name
    auto key = GetStorageKey(PREFIX_NAME, name);
    DeleteStorageValue(engine.GetSnapshot(), key);

    // Send notification
    std::vector<std::shared_ptr<vm::StackItem>> notificationArgs;
    notificationArgs.push_back(vm::StackItem::Create(name));
    notificationArgs.push_back(vm::StackItem::Create(io::ByteVector(owner.Data(), 20)));
    engine.Notify(GetScriptHash(), "Delete", notificationArgs);

    return vm::StackItem::Create(true);
}

std::shared_ptr<vm::StackItem> NameService::OnResolve(ApplicationEngine& engine,
                                                      const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.empty()) throw std::runtime_error("Invalid arguments");

    auto nameItem = args[0];
    auto name = nameItem->GetString();

    try
    {
        // Get name
        auto [owner, expiration] = GetName(engine.GetSnapshot(), name);

        // Check if name is expired
        if (expiration <= engine.GetSnapshot()->GetCurrentBlockIndex()) return vm::StackItem::Create(nullptr);

        return vm::StackItem::Create(io::ByteVector(owner.Data(), 20));
    }
    catch (const std::exception&)
    {
        return vm::StackItem::Create(nullptr);
    }
}

std::shared_ptr<vm::StackItem> NameService::OnGetOwner(ApplicationEngine& engine,
                                                       const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.empty()) throw std::runtime_error("Invalid arguments");

    auto nameItem = args[0];
    auto name = nameItem->GetString();

    try
    {
        // Get name
        auto [owner, expiration] = GetName(engine.GetSnapshot(), name);

        // Check if name is expired
        if (expiration <= engine.GetSnapshot()->GetCurrentBlockIndex()) return vm::StackItem::Create(nullptr);

        return vm::StackItem::Create(io::ByteVector(owner.Data(), 20));
    }
    catch (const std::exception&)
    {
        return vm::StackItem::Create(nullptr);
    }
}

std::shared_ptr<vm::StackItem> NameService::OnGetExpiration(ApplicationEngine& engine,
                                                            const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.empty()) throw std::runtime_error("Invalid arguments");

    auto nameItem = args[0];
    auto name = nameItem->GetString();

    // Check if name is valid
    if (!ValidateName(name)) return vm::StackItem::Create(nullptr);

    try
    {
        // Get name
        auto [owner, expiration] = GetName(engine.GetSnapshot(), name);

        return vm::StackItem::Create(static_cast<int64_t>(expiration));
    }
    catch (const std::exception&)
    {
        return vm::StackItem::Create(nullptr);
    }
}
}  // namespace neo::smartcontract::native
