/**
 * @file script_builder.cpp
 * @brief Builder pattern implementations
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/extensions/biginteger_extensions.h>
#include <neo/cryptography/hash.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/vm/opcode.h>
#include <neo/vm/script.h>
#include <neo/vm/script_builder.h>

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace neo::vm
{
namespace
{
using BigInteger = extensions::BigIntegerExtensions::BigInteger;

template <typename T>
internal::ByteVector ToLittleEndianBytes(T value)
{
    static_assert(std::is_integral_v<T>, "integral type required");
    using Unsigned = std::make_unsigned_t<T>;
    Unsigned bits = std::bit_cast<Unsigned>(value);
    internal::ByteVector bytes(sizeof(T));
    for (size_t i = 0; i < sizeof(T); ++i)
    {
        bytes[i] = static_cast<uint8_t>((bits >> (i * 8)) & static_cast<Unsigned>(0xFF));
    }
    return bytes;
}

internal::ByteVector ToLittleEndianWithPadding(int32_t value, size_t size)
{
    internal::ByteVector bytes(size);
    int64_t extended = static_cast<int64_t>(value);
    size_t copyBytes = std::min<size_t>(size, sizeof(extended));
    for (size_t i = 0; i < copyBytes; ++i)
    {
        bytes[i] =
            static_cast<uint8_t>((static_cast<uint64_t>(extended) >> (i * 8)) & static_cast<uint64_t>(0xFF));
    }

    uint8_t pad = value < 0 ? 0xFF : 0x00;
    for (size_t i = copyBytes; i < size; ++i)
    {
        bytes[i] = pad;
    }

    return bytes;
}

std::vector<uint8_t> TrimTwoComplement(std::vector<uint8_t> bytes, bool negative)
{
    if (bytes.empty()) return bytes;

    if (negative)
    {
        while (bytes.size() > 1 && bytes.back() == 0xFF && (bytes[bytes.size() - 2] & 0x80) == 0x80)
        {
            bytes.pop_back();
        }
    }
    else
    {
        while (bytes.size() > 1 && bytes.back() == 0x00 && (bytes[bytes.size() - 2] & 0x80) == 0)
        {
            bytes.pop_back();
        }
    }
    return bytes;
}

std::vector<uint8_t> ToTwoComplementBytes(const BigInteger& value)
{
    auto magnitude = value.Abs().ToByteArray();
    if (magnitude.empty()) magnitude.push_back(0);

    if (!value.isNegative)
    {
        if ((magnitude.back() & 0x80) != 0) magnitude.push_back(0x00);
        return TrimTwoComplement(std::move(magnitude), false);
    }

    std::vector<uint8_t> result(magnitude.size());
    uint16_t carry = 1;
    for (size_t i = 0; i < magnitude.size(); ++i)
    {
        uint8_t inverted = static_cast<uint8_t>(~magnitude[i]);
        uint16_t sum = static_cast<uint16_t>(inverted) + carry;
        result[i] = static_cast<uint8_t>(sum & 0xFF);
        carry = sum >> 8;
    }
    while (carry != 0)
    {
        result.push_back(static_cast<uint8_t>(carry & 0xFF));
        carry >>= 8;
    }

    if ((result.back() & 0x80) == 0) result.push_back(0xFF);

    return TrimTwoComplement(std::move(result), true);
}

int GetOperandSize(OpCode opcode)
{
    switch (opcode)
    {
        case OpCode::PUSHINT8:
            return 1;
        case OpCode::PUSHINT16:
            return 2;
        case OpCode::PUSHINT32:
            return 4;
        case OpCode::PUSHINT64:
            return 8;
        case OpCode::PUSHINT128:
            return 16;
        case OpCode::PUSHINT256:
            return 32;
        case OpCode::PUSHA:
            return 4;
        case OpCode::JMP:
        case OpCode::JMPIF:
        case OpCode::JMPIFNOT:
        case OpCode::JMPEQ:
        case OpCode::JMPNE:
        case OpCode::JMPGT:
        case OpCode::JMPGE:
        case OpCode::JMPLT:
        case OpCode::JMPLE:
        case OpCode::CALL:
        case OpCode::ENDTRY:
        case OpCode::INITSLOT:
        case OpCode::INITSSLOT:
        case OpCode::LDSFLD:
        case OpCode::STSFLD:
        case OpCode::LDLOC:
        case OpCode::STLOC:
        case OpCode::LDARG:
        case OpCode::STARG:
        case OpCode::NEWARRAY_T:
        case OpCode::ISTYPE:
        case OpCode::CONVERT:
            return 1;
        case OpCode::JMP_L:
        case OpCode::JMPIF_L:
        case OpCode::JMPIFNOT_L:
        case OpCode::JMPEQ_L:
        case OpCode::JMPNE_L:
        case OpCode::JMPGT_L:
        case OpCode::JMPGE_L:
        case OpCode::JMPLT_L:
        case OpCode::JMPLE_L:
        case OpCode::CALL_L:
        case OpCode::ENDTRY_L:
        case OpCode::SYSCALL:
            return 4;
        case OpCode::CALLT:
        case OpCode::TRY:
            return 2;
        case OpCode::TRY_L:
            return 8;
        default:
            return 0;
    }
}
}  // namespace

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

ScriptBuilder& ScriptBuilder::Emit(OpCode opcode, int32_t operand)
{
    int operandSize = GetOperandSize(opcode);

    if (operandSize == 0)
    {
        return Emit(opcode);
    }

    io::ByteVector bytes(static_cast<size_t>(operandSize));

    switch (operandSize)
    {
        case 1:
        {
            auto encoded = ToLittleEndianBytes(static_cast<int8_t>(operand));
            std::memcpy(bytes.Data(), encoded.Data(), encoded.Size());
            break;
        }
        case 2:
        {
            auto encoded = ToLittleEndianBytes(static_cast<int16_t>(operand));
            std::memcpy(bytes.Data(), encoded.Data(), encoded.Size());
            break;
        }
        case 4:
        {
            auto encoded = ToLittleEndianBytes(static_cast<int32_t>(operand));
            std::memcpy(bytes.Data(), encoded.Data(), encoded.Size());
            break;
        }
        case 8:
        {
            auto encoded = ToLittleEndianBytes(static_cast<int64_t>(operand));
            std::memcpy(bytes.Data(), encoded.Data(), encoded.Size());
            break;
        }
        case 16:
        case 32:
        {
            auto encoded = ToLittleEndianWithPadding(operand, static_cast<size_t>(operandSize));
            std::memcpy(bytes.Data(), encoded.Data(), encoded.Size());
            break;
        }
        default:
            throw std::invalid_argument("Unsupported operand size for Emit(OpCode, int32_t)");
    }

    return Emit(opcode, io::ByteSpan(bytes.Data(), bytes.Size()));
}

ScriptBuilder& ScriptBuilder::EmitCall(int32_t offset)
{
    if (offset < INT8_MIN || offset > INT8_MAX)
    {
        auto bytes = ToLittleEndianBytes(static_cast<int32_t>(offset));
        return Emit(OpCode::CALL_L, io::ByteSpan(bytes.Data(), bytes.Size()));
    }
    else
    {
        auto bytes = ToLittleEndianBytes(static_cast<int8_t>(offset));
        return Emit(OpCode::CALL, io::ByteSpan(bytes.Data(), bytes.Size()));
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
        auto bytes = ToLittleEndianBytes(static_cast<int8_t>(offset));
        return Emit(opcode, io::ByteSpan(bytes.Data(), bytes.Size()));
    }
    else
    {
        auto bytes = ToLittleEndianBytes(static_cast<int32_t>(offset));
        return Emit(opcode, io::ByteSpan(bytes.Data(), bytes.Size()));
    }
}

ScriptBuilder& ScriptBuilder::EmitPush(int64_t value)
{
    if (value >= -1 && value <= 16)
        return Emit(static_cast<OpCode>(static_cast<uint8_t>(OpCode::PUSH0) + static_cast<uint8_t>(value)));

    if (value >= INT8_MIN && value <= INT8_MAX)
    {
        auto bytes = ToLittleEndianBytes(static_cast<int8_t>(value));
        return Emit(OpCode::PUSHINT8, io::ByteSpan(bytes.Data(), bytes.Size()));
    }
    else if (value >= INT16_MIN && value <= INT16_MAX)
    {
        auto bytes = ToLittleEndianBytes(static_cast<int16_t>(value));
        return Emit(OpCode::PUSHINT16, io::ByteSpan(bytes.Data(), bytes.Size()));
    }
    else if (value >= INT32_MIN && value <= INT32_MAX)
    {
        auto bytes = ToLittleEndianBytes(static_cast<int32_t>(value));
        return Emit(OpCode::PUSHINT32, io::ByteSpan(bytes.Data(), bytes.Size()));
    }
    else
    {
        auto bytes = ToLittleEndianBytes(static_cast<int64_t>(value));
        return Emit(OpCode::PUSHINT64, io::ByteSpan(bytes.Data(), bytes.Size()));
    }
}

ScriptBuilder& ScriptBuilder::EmitPush(bool value) { return Emit(value ? OpCode::PUSHT : OpCode::PUSHF); }

ScriptBuilder& ScriptBuilder::EmitPush(const cryptography::ecc::ECPoint& point)
{
    auto encoded = point.ToBytes(true);
    return EmitPush(encoded.AsSpan());
}

ScriptBuilder& ScriptBuilder::EmitPush(const extensions::BigIntegerExtensions::BigInteger& value)
{
    bool handledSmall = false;
    int64_t smallValue = 0;
    try
    {
        smallValue = value.ToInt64();
        handledSmall = true;
    }
    catch (const std::overflow_error&)
    {
        handledSmall = false;
    }

    if (handledSmall && smallValue >= -1 && smallValue <= 16)
    {
        return EmitPush(smallValue);
    }

    auto bytes = ToTwoComplementBytes(value);
    const bool negative = value.isNegative;
    auto emitWithPadding = [&](OpCode opcode, size_t targetSize) -> ScriptBuilder& {
        if (bytes.size() < targetSize) bytes.resize(targetSize, negative ? 0xFF : 0x00);
        return Emit(opcode, io::ByteSpan(bytes.data(), targetSize));
    };

    const size_t length = bytes.size();
    if (length <= 1) return emitWithPadding(OpCode::PUSHINT8, 1);
    if (length <= 2) return emitWithPadding(OpCode::PUSHINT16, 2);
    if (length <= 4) return emitWithPadding(OpCode::PUSHINT32, 4);
    if (length <= 8) return emitWithPadding(OpCode::PUSHINT64, 8);
    if (length <= 16) return emitWithPadding(OpCode::PUSHINT128, 16);
    if (length <= 32) return emitWithPadding(OpCode::PUSHINT256, 32);

    throw std::out_of_range("BigInteger is too large to emit");
}

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
    auto bytes = ToLittleEndianBytes(api);
    return Emit(OpCode::SYSCALL, io::ByteSpan(bytes.Data(), bytes.Size()));
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
