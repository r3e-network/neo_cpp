#pragma once

#include <cstdint>
#include <memory>
#include <neo/vm/instruction.h>
#include <neo/vm/internal/byte_span.h>
#include <neo/vm/internal/byte_vector.h>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace neo::vm
{

/**
 * @brief Represents a script.
 */
class Script
{
  public:
    /**
     * @brief Constructs an empty Script.
     */
    Script();

    /**
     * @brief Constructs a Script from a byte array.
     * @param script The script.
     */
    explicit Script(const internal::ByteVector& script);

    /**
     * @brief Constructs a Script from a byte span.
     * @param script The script.
     */
    explicit Script(const internal::ByteSpan& script);

    /**
     * @brief Gets the script.
     * @return The script.
     */
    const internal::ByteVector& GetScript() const;

    /**
     * @brief Gets the length of the script.
     * @return The length of the script.
     */
    size_t GetLength() const;

    /**
     * @brief Gets the instruction at the specified position.
     * @param position The position.
     * @return The instruction.
     */
    std::shared_ptr<Instruction> GetInstruction(int32_t position) const;

    /**
     * @brief Gets the next instruction.
     * @param position The position.
     * @return The next instruction.
     */
    std::shared_ptr<Instruction> GetNextInstruction(int32_t& position) const;

    /**
     * @brief Gets the jump destination.
     * @param position The position.
     * @param offset The offset.
     * @return The jump destination.
     */
    int32_t GetJumpDestination(int32_t position, int32_t offset) const;

    /**
     * @brief Checks if this Script is equal to another Script.
     * @param other The other Script.
     * @return True if the Scripts are equal, false otherwise.
     */
    bool operator==(const Script& other) const;

    /**
     * @brief Checks if this Script is not equal to another Script.
     * @param other The other Script.
     * @return True if the Scripts are not equal, false otherwise.
     */
    bool operator!=(const Script& other) const;

    /**
     * @brief Gets the operand size for the specified opcode.
     * @param opcode The opcode.
     * @return The operand size.
     */
    static int32_t GetOperandSize(OpCode opcode);

    /**
     * @brief Gets the price for the specified opcode.
     * @param opcode The opcode.
     * @return The price.
     */
    static int64_t GetPrice(OpCode opcode);

    /**
     * @brief Gets the name of the specified opcode.
     * @param opcode The opcode.
     * @return The name.
     */
    static std::string GetOpCodeName(OpCode opcode);

  private:
    internal::ByteVector script_;
    mutable std::unordered_map<int32_t, std::shared_ptr<Instruction>> instructions_;
};
}  // namespace neo::vm
