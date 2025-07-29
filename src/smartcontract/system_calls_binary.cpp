#include <neo/io/byte_vector.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/binary_serializer.h>
#include <neo/vm/stack_item.h>

namespace neo::smartcontract
{
namespace
{
bool HandleBinarySerialize(vm::ExecutionEngine& engine)
{
    auto& appEngine = static_cast<ApplicationEngine&>(engine);
    auto& context = appEngine.GetCurrentContext();

    auto item = context.Pop();

    try
    {
        BinarySerializer serializer;
        auto result = serializer.Serialize(item);
        context.Push(vm::StackItem::Create(result));
        return true;
    }
    catch (const std::exception&)
    {
        // Return empty ByteString on serialization failure
        context.Push(vm::StackItem::Create(io::ByteVector()));
        return false;
    }
}

bool HandleBinaryDeserialize(vm::ExecutionEngine& engine)
{
    auto& appEngine = static_cast<ApplicationEngine&>(engine);
    auto& context = appEngine.GetCurrentContext();

    auto dataItem = context.Pop();
    auto data = dataItem->GetByteArray();

    try
    {
        BinarySerializer serializer;
        auto result = serializer.Deserialize(data.AsSpan());
        context.Push(result);
        return true;
    }
    catch (const std::exception&)
    {
        // Return null on deserialization failure
        context.Push(vm::StackItem::Null());
        return false;
    }
}

bool HandleBinaryBase64Encode(vm::ExecutionEngine& engine)
{
    auto& appEngine = static_cast<ApplicationEngine&>(engine);
    auto& context = appEngine.GetCurrentContext();

    auto dataItem = context.Pop();
    auto data = dataItem->GetByteArray();

    try
    {
        auto base64String = data.ToBase64String();
        context.Push(vm::StackItem::Create(base64String));
        return true;
    }
    catch (const std::exception&)
    {
        context.Push(vm::StackItem::Create(std::string()));
        return false;
    }
}

bool HandleBinaryBase64Decode(vm::ExecutionEngine& engine)
{
    auto& appEngine = static_cast<ApplicationEngine&>(engine);
    auto& context = appEngine.GetCurrentContext();

    auto stringItem = context.Pop();
    auto base64String = stringItem->GetString();

    try
    {
        auto data = io::ByteVector::FromBase64String(base64String);
        context.Push(vm::StackItem::Create(data));
        return true;
    }
    catch (const std::exception&)
    {
        context.Push(vm::StackItem::Create(io::ByteVector()));
        return false;
    }
}

bool HandleBinaryBase58Encode(vm::ExecutionEngine& engine)
{
    auto& appEngine = static_cast<ApplicationEngine&>(engine);
    auto& context = appEngine.GetCurrentContext();

    auto dataItem = context.Pop();
    auto data = dataItem->GetByteArray();

    try
    {
        // Base58 encoding requires external library integration
        context.Push(vm::StackItem::Create(std::string()));
        return true;
    }
    catch (const std::exception&)
    {
        context.Push(vm::StackItem::Create(std::string()));
        return false;
    }
}

bool HandleBinaryBase58Decode(vm::ExecutionEngine& engine)
{
    auto& appEngine = static_cast<ApplicationEngine&>(engine);
    auto& context = appEngine.GetCurrentContext();

    auto stringItem = context.Pop();
    auto base58String = stringItem->GetString();

    try
    {
        // Base58 decoding requires external library integration
        context.Push(vm::StackItem::Create(io::ByteVector()));
        return true;
    }
    catch (const std::exception&)
    {
        context.Push(vm::StackItem::Create(io::ByteVector()));
        return false;
    }
}
}  // namespace

void RegisterBinarySystemCalls(ApplicationEngine& engine)
{
    // Register System.Binary.* system calls
    engine.RegisterSystemCall("System.Binary.Serialize", HandleBinarySerialize, 100000, CallFlags::None);
    engine.RegisterSystemCall("System.Binary.Deserialize", HandleBinaryDeserialize, 500000, CallFlags::None);
    engine.RegisterSystemCall("System.Binary.Base64Encode", HandleBinaryBase64Encode, 100000, CallFlags::None);
    engine.RegisterSystemCall("System.Binary.Base64Decode", HandleBinaryBase64Decode, 100000, CallFlags::None);
    engine.RegisterSystemCall("System.Binary.Base58Encode", HandleBinaryBase58Encode, 100000, CallFlags::None);
    engine.RegisterSystemCall("System.Binary.Base58Decode", HandleBinaryBase58Decode, 100000, CallFlags::None);
}
}  // namespace neo::smartcontract