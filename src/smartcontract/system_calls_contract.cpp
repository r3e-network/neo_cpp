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

                context.Push(vm::StackItem::Create(static_cast<int64_t>(appEngine.flags_)));
                return true;
            }, 1 << 4);

            // System.Contract.CallNative
            engine.RegisterSystemCall("System.Contract.CallNative", [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                auto versionItem = context.Pop();
                auto version = static_cast<uint8_t>(versionItem->GetInteger());

                // Get the current script hash
                auto scriptHash = appEngine.GetCurrentScriptHash();

                // Check if the script hash corresponds to a native contract
                auto it = appEngine.contracts_.find(scriptHash);
                if (it == appEngine.contracts_.end())
                    throw std::runtime_error("It is not allowed to use \"System.Contract.CallNative\" directly.");

                // Check if the native contract is active based on the current block height
                auto currentHeight = appEngine.GetCurrentBlockHeight();
                if (!appEngine.IsContractActive(scriptHash, currentHeight))
                {
                    throw std::runtime_error("Native contract is not active at current block height");
                }

                // Call the native contract
                auto methodIt = it->second.find("Invoke");
                if (methodIt == it->second.end())
                    throw std::runtime_error("Native contract does not have an Invoke method");

                // Save the current flags
                CallFlags oldFlags = appEngine.flags_;

                // Set the flags to None for native contract invocation
                appEngine.flags_ = CallFlags::None;

                // Call the native contract's Invoke method
                bool result = methodIt->second(appEngine);

                // Restore the flags
                appEngine.flags_ = oldFlags;

                if (!result)
                    throw std::runtime_error("Native contract execution failed");

                return true;
            }, 0, CallFlags::None);

            // System.Contract.CreateStandardAccount
            engine.RegisterSystemCall("System.Contract.CreateStandardAccount", [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                auto pubKeyItem = context.Pop();
                auto pubKeyBytes = pubKeyItem->GetByteArray();

                // TODO: Implement proper account creation
                // For now, just hash the public key
                io::UInt160 hash = cryptography::Hash::Hash160(pubKeyBytes.AsSpan());
                io::ByteVector hashBytes(hash.Data(), hash.Data() + hash.Size());

                context.Push(vm::StackItem::Create(hashBytes));
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
                if (m <= 0 || m > pubKeysArray.size())
                {
                    throw std::runtime_error("Invalid m or pubKeys");
                }

                // Convert pubKeys to ECPoint array
                std::vector<cryptography::ECPoint> pubKeys;
                for (const auto& pubKeyItem : pubKeysArray)
                {
                    auto pubKeyBytes = pubKeyItem->GetByteArray();
                    cryptography::ECPoint ecPoint;
                    if (!ecPoint.FromBytes(pubKeyBytes.AsSpan()))
                    {
                        throw std::runtime_error("Invalid public key");
                    }
                    pubKeys.push_back(ecPoint);
                }

                // Sort public keys
                std::sort(pubKeys.begin(), pubKeys.end(), [](const cryptography::ECPoint& a, const cryptography::ECPoint& b) {
                    auto aBytes = a.ToArray();
                    auto bBytes = b.ToArray();
                    return std::lexicographical_compare(aBytes.begin(), aBytes.end(), bBytes.begin(), bBytes.end());
                });

                // Create multisig script
                io::ByteVector script;

                // Push m
                script.Push(static_cast<uint8_t>(m));

                // Push public keys
                for (const auto& pubKey : pubKeys)
                {
                    auto pubKeyBytes = pubKey.ToArray();
                    script.Push(static_cast<uint8_t>(pubKeyBytes.size()));
                    script.Concat(pubKeyBytes);
                }

                // Push n (number of public keys)
                script.Push(static_cast<uint8_t>(pubKeys.size()));

                // Push CHECKMULTISIG opcode
                script.Push(static_cast<uint8_t>(0xAE)); // CHECKMULTISIG opcode

                // Calculate script hash
                io::UInt160 hash = cryptography::Hash::Hash160(script.AsSpan());
                io::ByteVector hashBytes(hash.Data(), hash.Data() + hash.Size());

                context.Push(vm::StackItem::Create(hashBytes));
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
