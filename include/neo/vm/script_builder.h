#pragma once

#include <cstdint>
#include <memory>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_span.h>
#include <neo/io/byte_vector.h>
#include <neo/vm/internal/byte_vector.h>
#include <neo/vm/opcode.h>
#include <neo/vm/script.h>
#include <sstream>
#include <string>
#include <vector>

namespace neo::vm
{
/**
 * @brief A helper class for building scripts.
 */
class ScriptBuilder
{
  public:
    /**
     * @brief Initializes a new instance of the ScriptBuilder class.
     * @param initialCapacity The initial capacity of the script.
     */
    explicit ScriptBuilder(size_t initialCapacity = 0);

    /**
     * @brief Emits an instruction with the specified OpCode and operand.
     * @param opcode The OpCode to be emitted.
     * @param operand The operand to be emitted.
     * @return A reference to this instance after the emit operation has completed.
     */
    ScriptBuilder& Emit(OpCode opcode, const io::ByteSpan& operand = io::ByteSpan());

    /**
     * @brief Emits a call instruction with the specified offset.
     * @param offset The offset to be called.
     * @return A reference to this instance after the emit operation has completed.
     */
    ScriptBuilder& EmitCall(int32_t offset);

    /**
     * @brief Emits a jump instruction with the specified offset.
     * @param opcode The OpCode to be emitted. It must be a jump OpCode.
     * @param offset The offset to jump.
     * @return A reference to this instance after the emit operation has completed.
     */
    ScriptBuilder& EmitJump(OpCode opcode, int32_t offset);

    /**
     * @brief Emits a push instruction with the specified number.
     * @param value The number to be pushed.
     * @return A reference to this instance after the emit operation has completed.
     */
    ScriptBuilder& EmitPush(int64_t value);

    /**
     * @brief Emits a push instruction with the specified boolean value.
     * @param value The value to be pushed.
     * @return A reference to this instance after the emit operation has completed.
     */
    ScriptBuilder& EmitPush(bool value);

    /**
     * @brief Emits a push instruction with the specified data.
     * @param data The data to be pushed.
     * @return A reference to this instance after the emit operation has completed.
     */
    ScriptBuilder& EmitPush(const io::ByteSpan& data);

    /**
     * @brief Emits a push instruction with the specified string.
     * @param data The string to be pushed.
     * @return A reference to this instance after the emit operation has completed.
     */
    ScriptBuilder& EmitPush(const std::string& data);

    /**
     * @brief Emits a push instruction with the specified string literal.
     * @param data The string literal to be pushed.
     * @return A reference to this instance after the emit operation has completed.
     */
    ScriptBuilder& EmitPush(const char* data);

    /**
     * @brief Emits raw script.
     * @param script The raw script to be emitted.
     * @return A reference to this instance after the emit operation has completed.
     */
    ScriptBuilder& EmitRaw(const io::ByteSpan& script = io::ByteSpan());

    /**
     * @brief Emits an instruction with OpCode.SYSCALL.
     * @param api The operand of OpCode.SYSCALL.
     * @return A reference to this instance after the emit operation has completed.
     */
    ScriptBuilder& EmitSysCall(uint32_t api);

    /**
     * @brief Emits an instruction with OpCode.SYSCALL.
     * @param api The system call name.
     * @return A reference to this instance after the emit operation has completed.
     */
    ScriptBuilder& EmitSysCall(const std::string& api);

    /**
     * @brief Converts the value of this instance to a byte vector.
     * @return A byte vector contains the script.
     */
    io::ByteVector ToArray() const;

    /**
     * @brief Creates a Script from the current state of the ScriptBuilder.
     * @return A Script object.
     */
    Script ToScript() const;

    /**
     * @brief Emits a push instruction with the specified data (alias for EmitPush).
     * @param data The data to be pushed.
     * @return A reference to this instance after the emit operation has completed.
     */
    ScriptBuilder& EmitPushData(const io::ByteSpan& data)
    {
        return EmitPush(data);
    }

    /**
     * @brief Emits a push instruction with the specified data (alias for EmitPush).
     * @param data The data to be pushed.
     * @return A reference to this instance after the emit operation has completed.
     */
    ScriptBuilder& EmitPushData(const io::ByteVector& data)
    {
        return EmitPush(data.AsSpan());
    }

    /**
     * @brief Emits a push instruction with the specified number (alias for EmitPush).
     * @param value The number to be pushed.
     * @return A reference to this instance after the emit operation has completed.
     */
    ScriptBuilder& EmitPushNumber(int64_t value)
    {
        return EmitPush(value);
    }

    /**
     * @brief Emits a push instruction with the specified number (alias for EmitPush).
     * @param value The number to be pushed.
     * @return A reference to this instance after the emit operation has completed.
     */
    ScriptBuilder& EmitPushNumber(int32_t value)
    {
        return EmitPush(static_cast<int64_t>(value));
    }

    /**
     * @brief Emits a push instruction with the specified number (alias for EmitPush).
     * @param value The number to be pushed.
     * @return A reference to this instance after the emit operation has completed.
     */
    ScriptBuilder& EmitPushNumber(size_t value)
    {
        return EmitPush(static_cast<int64_t>(value));
    }

  private:
    io::ByteVector script_;
    std::shared_ptr<std::stringstream> stream_;
    std::unique_ptr<io::BinaryWriter> writer_;
};
}  // namespace neo::vm
