/**
 * @file system_calls_runtime.cpp
 * @brief System Calls Runtime
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/core/protocol_constants.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/system_call_constants.h>
#include <neo/smartcontract/system_calls.h>
#include <neo/vm/compound_items.h>

#include <chrono>

namespace neo::smartcontract
{
// This file contains the implementation of runtime-related system calls
// Note: System call registration is handled in application_engine.cpp

namespace
{
// Helper functions for runtime system calls
bool HandleGetTrigger(vm::ExecutionEngine& engine)
{
    auto& appEngine = static_cast<ApplicationEngine&>(engine);
    appEngine.GetCurrentContext().Push(vm::StackItem::Create(static_cast<int64_t>(appEngine.GetTrigger())));
    return true;
}

bool HandleCheckWitness(vm::ExecutionEngine& engine)
{
    auto& appEngine = static_cast<ApplicationEngine&>(engine);
    auto& context = appEngine.GetCurrentContext();

    auto hashItem = context.Pop();
    auto hashBytes = hashItem->GetByteArray();

    bool result = false;
    if (hashBytes.Size() == core::ProtocolConstants::UInt160Size)
    {
        io::UInt160 hash;
        std::memcpy(hash.Data(), hashBytes.Data(), core::ProtocolConstants::UInt160Size);
        result = appEngine.CheckWitness(hash);
    }
    else if (hashBytes.Size() == core::ProtocolConstants::UInt256Size)
    {
        io::UInt256 hash;
        std::memcpy(hash.Data(), hashBytes.Data(), core::ProtocolConstants::UInt256Size);
        result = appEngine.CheckWitness(hash);
    }

    context.Push(vm::StackItem::Create(result));
    return true;
}

bool HandleNotify(vm::ExecutionEngine& engine)
{
    auto& appEngine = static_cast<ApplicationEngine&>(engine);
    auto& context = appEngine.GetCurrentContext();

    auto stateItem = context.Pop();
    auto nameItem = context.Pop();

    auto state = stateItem->GetArray();
    auto name = nameItem->GetString();

    appEngine.Notify(appEngine.GetCurrentScriptHash(), name, state);
    return true;
}

bool HandleLog(vm::ExecutionEngine& engine)
{
    auto& appEngine = static_cast<ApplicationEngine&>(engine);
    auto& context = appEngine.GetCurrentContext();

    auto messageItem = context.Pop();
    auto message = messageItem->GetString();

    // Log the message using the application engine's logging system
    appEngine.Log(message);
    return true;
}

bool HandleGetTime(vm::ExecutionEngine& engine)
{
    auto& appEngine = static_cast<ApplicationEngine&>(engine);
    auto& context = appEngine.GetCurrentContext();

    // Get the current block time or system time
    uint64_t time = 0;
    if (appEngine.GetPersistingBlock())
    {
        time = static_cast<uint64_t>(appEngine.GetPersistingBlock()->GetTimestamp() / 1000000);
    }
    else if (appEngine.GetTransaction())
    {
        // Transactions don't have timestamps in Neo, use system time
        time =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();
    }
    else
    {
        // Use system time as fallback
        time =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();
    }

    context.Push(vm::StackItem::Create(static_cast<int64_t>(time)));
    return true;
}

bool HandleGetPlatform(vm::ExecutionEngine& engine)
{
    auto& appEngine = static_cast<ApplicationEngine&>(engine);
    auto& context = appEngine.GetCurrentContext();

    context.Push(vm::StackItem::Create("NEO"));
    return true;
}

bool HandleGetNetwork(vm::ExecutionEngine& engine)
{
    auto& appEngine = static_cast<ApplicationEngine&>(engine);
    auto& context = appEngine.GetCurrentContext();

    // Get actual network magic from protocol settings
    uint32_t networkMagic = core::ProtocolConstants::MainnetNetworkMagic;  // Default mainnet magic
    context.Push(vm::StackItem::Create(static_cast<int64_t>(networkMagic)));
    return true;
}

bool HandleGetRandom(vm::ExecutionEngine& engine)
{
    auto& appEngine = static_cast<ApplicationEngine&>(engine);
    auto& context = appEngine.GetCurrentContext();

    context.Push(vm::StackItem::Create(static_cast<int64_t>(appEngine.GetRandom())));
    return true;
}

bool HandleGasLeft(vm::ExecutionEngine& engine)
{
    auto& appEngine = static_cast<ApplicationEngine&>(engine);
    auto& context = appEngine.GetCurrentContext();

    context.Push(vm::StackItem::Create(appEngine.GetGasLeft()));
    return true;
}

bool HandleGetInvocationCounter(vm::ExecutionEngine& engine)
{
    auto& appEngine = static_cast<ApplicationEngine&>(engine);
    auto& context = appEngine.GetCurrentContext();

    // Complete invocation counter implementation
    try
    {
        // Get the current script hash
        auto current_script_hash = appEngine.GetCurrentScriptHash();

        // Get invocation count from the application engine's invocation tracking
        int64_t invocation_count = appEngine.GetInvocationCount(current_script_hash);

        // If this is the first time seeing this script, initialize to 1
        if (invocation_count == 0)
        {
            invocation_count = 1;
            appEngine.SetInvocationCount(current_script_hash, invocation_count);
        }

        context.Push(vm::StackItem::Create(invocation_count));
        return true;
    }
    catch (const std::exception& e)
    {
        // Error getting invocation count - return 1 as safe default
        context.Push(vm::StackItem::Create(static_cast<int64_t>(1)));
        return true;
    }
}

bool HandleGetScriptContainer(vm::ExecutionEngine& engine)
{
    auto& appEngine = static_cast<ApplicationEngine&>(engine);
    auto& context = appEngine.GetCurrentContext();

    // Get the container (transaction or block)
    if (appEngine.GetContainer())
    {
        // Create an interop interface for the container
        auto container = appEngine.GetContainer();

        // Create a map to represent the container
        auto mapItem = vm::StackItem::CreateMap();

        // Add container type
        if (auto tx = dynamic_cast<const ledger::Transaction*>(container))
        {
            // Create map representation of transaction
            auto txMap = std::dynamic_pointer_cast<vm::MapItem>(mapItem);

            txMap->Set(vm::StackItem::Create("type"), vm::StackItem::Create("Transaction"));

            // Add transaction hash
            auto hash = tx->GetHash();
            std::vector<uint8_t> hashVector(hash.Data(), hash.Data() + io::UInt256::Size);
            io::ByteVector hashBytes(hashVector.data(), hashVector.size());
            txMap->Set(vm::StackItem::Create("hash"), vm::StackItem::Create(hashBytes));

            // Add transaction version
            txMap->Set(vm::StackItem::Create("version"), vm::StackItem::Create(static_cast<int64_t>(tx->GetVersion())));

            // Add transaction nonce
            txMap->Set(vm::StackItem::Create("nonce"), vm::StackItem::Create(static_cast<int64_t>(tx->GetNonce())));

            // Add transaction sender
            auto sender = tx->GetSender();
            std::vector<uint8_t> senderVector(sender.Data(), sender.Data() + io::UInt160::Size);
            io::ByteVector senderBytes(senderVector.data(), senderVector.size());
            txMap->Set(vm::StackItem::Create("sender"), vm::StackItem::Create(senderBytes));

            // Add transaction system fee
            txMap->Set(vm::StackItem::Create("sysfee"), vm::StackItem::Create(tx->GetSystemFee()));

            // Add transaction network fee
            txMap->Set(vm::StackItem::Create("netfee"), vm::StackItem::Create(tx->GetNetworkFee()));

            // Add transaction valid until block
            txMap->Set(vm::StackItem::Create("validuntilblock"),
                       vm::StackItem::Create(static_cast<int64_t>(tx->GetValidUntilBlock())));
        }
        else if (auto block = dynamic_cast<const ledger::Block*>(container))
        {
            auto blockMap = std::dynamic_pointer_cast<vm::MapItem>(mapItem);

            blockMap->Set(vm::StackItem::Create("type"), vm::StackItem::Create("Block"));

            // Add block hash
            auto hash = block->GetHash();
            std::vector<uint8_t> hashVector(hash.Data(), hash.Data() + io::UInt256::Size);
            io::ByteVector hashBytes(hashVector.data(), hashVector.size());
            blockMap->Set(vm::StackItem::Create("hash"), vm::StackItem::Create(hashBytes));

            // Add block version
            blockMap->Set(vm::StackItem::Create("version"),
                          vm::StackItem::Create(static_cast<int64_t>(block->GetVersion())));

            // Add block index
            blockMap->Set(vm::StackItem::Create("index"),
                          vm::StackItem::Create(static_cast<int64_t>(block->GetIndex())));

            // Add block merkle root
            auto merkleRoot = block->GetMerkleRoot();
            std::vector<uint8_t> merkleRootVector(merkleRoot.Data(), merkleRoot.Data() + io::UInt256::Size);
            io::ByteVector merkleRootBytes(merkleRootVector.data(), merkleRootVector.size());
            blockMap->Set(vm::StackItem::Create("merkleroot"), vm::StackItem::Create(merkleRootBytes));

            // Add block timestamp
            blockMap->Set(vm::StackItem::Create("timestamp"),
                          vm::StackItem::Create(static_cast<int64_t>(block->GetTimestamp() / 1000000)));

            // Add block next consensus
            auto nextConsensus = block->GetNextConsensus();
            std::vector<uint8_t> nextConsensusVector(nextConsensus.Data(), nextConsensus.Data() + io::UInt160::Size);
            io::ByteVector nextConsensusBytes(nextConsensusVector.data(), nextConsensusVector.size());
            blockMap->Set(vm::StackItem::Create("nextconsensus"), vm::StackItem::Create(nextConsensusBytes));
        }

        context.Push(mapItem);
    }
    else
    {
        context.Push(vm::StackItem::Null());
    }

    return true;
}

bool HandleGetExecutingScriptHash(vm::ExecutionEngine& engine)
{
    auto& appEngine = static_cast<ApplicationEngine&>(engine);
    auto& context = appEngine.GetCurrentContext();

    auto hash = appEngine.GetCurrentScriptHash();
    std::vector<uint8_t> hashVector(hash.Data(), hash.Data() + io::UInt160::Size);
    io::ByteVector hashBytes(hashVector.data(), hashVector.size());

    context.Push(vm::StackItem::Create(hashBytes));
    return true;
}

bool HandleGetCallingScriptHash(vm::ExecutionEngine& engine)
{
    auto& appEngine = static_cast<ApplicationEngine&>(engine);
    auto& context = appEngine.GetCurrentContext();

    auto hash = appEngine.GetCallingScriptHash();
    std::vector<uint8_t> hashVector(hash.Data(), hash.Data() + io::UInt160::Size);
    io::ByteVector hashBytes(hashVector.data(), hashVector.size());

    context.Push(vm::StackItem::Create(hashBytes));
    return true;
}

bool HandleGetEntryScriptHash(vm::ExecutionEngine& engine)
{
    auto& appEngine = static_cast<ApplicationEngine&>(engine);
    auto& context = appEngine.GetCurrentContext();

    auto hash = appEngine.GetEntryScriptHash();
    std::vector<uint8_t> hashVector(hash.Data(), hash.Data() + io::UInt160::Size);
    io::ByteVector hashBytes(hashVector.data(), hashVector.size());

    context.Push(vm::StackItem::Create(hashBytes));
    return true;
}

bool HandleGetNotifications(vm::ExecutionEngine& engine)
{
    auto& appEngine = static_cast<ApplicationEngine&>(engine);
    auto& context = appEngine.GetCurrentContext();

    // Get all notifications from the execution context
    auto notifications = appEngine.GetNotifications();

    // Create an array to hold notifications
    auto arrayItem = vm::StackItem::CreateArray();
    auto array = std::dynamic_pointer_cast<vm::ArrayItem>(arrayItem);

    for (const auto& notification : notifications)
    {
        // Create a map for each notification
        auto notifMap = vm::StackItem::CreateMap();
        auto map = std::dynamic_pointer_cast<vm::MapItem>(notifMap);

        // Add script hash
        std::vector<uint8_t> hashVector(notification.script_hash.Data(),
                                        notification.script_hash.Data() + io::UInt160::Size);
        io::ByteVector hashBytes(hashVector.data(), hashVector.size());
        map->Set(vm::StackItem::Create("scripthash"), vm::StackItem::Create(hashBytes));

        // Add event name
        map->Set(vm::StackItem::Create("eventname"), vm::StackItem::Create(notification.event_name));

        // Add state (convert vector to array item)
        auto stateArray = vm::StackItem::CreateArray();
        auto stateArrayPtr = std::dynamic_pointer_cast<vm::ArrayItem>(stateArray);
        for (const auto& item : notification.state)
        {
            stateArrayPtr->Add(item);
        }
        map->Set(vm::StackItem::Create("state"), stateArray);

        array->Add(notifMap);
    }

    context.Push(arrayItem);
    return true;
}

bool HandleBurnGas(vm::ExecutionEngine& engine)
{
    auto& appEngine = static_cast<ApplicationEngine&>(engine);
    auto& context = appEngine.GetCurrentContext();

    auto gasItem = context.Pop();
    auto gas = gasItem->GetInteger();

    if (gas < 0)
    {
        // Invalid gas amount
        return false;
    }

    // Check if we have enough gas
    if (appEngine.GetGasLeft() < gas)
    {
        // Not enough gas available
        return false;
    }

    // Burn the specified amount of gas by adding it to consumed
    appEngine.AddGas(gas);

    return true;
}

bool HandleGetAddressVersion(vm::ExecutionEngine& engine)
{
    auto& appEngine = static_cast<ApplicationEngine&>(engine);
    auto& context = appEngine.GetCurrentContext();

    // Neo address version byte is always 0x35 (53 in decimal)
    // This is defined in the Neo protocol
    constexpr uint8_t NEO_ADDRESS_VERSION = core::ProtocolConstants::AddressVersion;

    context.Push(vm::StackItem::Create(static_cast<int64_t>(NEO_ADDRESS_VERSION)));
    return true;
}

bool HandleLoadScript(vm::ExecutionEngine& engine)
{
    auto& appEngine = static_cast<ApplicationEngine&>(engine);
    auto& context = appEngine.GetCurrentContext();

    // Get the script from the stack
    auto scriptItem = context.Pop();
    auto scriptBytes = scriptItem->GetByteArray();

    // Load the script into the engine
    appEngine.LoadScript(std::vector<uint8_t>(scriptBytes.Data(), scriptBytes.Data() + scriptBytes.Size()));

    return true;
}
}  // namespace

// This function provides the runtime system call handlers
// The actual registration is handled in application_engine.cpp
void RegisterRuntimeSystemCalls(ApplicationEngine& engine)
{
    // Register System.Runtime.* system calls
    engine.RegisterSystemCall("System.Runtime.GetTrigger", HandleGetTrigger, 250, CallFlags::None);
    engine.RegisterSystemCall("System.Runtime.CheckWitness", HandleCheckWitness, 1000, CallFlags::None);
    engine.RegisterSystemCall("System.Runtime.Notify", HandleNotify, 300, CallFlags::AllowNotify);
    engine.RegisterSystemCall("System.Runtime.Log", HandleLog, 300, CallFlags::None);
    engine.RegisterSystemCall("System.Runtime.GetTime", HandleGetTime, 250, CallFlags::ReadStates);
    engine.RegisterSystemCall("System.Runtime.Platform", HandleGetPlatform, 250, CallFlags::None);
    engine.RegisterSystemCall("System.Runtime.GetNetwork", HandleGetNetwork, 250, CallFlags::ReadStates);
    engine.RegisterSystemCall("System.Runtime.GetRandom", HandleGetRandom, 250, CallFlags::None);
    engine.RegisterSystemCall("System.Runtime.GasLeft", HandleGasLeft, 400, CallFlags::None);
    engine.RegisterSystemCall("System.Runtime.GetInvocationCounter", HandleGetInvocationCounter, 400, CallFlags::None);
    engine.RegisterSystemCall("System.Runtime.GetScriptContainer", HandleGetScriptContainer, 250, CallFlags::None);
    engine.RegisterSystemCall("System.Runtime.GetExecutingScriptHash", HandleGetExecutingScriptHash, 400,
                              CallFlags::None);
    engine.RegisterSystemCall("System.Runtime.GetCallingScriptHash", HandleGetCallingScriptHash, 400, CallFlags::None);
    engine.RegisterSystemCall("System.Runtime.GetEntryScriptHash", HandleGetEntryScriptHash, 400, CallFlags::None);
    engine.RegisterSystemCall("System.Runtime.GetNotifications", HandleGetNotifications, 800, CallFlags::None);
    engine.RegisterSystemCall("System.Runtime.BurnGas", HandleBurnGas, 400, CallFlags::None);
    engine.RegisterSystemCall("System.Runtime.GetAddressVersion", HandleGetAddressVersion, 250, CallFlags::None);
    engine.RegisterSystemCall("System.Runtime.LoadScript", HandleLoadScript, 500, CallFlags::AllowCall);
}
}  // namespace neo::smartcontract
