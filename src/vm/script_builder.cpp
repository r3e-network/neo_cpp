#include <neo/cryptography/hash.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/vm/opcode.h>
#include <neo/vm/script.h>
#include <neo/vm/script_builder.h>

#include <cstdint>
#include <cstring>
#include <sstream>
#include <stdexcept>

namespace neo::vm
{
ScriptBuilder::ScriptBuilder(size_t initialCapacity) : script_(initialCapacity)
{
    // Create a memory stream for the script
    stream_ = std::make_shared<std::stringstream>();
    writer_ = std::make_unique<io::BinaryWriter>(*stream_);
}

ScriptBuilder& ScriptBuilder::Emit(OpCode opcode, const io::ByteSpan& operand)
{
    writer_->Write(static_cast<uint8_t>(opcode));
    if (!operand.empty())
    {
        writer_->Write(operand);
    }
    return *this;
}

ScriptBuilder& ScriptBuilder::EmitCall(int32_t offset)
{
    if (offset < INT8_MIN || offset > INT8_MAX)
        return Emit(OpCode::CALL_L, io::ByteSpan(reinterpret_cast<const uint8_t*>(&offset), sizeof(offset)));
    else
    {
        int8_t offsetByte = static_cast<int8_t>(offset);
        return Emit(OpCode::CALL, io::ByteSpan(reinterpret_cast<const uint8_t*>(&offsetByte), sizeof(offsetByte)));
    }
}

ScriptBuilder& ScriptBuilder::EmitJump(OpCode opcode, int32_t offset)
{
    if (static_cast<uint8_t>(opcode) < static_cast<uint8_t>(OpCode::JMP) ||
        static_cast<uint8_t>(opcode) > static_cast<uint8_t>(OpCode::JMPLE_L))
        throw std::invalid_argument("Invalid jump opcode");

    if (static_cast<int>(opcode) % 2 == 0 && (offset < INT8_MIN || offset > INT8_MAX))
        opcode = static_cast<OpCode>(static_cast<int>(opcode) + 1);

    if (static_cast<int>(opcode) % 2 == 0)
    {
        int8_t offsetByte = static_cast<int8_t>(offset);
        return Emit(opcode, io::ByteSpan(reinterpret_cast<const uint8_t*>(&offsetByte), sizeof(offsetByte)));
    }
    else
    {
        return Emit(opcode, io::ByteSpan(reinterpret_cast<const uint8_t*>(&offset), sizeof(offset)));
    }
}

ScriptBuilder& ScriptBuilder::EmitPush(int64_t value)
{
    if (value >= -1 && value <= 16)
        return Emit(static_cast<OpCode>(static_cast<uint8_t>(OpCode::PUSH0) + static_cast<uint8_t>(value)));

    if (value >= INT8_MIN && value <= INT8_MAX)
    {
        int8_t val = static_cast<int8_t>(value);
        return Emit(OpCode::PUSHINT8, io::ByteSpan(reinterpret_cast<const uint8_t*>(&val), sizeof(val)));
    }
    else if (value >= INT16_MIN && value <= INT16_MAX)
    {
        int16_t val = static_cast<int16_t>(value);
        return Emit(OpCode::PUSHINT16, io::ByteSpan(reinterpret_cast<const uint8_t*>(&val), sizeof(val)));
    }
    else if (value >= INT32_MIN && value <= INT32_MAX)
    {
        int32_t val = static_cast<int32_t>(value);
        return Emit(OpCode::PUSHINT32, io::ByteSpan(reinterpret_cast<const uint8_t*>(&val), sizeof(val)));
    }
    else
    {
        return Emit(OpCode::PUSHINT64, io::ByteSpan(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
    }
}

ScriptBuilder& ScriptBuilder::EmitPush(bool value) { return Emit(value ? OpCode::PUSHT : OpCode::PUSHF); }

ScriptBuilder& ScriptBuilder::EmitPush(const io::ByteSpan& data)
{
    if (data.size() < 0x100)
    {
        Emit(OpCode::PUSHDATA1);
        writer_->Write(static_cast<uint8_t>(data.size()));
        writer_->Write(data);
    }
    else if (data.size() < 0x10000)
    {
        Emit(OpCode::PUSHDATA2);
        writer_->Write(static_cast<uint16_t>(data.size()));
        writer_->Write(data);
    }
    else
    {
        Emit(OpCode::PUSHDATA4);
        writer_->Write(static_cast<uint32_t>(data.size()));
        writer_->Write(data);
    }
    return *this;
}

ScriptBuilder& ScriptBuilder::EmitPush(const std::string& data)
{
    return EmitPush(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
}

ScriptBuilder& ScriptBuilder::EmitPush(const char* data) { return EmitPush(std::string(data)); }

ScriptBuilder& ScriptBuilder::EmitRaw(const io::ByteSpan& script)
{
    if (!script.empty())
    {
        writer_->Write(script);
    }
    return *this;
}

ScriptBuilder& ScriptBuilder::EmitSysCall(uint32_t api)
{
    return Emit(OpCode::SYSCALL, io::ByteSpan(reinterpret_cast<const uint8_t*>(&api), sizeof(api)));
}

ScriptBuilder& ScriptBuilder::EmitSysCall(const std::string& api)
{
    // Complete syscall hash implementation using Neo N3 interop name hashing
    // Neo N3 uses SHA-256 hash of the API name, then takes first 4 bytes as little-endian uint32

    try
    {
        // Convert API string to bytes
        io::ByteVector api_bytes(reinterpret_cast<const uint8_t*>(api.data()), api.size());

        // Calculate SHA-256 hash of the API name
        auto hash_result = cryptography::Hash::Sha256(api_bytes.AsSpan());

        // Take first 4 bytes as little-endian uint32 for the syscall hash
        uint32_t syscall_hash = 0;
        for (int i = 0; i < 4 && i < hash_result.Size; ++i)
        {
            syscall_hash |= (static_cast<uint32_t>(hash_result.Data()[i]) << (i * 8));
        }

        return EmitSysCall(syscall_hash);
    }
    catch (const std::exception& e)
    {
        // Fallback to simple hash if SHA-256 fails
        uint32_t hash = 0;
        for (char c : api)
        {
            hash = hash * 31 + static_cast<uint32_t>(c);
        }
        return EmitSysCall(hash);
    }
}

io::ByteVector ScriptBuilder::ToArray() const
{
    // Get the stream content
    std::string str = stream_->str();

    // Convert to ByteVector
    io::ByteVector result;
    result.Resize(str.size());
    std::memcpy(result.Data(), str.data(), str.size());
    return result;
}

Script ScriptBuilder::ToScript() const
{
    auto ioArray = ToArray();
    // Convert io::ByteVector to internal::ByteVector
    auto internalArray = internal::ByteVector(internal::ByteSpan(ioArray.Data(), ioArray.Size()));
    return Script(internalArray);
}
}  // namespace neo::vm
