#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/system_calls.h>
#include <neo/cryptography/hash.h>
#include <neo/cryptography/ecdsa.h>

namespace neo::smartcontract
{
    // This file contains the implementation of crypto-related system calls

    namespace
    {
        void RegisterCryptoSystemCallsImpl(ApplicationEngine& engine)
        {
            // System.Crypto.VerifySignature
            engine.RegisterSystemCall("System.Crypto.VerifySignature", [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                auto signatureItem = context.Pop();
                auto pubKeyItem = context.Pop();
                auto messageItem = context.Pop();

                auto signature = signatureItem->GetByteArray();
                auto pubKey = pubKeyItem->GetByteArray();
                auto message = messageItem->GetByteArray();

                // Verify that the signature and public key have the correct length
                if (signature.Size() != 64 && signature.Size() != 65)
                {
                    context.Push(vm::StackItem::Create(false));
                    return true;
                }

                if (pubKey.Size() != 33 && pubKey.Size() != 65)
                {
                    context.Push(vm::StackItem::Create(false));
                    return true;
                }

                try
                {
                    // Create ECPoint from public key
                    cryptography::ECPoint ecPoint;
                    if (!ecPoint.FromBytes(pubKey.AsSpan()))
                    {
                        context.Push(vm::StackItem::Create(false));
                        return true;
                    }

                    // Create signature from bytes
                    cryptography::ECDsa ecdsa(ecPoint);

                    // Verify signature
                    bool result = ecdsa.VerifySignature(message.AsSpan(), signature.AsSpan());
                    context.Push(vm::StackItem::Create(result));
                }
                catch (const std::exception&)
                {
                    context.Push(vm::StackItem::Create(false));
                }

                return true;
            }, 1 << 15);

            // System.Crypto.CheckSig
            engine.RegisterSystemCall("System.Crypto.CheckSig", [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                auto signatureItem = context.Pop();
                auto pubKeyItem = context.Pop();

                auto signature = signatureItem->GetByteArray();
                auto pubKey = pubKeyItem->GetByteArray();

                try
                {
                    // Get the container (transaction or block)
                    auto container = appEngine.GetContainer();
                    if (!container)
                    {
                        context.Push(vm::StackItem::Create(false));
                        return true;
                    }

                    // Get the signature data from the container
                    io::ByteVector signData;

                    // Get the proper signature data based on container type
                    if (auto tx = dynamic_cast<const ledger::Transaction*>(container))
                    {
                        // Get the transaction's signature data (unsigned transaction data)
                        signData = tx->GetSignData(appEngine.GetNetworkMagic());
                    }
                    else if (auto block = dynamic_cast<const ledger::Block*>(container))
                    {
                        // Get the block's signature data (unsigned block header data)
                        signData = block->GetSignData(appEngine.GetNetworkMagic());
                    }
                    else
                    {
                        context.Push(vm::StackItem::Create(false));
                        return true;
                    }

                    // Create ECPoint from public key
                    cryptography::ECPoint ecPoint;
                    if (!ecPoint.FromBytes(pubKey.AsSpan()))
                    {
                        context.Push(vm::StackItem::Create(false));
                        return true;
                    }

                    // Create signature from bytes
                    cryptography::ECDsa ecdsa(ecPoint);

                    // Verify signature
                    bool result = ecdsa.VerifySignature(signData.AsSpan(), signature.AsSpan());
                    context.Push(vm::StackItem::Create(result));
                }
                catch (const std::exception&)
                {
                    context.Push(vm::StackItem::Create(false));
                }

                return true;
            }, 1 << 15);

            // System.Crypto.Hash160
            engine.RegisterSystemCall("System.Crypto.Hash160", [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                auto dataItem = context.Pop();
                auto data = dataItem->GetByteArray();

                auto hash = cryptography::Hash::Hash160(data.AsSpan());
                io::ByteVector hashBytes(hash.Data(), hash.Data() + hash.Size());

                context.Push(vm::StackItem::Create(hashBytes));
                return true;
            }, 1 << 15);

            // System.Crypto.Hash256
            engine.RegisterSystemCall("System.Crypto.Hash256", [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                auto dataItem = context.Pop();
                auto data = dataItem->GetByteArray();

                auto hash = cryptography::Hash::Sha256(data.AsSpan());
                io::ByteVector hashBytes(hash.Data(), hash.Data() + hash.Size());

                context.Push(vm::StackItem::Create(hashBytes));
                return true;
            }, 1 << 15);
        }
    }

    // This function will be called from the RegisterSystemCalls method in application_engine_system_calls.cpp
    void RegisterCryptoSystemCalls(ApplicationEngine& engine)
    {
        RegisterCryptoSystemCallsImpl(engine);
    }
}
