#pragma once

#include <neo/vm/script_builder.h>
#include <neo/vm/opcode.h>
#include <neo/vm/instruction.h>
#include <neo/vm/internal/byte_vector.h>
#include <neo/io/byte_span.h>
#include <string>
#include <vector>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace neo::vm::tests {

/**
 * @brief Utility class for converting between scripts and JSON
 */
class ScriptConverter {
public:
    /**
     * @brief Converts a JSON array to a script
     * @param json The JSON array
     * @return The script as a byte array
     */
    static std::vector<uint8_t> FromJson(const nlohmann::json& json) {
        if (!json.is_array()) {
            throw std::invalid_argument("JSON must be an array");
        }

        ScriptBuilder script;
        
        for (const auto& entry : json) {
            if (entry.is_string()) {
                std::string value = entry.get<std::string>();
                
                // Check if it's an opcode or hex string
                if (value.size() >= 2 && value[0] == '0' && value[1] == 'x') {
                    // Hex string - convert and emit raw bytes
                    auto bytes = HexStringToBytes(value.substr(2));
                    script.EmitRaw(io::ByteSpan(bytes.data(), bytes.size()));
                } else {
                    // Try to parse as opcode
                    OpCode opCode = ParseOpCode(value);
                    script.Emit(opCode);
                }
            }
        }
        
        return script.ToArray();
    }
    
    /**
     * @brief Converts a script to a JSON array
     * @param script The script as a byte array
     * @return The JSON array representing the script
     */
    static nlohmann::json ToJson(const std::vector<uint8_t>& script) {
        nlohmann::json array = nlohmann::json::array();
        
        try {
            // Convert std::vector to internal::ByteVector
            neo::vm::internal::ByteVector internalScript;
            internalScript.Reserve(script.size());
            for (uint8_t byte : script) {
                internalScript.Push(byte);
            }
            
            size_t ip = 0;
            while (ip < script.size()) {
                Instruction instruction(internalScript.AsSpan(), static_cast<int>(ip));
                
                // Add opcode name
                array.push_back(OpCodeToString(instruction.opcode));
                
                ip += instruction.Size();
            }
        } catch (const std::exception&) {
            // If parsing fails, just add remaining bytes as hex
            if (script.size() > 0) {
                array.push_back("0x" + BytesToHexString(script));
            }
        }
        
        return array;
    }

private:
    /**
     * @brief Converts a hex string to bytes
     * @param hex Hex string without '0x' prefix
     * @return Byte array
     */
    static std::vector<uint8_t> HexStringToBytes(const std::string& hex) {
        std::vector<uint8_t> bytes;
        for (size_t i = 0; i < hex.length(); i += 2) {
            std::string byteString = hex.substr(i, 2);
            bytes.push_back(static_cast<uint8_t>(std::stoi(byteString, nullptr, 16)));
        }
        return bytes;
    }
    
    /**
     * @brief Converts bytes to a hex string
     * @param bytes Byte array
     * @return Hex string without '0x' prefix
     */
    static std::string BytesToHexString(const std::vector<uint8_t>& bytes) {
        static const char* hexChars = "0123456789ABCDEF";
        std::string result;
        result.reserve(bytes.size() * 2);
        
        for (uint8_t byte : bytes) {
            result.push_back(hexChars[byte >> 4]);
            result.push_back(hexChars[byte & 0x0F]);
        }
        
        return result;
    }
    
    /**
     * @brief Parses a string to an OpCode
     * @param value String representation of the opcode
     * @return The OpCode
     */
    static OpCode ParseOpCode(const std::string& value) {
        // Convert to uppercase for case-insensitive comparison
        std::string upperValue = value;
        std::transform(upperValue.begin(), upperValue.end(), upperValue.begin(), 
                      [](unsigned char c) { return std::toupper(c); });
        
        // Map opcode string to OpCode enum value
        if (upperValue == "NOP") return OpCode::NOP;
        else if (upperValue == "PUSH0") return OpCode::PUSH0;
        else if (upperValue == "PUSHDATA1") return OpCode::PUSHDATA1;
        else if (upperValue == "PUSHDATA2") return OpCode::PUSHDATA2;
        else if (upperValue == "PUSHDATA4") return OpCode::PUSHDATA4;
        else if (upperValue == "PUSHM1") return OpCode::PUSHM1;
        else if (upperValue == "PUSH1") return OpCode::PUSH1;
        else if (upperValue == "PUSH2") return OpCode::PUSH2;
        else if (upperValue == "PUSH3") return OpCode::PUSH3;
        else if (upperValue == "PUSH4") return OpCode::PUSH4;
        else if (upperValue == "PUSH5") return OpCode::PUSH5;
        else if (upperValue == "PUSH6") return OpCode::PUSH6;
        else if (upperValue == "PUSH7") return OpCode::PUSH7;
        else if (upperValue == "PUSH8") return OpCode::PUSH8;
        else if (upperValue == "PUSH9") return OpCode::PUSH9;
        else if (upperValue == "PUSH10") return OpCode::PUSH10;
        else if (upperValue == "PUSH11") return OpCode::PUSH11;
        else if (upperValue == "PUSH12") return OpCode::PUSH12;
        else if (upperValue == "PUSH13") return OpCode::PUSH13;
        else if (upperValue == "PUSH14") return OpCode::PUSH14;
        else if (upperValue == "PUSH15") return OpCode::PUSH15;
        else if (upperValue == "PUSH16") return OpCode::PUSH16;
        else if (upperValue == "ADD") return OpCode::ADD;
        
        // Add more opcodes as needed, or implement a more comprehensive mapping
        
        throw std::invalid_argument("Unknown opcode: " + value);
    }
    
    /**
     * @brief Converts an OpCode to its string representation
     * @param opCode The OpCode
     * @return The string representation
     */
    static std::string OpCodeToString(OpCode opCode) {
        switch (opCode) {
            case OpCode::NOP: return "NOP";
            case OpCode::PUSH0: return "PUSH0";
            case OpCode::PUSHDATA1: return "PUSHDATA1";
            case OpCode::PUSHDATA2: return "PUSHDATA2";
            case OpCode::PUSHDATA4: return "PUSHDATA4";
            case OpCode::PUSHM1: return "PUSHM1";
            case OpCode::PUSH1: return "PUSH1";
            case OpCode::PUSH2: return "PUSH2";
            case OpCode::PUSH3: return "PUSH3";
            case OpCode::PUSH4: return "PUSH4";
            case OpCode::PUSH5: return "PUSH5";
            case OpCode::PUSH6: return "PUSH6";
            case OpCode::PUSH7: return "PUSH7";
            case OpCode::PUSH8: return "PUSH8";
            case OpCode::PUSH9: return "PUSH9";
            case OpCode::PUSH10: return "PUSH10";
            case OpCode::PUSH11: return "PUSH11";
            case OpCode::PUSH12: return "PUSH12";
            case OpCode::PUSH13: return "PUSH13";
            case OpCode::PUSH14: return "PUSH14";
            case OpCode::PUSH15: return "PUSH15";
            case OpCode::PUSH16: return "PUSH16";
            case OpCode::ADD: return "ADD";
            
            // Add more cases as needed, or implement a more comprehensive mapping
            
            default: return "UNKNOWN";
        }
    }
};

} // namespace neo::vm::tests
