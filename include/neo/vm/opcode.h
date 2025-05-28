#pragma once

#include <cstdint>
#include <string>

namespace neo::vm
{
    /**
     * @brief Represents the operation codes of the Neo virtual machine.
     * This enum exactly matches the C# implementation.
     */
    enum class OpCode : uint8_t
    {
        // Constants
        PUSHINT8 = 0x00,    // Pushes a 1-byte signed integer onto the stack.
        PUSHINT16 = 0x01,   // Pushes a 2-bytes signed integer onto the stack.
        PUSHINT32 = 0x02,   // Pushes a 4-bytes signed integer onto the stack.
        PUSHINT64 = 0x03,   // Pushes a 8-bytes signed integer onto the stack.
        PUSHINT128 = 0x04,  // Pushes a 16-bytes signed integer onto the stack.
        PUSHINT256 = 0x05,  // Pushes a 32-bytes signed integer onto the stack.
        PUSHA = 0x0A,       // Converts the 4-bytes offset to an address, and pushes it onto the stack.
        PUSHNULL = 0x0B,    // The null reference is pushed onto the stack.
        PUSHDATA1 = 0x0C,   // The next byte contains the number of bytes to be pushed onto the stack.
        PUSHDATA2 = 0x0D,   // The next two bytes contain the number of bytes to be pushed onto the stack.
        PUSHDATA4 = 0x0E,   // The next four bytes contain the number of bytes to be pushed onto the stack.
        PUSHM1 = 0x0F,      // The number -1 is pushed onto the stack.
        PUSH0 = 0x10,       // The number 0 is pushed onto the stack.
        PUSH1 = 0x11,       // The number 1 is pushed onto the stack.
        PUSH2 = 0x12,       // The number 2 is pushed onto the stack.
        PUSH3 = 0x13,       // The number 3 is pushed onto the stack.
        PUSH4 = 0x14,       // The number 4 is pushed onto the stack.
        PUSH5 = 0x15,       // The number 5 is pushed onto the stack.
        PUSH6 = 0x16,       // The number 6 is pushed onto the stack.
        PUSH7 = 0x17,       // The number 7 is pushed onto the stack.
        PUSH8 = 0x18,       // The number 8 is pushed onto the stack.
        PUSH9 = 0x19,       // The number 9 is pushed onto the stack.
        PUSH10 = 0x1A,      // The number 10 is pushed onto the stack.
        PUSH11 = 0x1B,      // The number 11 is pushed onto the stack.
        PUSH12 = 0x1C,      // The number 12 is pushed onto the stack.
        PUSH13 = 0x1D,      // The number 13 is pushed onto the stack.
        PUSH14 = 0x1E,      // The number 14 is pushed onto the stack.
        PUSH15 = 0x1F,      // The number 15 is pushed onto the stack.
        PUSH16 = 0x20,      // The number 16 is pushed onto the stack.

        // Boolean constants
        PUSHT = 0x08,       // Pushes the boolean value True onto the stack.
        PUSHF = 0x09,       // Pushes the boolean value False onto the stack.

        // Flow control
        NOP = 0x21,         // Does nothing.
        JMP = 0x22,         // Unconditionally transfers control to a target instruction.
        JMP_L = 0x23,       // Unconditionally transfers control to a target instruction (4-byte offset).
        JMPIF = 0x24,       // Transfers control to a target instruction if the top stack value is True.
        JMPIF_L = 0x25,     // Transfers control to a target instruction if the top stack value is True (4-byte offset).
        JMPIFNOT = 0x26,    // Transfers control to a target instruction if the top stack value is False.
        JMPIFNOT_L = 0x27,  // Transfers control to a target instruction if the top stack value is False (4-byte offset).
        JMPEQ = 0x28,       // Transfers control to a target instruction if two values are equal.
        JMPEQ_L = 0x29,     // Transfers control to a target instruction if two values are equal (4-byte offset).
        JMPNE = 0x2A,       // Transfers control to a target instruction when two values are not equal.
        JMPNE_L = 0x2B,     // Transfers control to a target instruction when two values are not equal (4-byte offset).
        JMPGT = 0x2C,       // Transfers control to a target instruction if the first value is greater than the second value.
        JMPGT_L = 0x2D,     // Transfers control to a target instruction if the first value is greater than the second value (4-byte offset).
        JMPGE = 0x2E,       // Transfers control to a target instruction if the first value is greater than or equal to the second value.
        JMPGE_L = 0x2F,     // Transfers control to a target instruction if the first value is greater than or equal to the second value (4-byte offset).
        JMPLT = 0x30,       // Transfers control to a target instruction if the first value is less than the second value.
        JMPLT_L = 0x31,     // Transfers control to a target instruction if the first value is less than the second value (4-byte offset).
        JMPLE = 0x32,       // Transfers control to a target instruction if the first value is less than or equal to the second value.
        JMPLE_L = 0x33,     // Transfers control to a target instruction if the first value is less than or equal to the second value (4-byte offset).
        CALL = 0x34,        // Calls the function at the target address.
        CALL_L = 0x35,      // Calls the function at the target address (4-byte offset).
        CALLA = 0x36,       // Pop the address of a function from the stack, and call the function.
        CALLT = 0x37,       // Calls the function which is described by the token.
        ABORT = 0x38,       // It turns the vm state to FAULT immediately, and cannot be caught.
        ASSERT = 0x39,      // Pop the top value of the stack. If it's false, exit vm execution and set vm state to FAULT.
        THROW = 0x3A,       // Pop the top value of the stack, and throw it.
        TRY = 0x3B,         // TRY CatchOffset(sbyte) FinallyOffset(sbyte).
        TRY_L = 0x3C,       // TRY_L CatchOffset(int) FinallyOffset(int).
        ENDTRY = 0x3D,      // Ensures that the appropriate surrounding finally blocks are executed.
        ENDTRY_L = 0x3E,    // Ensures that the appropriate surrounding finally blocks are executed (4-byte offset).
        ENDFINALLY = 0x3F,  // End finally.
        RET = 0x40,         // Returns from the current method.
        SYSCALL = 0x41,     // Calls to an interop service.
        LEAVE = 0x42,       // Exits a try block and jumps to the specified target instruction (1-byte offset).
        LEAVE_L = 0x44,     // Exits a try block and jumps to the specified target instruction (4-byte offset).

        // Stack
        DEPTH = 0x43,       // Puts the number of stack items onto the stack.
        DROP = 0x45,        // Removes the top stack item.
        NIP = 0x46,         // Removes the second-to-top stack item.
        XDROP = 0x48,       // The item n back in the main stack is removed.
        CLEAR = 0x49,       // Clear the stack.
        DUP = 0x4A,         // Duplicates the top stack item.
        OVER = 0x4B,        // Copies the second-to-top stack item to the top.
        PICK = 0x4D,        // The item n back in the stack is copied to the top.
        TUCK = 0x4E,        // The item at the top of the stack is copied and inserted before the second-to-top item.
        SWAP = 0x50,        // The top two items on the stack are swapped.
        ROT = 0x51,         // The top three items on the stack are rotated to the left.
        ROLL = 0x52,        // The item n back in the stack is moved to the top.
        REVERSE3 = 0x53,    // Reverse the order of the top 3 items on the stack.
        REVERSE4 = 0x54,    // Reverse the order of the top 4 items on the stack.
        REVERSEN = 0x55,    // Pop the number N on the stack, and reverse the order of the top N items on the stack.

        // Slot
        INITSSLOT = 0x56,   // Initialize the static field list for the current execution context.
        INITSLOT = 0x57,    // Initialize the argument slot and the local variable list for the current execution context.
        LDSFLD0 = 0x58,     // Loads the static field at index 0 onto the evaluation stack.
        LDSFLD1 = 0x59,     // Loads the static field at index 1 onto the evaluation stack.
        LDSFLD2 = 0x5A,     // Loads the static field at index 2 onto the evaluation stack.
        LDSFLD3 = 0x5B,     // Loads the static field at index 3 onto the evaluation stack.
        LDSFLD4 = 0x5C,     // Loads the static field at index 4 onto the evaluation stack.
        LDSFLD5 = 0x5D,     // Loads the static field at index 5 onto the evaluation stack.
        LDSFLD6 = 0x5E,     // Loads the static field at index 6 onto the evaluation stack.
        LDSFLD = 0x5F,      // Loads the static field at a specified index onto the evaluation stack.
        STSFLD0 = 0x60,     // Stores the value on top of the evaluation stack in the static field list at index 0.
        STSFLD1 = 0x61,     // Stores the value on top of the evaluation stack in the static field list at index 1.
        STSFLD2 = 0x62,     // Stores the value on top of the evaluation stack in the static field list at index 2.
        STSFLD3 = 0x63,     // Stores the value on top of the evaluation stack in the static field list at index 3.
        STSFLD4 = 0x64,     // Stores the value on top of the evaluation stack in the static field list at index 4.
        STSFLD5 = 0x65,     // Stores the value on top of the evaluation stack in the static field list at index 5.
        STSFLD6 = 0x66,     // Stores the value on top of the evaluation stack in the static field list at index 6.
        STSFLD = 0x67,      // Stores the value on top of the evaluation stack in the static field list at a specified index.
        LDLOC0 = 0x68,      // Loads the local variable at index 0 onto the evaluation stack.
        LDLOC1 = 0x69,      // Loads the local variable at index 1 onto the evaluation stack.
        LDLOC2 = 0x6A,      // Loads the local variable at index 2 onto the evaluation stack.
        LDLOC3 = 0x6B,      // Loads the local variable at index 3 onto the evaluation stack.
        LDLOC4 = 0x6C,      // Loads the local variable at index 4 onto the evaluation stack.
        LDLOC5 = 0x6D,      // Loads the local variable at index 5 onto the evaluation stack.
        LDLOC6 = 0x6E,      // Loads the local variable at index 6 onto the evaluation stack.
        LDLOC = 0x6F,       // Loads the local variable at a specified index onto the evaluation stack.
        STLOC0 = 0x70,      // Stores the value on top of the evaluation stack in the local variable list at index 0.
        STLOC1 = 0x71,      // Stores the value on top of the evaluation stack in the local variable list at index 1.
        STLOC2 = 0x72,      // Stores the value on top of the evaluation stack in the local variable list at index 2.
        STLOC3 = 0x73,      // Stores the value on top of the evaluation stack in the local variable list at index 3.
        STLOC4 = 0x74,      // Stores the value on top of the evaluation stack in the local variable list at index 4.
        STLOC5 = 0x75,      // Stores the value on top of the evaluation stack in the local variable list at index 5.
        STLOC6 = 0x76,      // Stores the value on top of the evaluation stack in the local variable list at index 6.
        STLOC = 0x77,       // Stores the value on top of the evaluation stack in the local variable list at a specified index.
        LDARG0 = 0x78,      // Loads the argument at index 0 onto the evaluation stack.
        LDARG1 = 0x79,      // Loads the argument at index 1 onto the evaluation stack.
        LDARG2 = 0x7A,      // Loads the argument at index 2 onto the evaluation stack.
        LDARG3 = 0x7B,      // Loads the argument at index 3 onto the evaluation stack.
        LDARG4 = 0x7C,      // Loads the argument at index 4 onto the evaluation stack.
        LDARG5 = 0x7D,      // Loads the argument at index 5 onto the evaluation stack.
        LDARG6 = 0x7E,      // Loads the argument at index 6 onto the evaluation stack.
        LDARG = 0x7F,       // Loads the argument at a specified index onto the evaluation stack.
        STARG0 = 0x80,      // Stores the value on top of the evaluation stack in the argument slot at index 0.
        STARG1 = 0x81,      // Stores the value on top of the evaluation stack in the argument slot at index 1.
        STARG2 = 0x82,      // Stores the value on top of the evaluation stack in the argument slot at index 2.
        STARG3 = 0x83,      // Stores the value on top of the evaluation stack in the argument slot at index 3.
        STARG4 = 0x84,      // Stores the value on top of the evaluation stack in the argument slot at index 4.
        STARG5 = 0x85,      // Stores the value on top of the evaluation stack in the argument slot at index 5.
        STARG6 = 0x86,      // Stores the value on top of the evaluation stack in the argument slot at index 6.
        STARG = 0x87,       // Stores the value on top of the evaluation stack in the argument slot at a specified index.

        // Splice
        NEWBUFFER = 0x88,   // Creates a new buffer and pushes it onto the stack.
        MEMCPY = 0x89,      // Copies a range of bytes from one buffer to another.
        CAT = 0x8B,         // Concatenates two strings.
        SUBSTR = 0x8C,      // Returns a section of a string.
        LEFT = 0x8D,        // Keeps only characters left of the specified point in a string.
        RIGHT = 0x8E,       // Keeps only characters right of the specified point in a string.

        // Bitwise logic
        INVERT = 0x90,      // Flips all of the bits in the input.
        AND = 0x91,         // Boolean and between each bit in the inputs.
        OR = 0x92,          // Boolean or between each bit in the inputs.
        XOR = 0x93,         // Boolean exclusive or between each bit in the inputs.
        EQUAL = 0x97,       // Returns 1 if the inputs are exactly equal, 0 otherwise.
        NOTEQUAL = 0x98,    // Returns 1 if the inputs are not equal, 0 otherwise.

        // Arithmetic
        SIGN = 0x99,        // Puts the sign of top stack item on top of the main stack.
        ABS = 0x9A,         // The input is made positive.
        NEGATE = 0x9B,      // The sign of the input is flipped.
        INC = 0x9C,         // 1 is added to the input.
        DEC = 0x9D,         // 1 is subtracted from the input.
        ADD = 0x9E,         // a is added to b.
        SUB = 0x9F,         // b is subtracted from a.
        MUL = 0xA0,         // a is multiplied by b.
        DIV = 0xA1,         // a is divided by b.
        MOD = 0xA2,         // Returns the remainder after dividing a by b.
        POW = 0xA3,         // The result of raising value to the exponent power.
        SQRT = 0xA4,        // Returns the square root of a specified number.
        MODMUL = 0xA5,      // Performs modulus division on a number multiplied by another number.
        MODPOW = 0xA6,      // Performs modulus division on a number raised to the power of another number.
        SHL = 0xA8,         // Shifts a left b bits, preserving sign.
        SHR = 0xA9,         // Shifts a right b bits, preserving sign.
        NOT = 0xAA,         // If the input is 0 or 1, it is flipped. Otherwise the output will be 0.
        BOOLAND = 0xAB,     // If both a and b are not 0, the output is 1. Otherwise 0.
        BOOLOR = 0xAC,      // If a or b is not 0, the output is 1. Otherwise 0.
        NZ = 0xB1,          // Returns 0 if the input is 0. 1 otherwise.
        NUMEQUAL = 0xB3,    // Returns 1 if the numbers are equal, 0 otherwise.
        NUMNOTEQUAL = 0xB4, // Returns 1 if the numbers are not equal, 0 otherwise.
        LT = 0xB5,          // Returns 1 if a is less than b, 0 otherwise.
        LE = 0xB6,          // Returns 1 if a is less than or equal to b, 0 otherwise.
        GT = 0xB7,          // Returns 1 if a is greater than b, 0 otherwise.
        GE = 0xB8,          // Returns 1 if a is greater than or equal to b, 0 otherwise.
        MIN = 0xB9,         // Returns the smaller of a and b.
        MAX = 0xBA,         // Returns the larger of a and b.
        WITHIN = 0xBB,      // Returns 1 if x is within the specified range (left-inclusive), 0 otherwise.

        // Compound-type
        PACKMAP = 0xBE,     // A value n is taken from top of main stack. The next n*2 items on main stack are removed, put inside n-sized map and this map is put on top of the main stack.
        PACKSTRUCT = 0xBF,  // A value n is taken from top of main stack. The next n items on main stack are removed, put inside n-sized struct and this struct is put on top of the main stack.
        PACK = 0xC0,        // A value n is taken from top of main stack. The next n items on main stack are removed, put inside n-sized array and this array is put on top of the main stack.
        UNPACK = 0xC1,      // A collection is removed from top of the main stack. Its elements are put on top of the main stack (in reverse order) and the collection size is also put on main stack.
        NEWARRAY0 = 0xC2,   // An empty array (with size 0) is put on top of the main stack.
        NEWARRAY = 0xC3,    // A value n is taken from top of main stack. A null-filled array with size n is put on top of the main stack.
        NEWARRAY_T = 0xC4,  // A value n is taken from top of main stack. An array of type T with size n is put on top of the main stack.
        NEWSTRUCT0 = 0xC5,  // An empty struct (with size 0) is put on top of the main stack.
        NEWSTRUCT = 0xC6,   // A value n is taken from top of main stack. A zero-filled struct with size n is put on top of the main stack.
        NEWMAP = 0xC8,      // A Map is created and put on top of the main stack.
        SIZE = 0xCA,        // An array is removed from top of the main stack. Its size is put on top of the main stack.
        HASKEY = 0xCB,      // An input index n (or key) and an array (or map) are removed from the top of the main stack. Puts True on top of main stack if array[n] (or map[n]) exist, and False otherwise.
        KEYS = 0xCC,        // A map is taken from top of the main stack. The keys of this map are put on top of the main stack.
        VALUES = 0xCD,      // A map is taken from top of the main stack. The values of this map are put on top of the main stack.
        PICKITEM = 0xCE,    // An input index n (or key) and an array (or map) are taken from main stack. Element array[n] (or map[n]) is put on top of the main stack.
        APPEND = 0xCF,      // The item on top of main stack is removed and appended to the second item on top of the main stack.
        SETITEM = 0xD0,     // A value v, index n (or key) and an array (or map) are taken from main stack. Attribution array[n]=v (or map[n]=v) is performed.
        REVERSEITEMS = 0xD1,// An array is removed from the top of the main stack and its elements are reversed.
        REMOVE = 0xD2,      // An input index n (or key) and an array (or map) are removed from the top of the main stack. Element array[n] (or map[n]) is removed.
        CLEARITEMS = 0xD3,  // Remove all the items from the compound-type.
        POPITEM = 0xD4,     // Remove the last element from an array, and push it onto the stack.

        // Types
        ISNULL = 0xD8,      // Returns True if the input is null; otherwise, False.
        ISTYPE = 0xD9,      // Returns True if the top item of the stack is of the specified type; otherwise, False.
        CONVERT = 0xDB,     // Converts the top item of the stack to the specified type.

        // Extensions
        ABORTMSG = 0xE0,    // Pops the top stack item. Then, turns the vm state to FAULT immediately, and cannot be caught. The top stack value is used as reason.
        ASSERTMSG = 0xE1    // Pops the top two stack items. If the second-to-top stack value is false, exits the vm execution and sets the vm state to FAULT. In this case, the top stack value is used as reason for the exit. Otherwise, it is ignored.
    };

    /**
     * @brief Gets the name of an OpCode.
     * @param opcode The OpCode.
     * @return The name of the OpCode.
     */
    std::string GetOpCodeName(OpCode opcode);

    /**
     * @brief Checks if an OpCode is a push operation.
     * @param opcode The OpCode.
     * @return True if the OpCode is a push operation, false otherwise.
     */
    bool IsPushOp(OpCode opcode);

    /**
     * @brief Gets the size of the operand for a push operation.
     * @param opcode The OpCode.
     * @return The size of the operand, or -1 if the OpCode is not a push operation.
     */
    int GetPushOpOperandSize(OpCode opcode);
}
