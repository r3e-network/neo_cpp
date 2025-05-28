#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/storage_iterator.h>
#include <neo/smartcontract/system_calls.h>
#include <neo/smartcontract/system_call_constants.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/storage_item.h>

namespace neo::smartcontract
{
    // This file contains the implementation of storage-related system calls

    namespace
    {

        void RegisterStorageSystemCallsImpl(ApplicationEngine& engine)
        {
            // System.Storage.Get
            engine.RegisterSystemCall(system_call::StorageGet, [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                auto keyItem = context.Pop();
                auto keyBytes = keyItem->GetByteArray();

                persistence::StorageKey key(appEngine.GetCurrentScriptHash(), keyBytes);
                auto item = appEngine.GetSnapshot()->TryGet(key);

                if (item)
                {
                    context.Push(vm::StackItem::Create(item->GetValue()));
                }
                else
                {
                    context.Push(vm::StackItem::Create(io::ByteVector()));
                }

                return true;
            }, gas_cost::StorageGet, CallFlags::ReadStates);

            // System.Storage.Put
            engine.RegisterSystemCall(system_call::StoragePut, [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                auto valueItem = context.Pop();
                auto keyItem = context.Pop();

                auto valueBytes = valueItem->GetByteArray();
                auto keyBytes = keyItem->GetByteArray();

                persistence::StorageKey key(appEngine.GetCurrentScriptHash(), keyBytes);
                persistence::StorageItem item(valueBytes);

                appEngine.GetSnapshot()->Add(key, item);
                return true;
            }, gas_cost::StoragePut, CallFlags::WriteStates);

            // System.Storage.Delete
            engine.RegisterSystemCall(system_call::StorageDelete, [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                auto keyItem = context.Pop();
                auto keyBytes = keyItem->GetByteArray();

                persistence::StorageKey key(appEngine.GetCurrentScriptHash(), keyBytes);
                appEngine.GetSnapshot()->Delete(key);

                return true;
            }, gas_cost::StorageDelete, CallFlags::WriteStates);

            // System.Storage.Find
            engine.RegisterSystemCall(system_call::StorageFind, [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                auto prefixItem = context.Pop();
                auto prefixBytes = prefixItem->GetByteArray();

                persistence::StorageKey prefix(appEngine.GetCurrentScriptHash(), prefixBytes);
                auto iterator = appEngine.GetSnapshot()->Find(prefix);

                // Create a new iterator
                auto storageIterator = std::make_shared<StorageIterator>(appEngine.GetSnapshot(), prefix);

                // Create an interop interface for the iterator
                auto iteratorItem = vm::StackItem::CreateInteropInterface(storageIterator);

                context.Push(iteratorItem);
                return true;
            }, gas_cost::StorageFind, CallFlags::ReadStates);

            // System.Iterator.Next
            engine.RegisterSystemCall(system_call::IteratorNext, [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                auto iteratorItem = context.Pop();

                // Check if the item is an interop interface
                if (!iteratorItem->IsInteropInterface())
                    throw std::runtime_error("Item is not an iterator");

                // Get the iterator
                auto iterator = iteratorItem->GetInterface<void>();

                // Check if it's a storage iterator
                if (auto storageIterator = std::dynamic_pointer_cast<StorageIterator>(iterator))
                {
                    // Check if there are more items
                    bool hasNext = storageIterator->HasNext();

                    // If there are more items, advance the iterator
                    if (hasNext)
                    {
                        storageIterator->Next();
                    }

                    context.Push(vm::StackItem::Create(hasNext));
                }
                else
                {
                    throw std::runtime_error("Unknown iterator type");
                }

                return true;
            }, gas_cost::IteratorNext);

            // System.Iterator.Value
            engine.RegisterSystemCall(system_call::IteratorValue, [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                auto iteratorItem = context.Pop();

                // Check if the item is an interop interface
                if (!iteratorItem->IsInteropInterface())
                    throw std::runtime_error("Item is not an iterator");

                // Get the iterator
                auto iterator = iteratorItem->GetInterface<void>();

                // Check if it's a storage iterator
                if (auto storageIterator = std::dynamic_pointer_cast<StorageIterator>(iterator))
                {
                    // Get the current key-value pair
                    auto pair = storageIterator->GetCurrent();

                    // Create an array with key and value
                    auto arrayItem = vm::StackItem::CreateArray();
                    arrayItem->Add(vm::StackItem::Create(pair.first));
                    arrayItem->Add(vm::StackItem::Create(pair.second));

                    context.Push(arrayItem);
                }
                else
                {
                    throw std::runtime_error("Unknown iterator type");
                }

                return true;
            }, gas_cost::IteratorValue);
        }
    }

    // This function will be called from the RegisterSystemCalls method in application_engine_system_calls.cpp
    void RegisterStorageSystemCalls(ApplicationEngine& engine)
    {
        RegisterStorageSystemCallsImpl(engine);
    }
}
