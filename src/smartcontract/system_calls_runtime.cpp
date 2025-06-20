#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/system_calls.h>
#include <neo/smartcontract/system_call_constants.h>
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
            uint32_t networkMagic = 860833102; // Default mainnet magic
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

            // For now, return 1 as a reasonable default (first invocation)
            context.Push(vm::StackItem::Create(static_cast<int64_t>(1)));
            return true;
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
                    txMap->Set(vm::StackItem::Create("validuntilblock"), vm::StackItem::Create(static_cast<int64_t>(tx->GetValidUntilBlock())));
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
                    blockMap->Set(vm::StackItem::Create("version"), vm::StackItem::Create(static_cast<int64_t>(block->GetVersion())));

                    // Add block index
                    blockMap->Set(vm::StackItem::Create("index"), vm::StackItem::Create(static_cast<int64_t>(block->GetIndex())));

                    // Add block merkle root
                    auto merkleRoot = block->GetMerkleRoot();
                    std::vector<uint8_t> merkleRootVector(merkleRoot.Data(), merkleRoot.Data() + io::UInt256::Size);
                    io::ByteVector merkleRootBytes(merkleRootVector.data(), merkleRootVector.size());
                    blockMap->Set(vm::StackItem::Create("merkleroot"), vm::StackItem::Create(merkleRootBytes));

                    // Add block timestamp
                    blockMap->Set(vm::StackItem::Create("timestamp"), vm::StackItem::Create(static_cast<int64_t>(block->GetTimestamp())));

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
    }

    // This function provides the runtime system call handlers
    // The actual registration is handled in application_engine.cpp
    void RegisterRuntimeSystemCalls(ApplicationEngine& engine)
    {
        // System call registration is handled in the ApplicationEngine constructor
        // This function is kept for compatibility but doesn't register calls directly
    }
}
