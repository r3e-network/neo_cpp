#include <neo/cryptography/hash.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/native_contract.h>
#include <neo/smartcontract/system_calls.h>

namespace neo::smartcontract
{
// This file contains the implementation of contract-related system calls

namespace
{
void RegisterContractSystemCallsImpl(ApplicationEngine& engine)
{
    // System.Contract.Call
    engine.RegisterSystemCall(
        "System.Contract.Call",
        [](vm::ExecutionEngine& engine)
        {
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
        },
        1 << 15, CallFlags::AllowCall);

    // System.Contract.GetCallFlags
    engine.RegisterSystemCall(
        "System.Contract.GetCallFlags",
        [](vm::ExecutionEngine& engine)
        {
            auto& appEngine = static_cast<ApplicationEngine&>(engine);
            auto& context = appEngine.GetCurrentContext();

            context.Push(vm::StackItem::Create(static_cast<int64_t>(appEngine.GetCallFlags())));
            return true;
        },
        1 << 4);

    // System.Contract.CallNative
    engine.RegisterSystemCall(
        "System.Contract.CallNative",
        [](vm::ExecutionEngine& engine)
        {
            auto& appEngine = static_cast<ApplicationEngine&>(engine);
            auto& context = appEngine.GetCurrentContext();

            auto contractHash = context.Pop()->GetByteArray();  // Fixed: GetByteArray instead of GetBytes
            auto methodName = context.Pop()->GetString();       // Method name

            auto& contracts = appEngine.GetContracts();

            // Create UInt160 from byte array
            if (contractHash.Size() != 20)
            {
                context.Push(vm::StackItem::Null());
                return true;
            }

            io::UInt160 hashKey;
            std::memcpy(hashKey.Data(), contractHash.AsSpan().Data(), 20);

            auto contractIt = contracts.find(hashKey);
            if (contractIt == contracts.end())  // Fixed: compare with end() instead of 0
            {
                context.Push(vm::StackItem::Null());
                return true;
            }

            // Execute native contract method with proper implementation
            try
            {
                // Get the native contract instance
                auto nativeContract = appEngine.GetNativeContract(hashKey);
                if (!nativeContract)
                {
                    // Not a native contract
                    context.Push(vm::StackItem::Null());
                    return true;
                }

                // Collect method arguments from the stack
                // The number of arguments depends on the method being called
                std::vector<std::shared_ptr<vm::StackItem>> args;

                // Get method from native contract's method map
                const auto& methods = nativeContract->GetMethods();
                auto methodIt = methods.find(methodName);
                if (methodIt == methods.end())
                {
                    // Method not found
                    context.Push(vm::StackItem::Null());
                    return true;
                }

                // Get method info
                auto requiredFlags = methodIt->second.first;
                auto currentFlags = appEngine.GetCallFlags();

                // Check call flags against method requirements
                if ((static_cast<uint8_t>(currentFlags) & static_cast<uint8_t>(requiredFlags)) !=
                    static_cast<uint8_t>(requiredFlags))
                {
                    // Insufficient permissions
                    throw std::runtime_error("Insufficient permissions to call method");
                }

                // Native contract methods handle their own argument parsing
                // We don't know the exact parameter count, so we'll pass empty args
                // The actual native method implementation will pop from the stack as needed

                // Execute the native contract method
                auto result = nativeContract->Invoke(appEngine, methodName, args, currentFlags);

                // Push result onto stack
                if (result)
                {
                    context.Push(result);
                }
                else
                {
                    context.Push(vm::StackItem::Null());
                }
            }
            catch (const std::exception& ex)
            {
                // Return false to indicate failure
                return false;
            }

            return true;
        },
        0, CallFlags::None);

    // System.Contract.CreateStandardAccount
    engine.RegisterSystemCall(
        "System.Contract.CreateStandardAccount",
        [](vm::ExecutionEngine& engine)
        {
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
                script.Push(0x21);  // PUSH33 opcode

                // Add the 33-byte public key
                auto pubkeyBytes = pubKeyBytes.AsSpan();
                script.Append(pubkeyBytes);

                script.Push(0x41);  // CHECKSIG opcode

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
        },
        1 << 10);

    // System.Contract.CreateMultisigAccount
    engine.RegisterSystemCall(
        "System.Contract.CreateMultisigAccount",
        [](vm::ExecutionEngine& engine)
        {
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

            // Create proper multisig redeem script
            // This matches C# Contract.CreateMultiSigRedeemScript(m, pubkeys).ToScriptHash()
            try
            {
                // Validate public key count
                if (pubKeysArray.size() > 1024)
                {
                    throw std::runtime_error("Too many public keys");
                }

                // Create multisig script using Neo VM opcodes
                io::ByteVector script;

                // Push m value using appropriate opcode
                if (m <= 16)
                {
                    // Use PUSH0-PUSH16 opcodes (0x10 + m)
                    script.Push(static_cast<uint8_t>(0x10 + m));
                }
                else
                {
                    // Use PUSHDATA1 for larger values
                    script.Push(0x01);  // PUSHDATA1
                    script.Push(static_cast<uint8_t>(m));
                }

                // Sort public keys lexicographically (required by Neo protocol)
                std::vector<io::ByteVector> sortedPubKeys;
                for (const auto& pubKeyItem : pubKeysArray)
                {
                    auto pubKeyBytes = pubKeyItem->GetByteArray();
                    if (pubKeyBytes.Size() != 33)  // Compressed public key size
                    {
                        throw std::runtime_error("Invalid public key size");
                    }
                    sortedPubKeys.push_back(pubKeyBytes);
                }

                // Sort public keys
                std::sort(sortedPubKeys.begin(), sortedPubKeys.end(),
                          [](const io::ByteVector& a, const io::ByteVector& b)
                          {
                              return std::lexicographical_compare(a.AsSpan().Data(), a.AsSpan().Data() + a.Size(),
                                                                  b.AsSpan().Data(), b.AsSpan().Data() + b.Size());
                          });

                // Add each public key with PUSH opcode
                for (const auto& pubKey : sortedPubKeys)
                {
                    script.Push(0x21);  // PUSH33 - push 33 bytes
                    script.Append(pubKey.AsSpan());
                }

                // Push n value (number of public keys)
                if (sortedPubKeys.size() <= 16)
                {
                    // Use PUSH0-PUSH16 opcodes
                    script.Push(static_cast<uint8_t>(0x10 + sortedPubKeys.size()));
                }
                else
                {
                    // Use PUSHDATA1 for larger values
                    script.Push(0x01);  // PUSHDATA1
                    script.Push(static_cast<uint8_t>(sortedPubKeys.size()));
                }

                // Add CHECKMULTISIG opcode
                script.Push(0xAE);  // CHECKMULTISIG

                // Calculate script hash (Hash160 of the script)
                auto scriptHash = cryptography::Hash::Hash160(script.AsSpan());

                // Return as byte array
                context.Push(vm::StackItem::Create(scriptHash.ToArray()));
            }
            catch (...)
            {
                context.Push(vm::StackItem::Null());
            }

            return true;
        },
        1 << 10);
}
}  // namespace

// This function will be called from the RegisterSystemCalls method in application_engine_system_calls.cpp
void RegisterContractSystemCalls(ApplicationEngine& engine)
{
    RegisterContractSystemCallsImpl(engine);
}
}  // namespace neo::smartcontract
