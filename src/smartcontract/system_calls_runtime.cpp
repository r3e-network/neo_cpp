#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/system_calls.h>
#include <neo/smartcontract/system_call_constants.h>
#include <chrono>

namespace neo::smartcontract
{
    // This file contains the implementation of runtime-related system calls

    namespace
    {
        void RegisterRuntimeSystemCallsImpl(ApplicationEngine& engine)
        {
            // System.Runtime.GetTrigger
            engine.RegisterSystemCall(system_call::GetTrigger, [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                appEngine.GetCurrentContext().Push(vm::StackItem::Create(static_cast<int64_t>(appEngine.GetTrigger())));
                return true;
            }, gas_cost::GetTrigger);

            // System.Runtime.CheckWitness
            engine.RegisterSystemCall(system_call::CheckWitness, [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                auto hashItem = context.Pop();
                auto hashBytes = hashItem->GetByteArray();

                bool result = false;
                if (hashBytes.Size() == 20)
                {
                    io::UInt160 hash;
                    std::memcpy(hash.Data(), hashBytes.Data(), 20);
                    result = appEngine.CheckWitness(hash);
                }
                else if (hashBytes.Size() == 32)
                {
                    io::UInt256 hash;
                    std::memcpy(hash.Data(), hashBytes.Data(), 32);
                    result = appEngine.CheckWitness(hash);
                }

                context.Push(vm::StackItem::Create(result));
                return true;
            }, gas_cost::CheckWitness);

            // System.Runtime.Notify
            engine.RegisterSystemCall(system_call::Notify, [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                auto stateItem = context.Pop();
                auto nameItem = context.Pop();

                auto state = stateItem->GetArray();
                auto name = nameItem->GetString();

                appEngine.Notify(appEngine.GetCurrentScriptHash(), name, state);
                return true;
            }, gas_cost::Notify, CallFlags::AllowNotify);

            // System.Runtime.Log
            engine.RegisterSystemCall(system_call::Log, [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                auto messageItem = context.Pop();
                auto message = messageItem->GetString();

                // Log the message using the application engine's logging system
                appEngine.Log(message);
                return true;
            }, gas_cost::Log, CallFlags::AllowNotify);

            // System.Runtime.GetTime
            engine.RegisterSystemCall(system_call::GetTime, [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                // Get the current block time or system time
                uint64_t time = 0;
                if (appEngine.GetPersistingBlock())
                {
                    time = appEngine.GetPersistingBlock()->GetTimestamp();
                }
                else if (appEngine.GetTransaction())
                {
                    time = appEngine.GetTransaction()->GetTimestamp();
                }
                else
                {
                    // Use system time as fallback
                    time = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()
                    ).count();
                }

                context.Push(vm::StackItem::Create(static_cast<int64_t>(time)));
                return true;
            }, gas_cost::GetTime);

            // System.Runtime.GetPlatform
            engine.RegisterSystemCall(system_call::GetPlatform, [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                context.Push(vm::StackItem::Create("NEO"));
                return true;
            }, gas_cost::GetPlatform);

            // System.Runtime.GetNetwork
            engine.RegisterSystemCall(system_call::GetNetwork, [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                // TODO: Get actual network magic
                context.Push(vm::StackItem::Create(static_cast<int64_t>(860833102)));
                return true;
            }, gas_cost::GetNetwork);

            // System.Runtime.GetRandom
            engine.RegisterSystemCall(system_call::GetRandom, [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                context.Push(vm::StackItem::Create(static_cast<int64_t>(appEngine.GetRandom())));
                return true;
            }, gas_cost::GetRandom);

            // System.Runtime.GasLeft
            engine.RegisterSystemCall(system_call::GasLeft, [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                context.Push(vm::StackItem::Create(appEngine.GetGasLeft()));
                return true;
            }, gas_cost::GasLeft);

            // System.Runtime.GetInvocationCounter
            engine.RegisterSystemCall(system_call::GetInvocationCounter, [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                // TODO: Implement invocation counter
                context.Push(vm::StackItem::Create(static_cast<int64_t>(1)));
                return true;
            }, gas_cost::GetInvocationCounter);

            // System.Runtime.GetScriptContainer
            engine.RegisterSystemCall(system_call::GetScriptContainer, [](vm::ExecutionEngine& engine) {
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
                        mapItem->SetValue("type", vm::StackItem::Create("Transaction"));

                        // Add transaction hash
                        auto hash = tx->GetHash();
                        io::ByteVector hashBytes(hash.Data(), hash.Data() + hash.Size());
                        mapItem->SetValue("hash", vm::StackItem::Create(hashBytes));

                        // Add transaction version
                        mapItem->SetValue("version", vm::StackItem::Create(static_cast<int64_t>(tx->GetVersion())));

                        // Add transaction nonce
                        mapItem->SetValue("nonce", vm::StackItem::Create(static_cast<int64_t>(tx->GetNonce())));

                        // Add transaction sender
                        auto sender = tx->GetSender();
                        io::ByteVector senderBytes(sender.Data(), sender.Data() + sender.Size());
                        mapItem->SetValue("sender", vm::StackItem::Create(senderBytes));

                        // Add transaction system fee
                        mapItem->SetValue("sysfee", vm::StackItem::Create(tx->GetSystemFee()));

                        // Add transaction network fee
                        mapItem->SetValue("netfee", vm::StackItem::Create(tx->GetNetworkFee()));

                        // Add transaction valid until block
                        mapItem->SetValue("validuntilblock", vm::StackItem::Create(static_cast<int64_t>(tx->GetValidUntilBlock())));
                    }
                    else if (auto block = dynamic_cast<const ledger::Block*>(container))
                    {
                        mapItem->SetValue("type", vm::StackItem::Create("Block"));

                        // Add block hash
                        auto hash = block->GetHash();
                        io::ByteVector hashBytes(hash.Data(), hash.Data() + hash.Size());
                        mapItem->SetValue("hash", vm::StackItem::Create(hashBytes));

                        // Add block version
                        mapItem->SetValue("version", vm::StackItem::Create(static_cast<int64_t>(block->GetVersion())));

                        // Add block index
                        mapItem->SetValue("index", vm::StackItem::Create(static_cast<int64_t>(block->GetIndex())));

                        // Add block merkle root
                        auto merkleRoot = block->GetMerkleRoot();
                        io::ByteVector merkleRootBytes(merkleRoot.Data(), merkleRoot.Data() + merkleRoot.Size());
                        mapItem->SetValue("merkleroot", vm::StackItem::Create(merkleRootBytes));

                        // Add block timestamp
                        mapItem->SetValue("timestamp", vm::StackItem::Create(static_cast<int64_t>(block->GetTimestamp())));

                        // Add block next consensus
                        auto nextConsensus = block->GetNextConsensus();
                        io::ByteVector nextConsensusBytes(nextConsensus.Data(), nextConsensus.Data() + nextConsensus.Size());
                        mapItem->SetValue("nextconsensus", vm::StackItem::Create(nextConsensusBytes));
                    }

                    context.Push(mapItem);
                }
                else
                {
                    context.Push(vm::StackItem::Create(nullptr));
                }

                return true;
            }, gas_cost::GetScriptContainer);

            // System.Runtime.GetExecutingScriptHash
            engine.RegisterSystemCall(system_call::GetExecutingScriptHash, [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                auto hash = appEngine.GetCurrentScriptHash();
                io::ByteVector hashBytes(hash.Data(), hash.Data() + hash.Size());

                context.Push(vm::StackItem::Create(hashBytes));
                return true;
            }, gas_cost::GetExecutingScriptHash);

            // System.Runtime.GetCallingScriptHash
            engine.RegisterSystemCall(system_call::GetCallingScriptHash, [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                auto hash = appEngine.GetCallingScriptHash();
                io::ByteVector hashBytes(hash.Data(), hash.Data() + hash.Size());

                context.Push(vm::StackItem::Create(hashBytes));
                return true;
            }, gas_cost::GetCallingScriptHash);

            // System.Runtime.GetEntryScriptHash
            engine.RegisterSystemCall(system_call::GetEntryScriptHash, [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                auto hash = appEngine.GetEntryScriptHash();
                io::ByteVector hashBytes(hash.Data(), hash.Data() + hash.Size());

                context.Push(vm::StackItem::Create(hashBytes));
                return true;
            }, gas_cost::GetEntryScriptHash);
        }
    }

    // This function will be called from the RegisterSystemCalls method in application_engine_system_calls.cpp
    void RegisterRuntimeSystemCalls(ApplicationEngine& engine)
    {
        RegisterRuntimeSystemCallsImpl(engine);
    }
}
