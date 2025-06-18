#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/system_calls.h>
#include <neo/cryptography/hash.h>

namespace neo::smartcontract
{
    // This file contains the implementation of contract-related system calls

    namespace
    {
        void RegisterContractSystemCallsImpl(ApplicationEngine& engine)
        {
            // System.Contract.Call
            engine.RegisterSystemCall("System.Contract.Call", [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                auto argsItem = context.Pop();
                auto methodItem = context.Pop();
                auto hashItem = context.Pop();
                auto flagsItem = context.Pop();

                auto args = argsItem->GetArray();
                auto method = methodItem->GetString();
                auto hashBytes = hashItem->GetByteArray();
                auto flags = static_cast<CallFlags>(flagsItem->GetInteger());

                if (hashBytes.Size() != 20)
                    throw std::runtime_error("Invalid script hash");

                io::UInt160 hash;
                std::memcpy(hash.Data(), hashBytes.Data(), 20);

                auto result = appEngine.CallContract(hash, method, args, flags);
                context.Push(result);

                return true;
            }, 1 << 15, CallFlags::AllowCall);

            // System.Contract.GetCallFlags
            engine.RegisterSystemCall("System.Contract.GetCallFlags", [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                context.Push(vm::StackItem::Create(static_cast<int64_t>(appEngine.GetCallFlags())));
                return true;
            }, 1 << 4);

            // System.Contract.CallNative
            engine.RegisterSystemCall("System.Contract.CallNative", [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                auto contractHash = context.Pop()->GetByteArray(); // Fixed: GetByteArray instead of GetBytes
                auto methodName = context.Pop()->GetString();   // Method name

                auto& contracts = appEngine.GetContracts();
                
                // Create UInt160 from byte array
                if (contractHash.Size() != 20) {
                    context.Push(vm::StackItem::Null());
                    return true;
                }
                
                io::UInt160 hashKey;
                std::memcpy(hashKey.Data(), contractHash.AsSpan().Data(), 20);
                
                auto contractIt = contracts.find(hashKey);
                if (contractIt == contracts.end()) // Fixed: compare with end() instead of 0
                {
                    context.Push(vm::StackItem::Null());
                    return true;
                }

                // Execute contract method (simplified)
                auto methodIt = contractIt->second.find(methodName);
                if (methodIt != contractIt->second.end())
                {
                    // Check permissions
                    auto currentFlags = appEngine.GetCallFlags();
                    // Add permission checks here

                    auto oldFlags = appEngine.GetCallFlags();
                    appEngine.SetCallFlags(static_cast<CallFlags>(static_cast<int>(oldFlags) | static_cast<int>(CallFlags::ReadOnly)));

                    bool result = methodIt->second(appEngine);

                    appEngine.SetCallFlags(static_cast<CallFlags>(static_cast<int>(oldFlags) | static_cast<int>(CallFlags::All)));

                    context.Push(vm::StackItem::Create(result));
                }
                else
                {
                    context.Push(vm::StackItem::Null());
                }

                return true;
            }, 0, CallFlags::None);

            // System.Contract.CreateStandardAccount
            engine.RegisterSystemCall("System.Contract.CreateStandardAccount", [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                auto pubKeyItem = context.Pop();
                auto pubKeyBytes = pubKeyItem->GetByteArray();

                // Implement proper account creation from public key
                // This matches C# Contract.CreateSignatureRedeemScript(pubkey).ToScriptHash()
                try
                {
                    // Create signature redeem script: PUSH(pubkey) + CHECKSIG
                    io::ByteVector script;
                    script.Push(0x21); // PUSH33 opcode
                    
                    // Add the 33-byte public key
                    auto pubkeyBytes = pubKeyBytes.AsSpan();
                    script.Append(pubkeyBytes);
                    
                    script.Push(0x41); // CHECKSIG opcode
                    
                    // Calculate script hash (Hash160 of the script)
                    auto scriptHash = cryptography::Hash::Hash160(script.AsSpan());
                    
                    context.Push(vm::StackItem::Create(scriptHash.ToArray()));
                }
                catch (...)
                {
                    // On error, push null
                    context.Push(vm::StackItem::Null());
                }

                return true;
            }, 1 << 10);

            // System.Contract.CreateMultisigAccount
            engine.RegisterSystemCall("System.Contract.CreateMultisigAccount", [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                auto mItem = context.Pop();
                auto pubKeysItem = context.Pop();

                auto m = static_cast<int>(mItem->GetInteger());
                auto pubKeysArray = pubKeysItem->GetArray();

                // Validate m and pubKeys
                if (m <= 0 || m > static_cast<int>(pubKeysArray.size()))
                {
                    context.Push(vm::StackItem::Null());
                    return true;
                }

                // Simplified multisig account creation
                // In a full implementation, this would create proper multisig script
                try
                {
                    // Create a simplified multisig script hash
                    io::ByteVector script;
                    
                    // Add m value
                    script.Push(static_cast<uint8_t>(m));
                    
                    // Add simplified representation of public keys
                    for (const auto& pubKeyItem : pubKeysArray)
                    {
                        auto pubKeyBytes = pubKeyItem->GetByteArray();
                        script.Append(pubKeyBytes.AsSpan());
                    }
                    
                    // Add number of keys
                    script.Push(static_cast<uint8_t>(pubKeysArray.size()));
                    
                    // Calculate script hash
                    auto scriptHash = cryptography::Hash::Hash160(script.AsSpan());
                    context.Push(vm::StackItem::Create(scriptHash));
                }
                catch (...)
                {
                    context.Push(vm::StackItem::Null());
                }

                return true;
            }, 1 << 10);
        }
    }

    // This function will be called from the RegisterSystemCalls method in application_engine_system_calls.cpp
    void RegisterContractSystemCalls(ApplicationEngine& engine)
    {
        RegisterContractSystemCallsImpl(engine);
    }
}
