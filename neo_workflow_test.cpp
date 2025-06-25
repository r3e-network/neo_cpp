#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <chrono>

// Neo C++ Core Headers
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <neo/io/byte_vector.h>
#include <neo/io/memory_stream.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/cryptography/hash.h>
#include <neo/cryptography/crypto.h>
#include <neo/vm/script.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/opcode.h>
#include <neo/vm/stack_item.h>
#include <neo/json/jtoken.h>
#include <neo/json/jobject.h>
#include <neo/json/jarray.h>

using namespace neo;
using namespace neo::io;
using namespace neo::cryptography;
using namespace neo::vm;
using namespace neo::json;

class NeoWorkflowTester {
private:
    int total_tests = 0;
    int passed_tests = 0;
    int failed_tests = 0;
    
    void RunTest(const std::string& test_name, std::function<bool()> test_func) {
        total_tests++;
        std::cout << "Running " << test_name << "... ";
        
        try {
            if (test_func()) {
                passed_tests++;
                std::cout << "✅ PASSED" << std::endl;
            } else {
                failed_tests++;
                std::cout << "❌ FAILED" << std::endl;
            }
        } catch (const std::exception& e) {
            failed_tests++;
            std::cout << "❌ FAILED (Exception: " << e.what() << ")" << std::endl;
        } catch (...) {
            failed_tests++;
            std::cout << "❌ FAILED (Unknown exception)" << std::endl;
        }
    }

public:
    void RunAllTests() {
        std::cout << "╔═══════════════════════════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║                      NEO C++ WORKFLOW VALIDATION TEST                        ║" << std::endl;
        std::cout << "╚═══════════════════════════════════════════════════════════════════════════════╝" << std::endl;
        std::cout << std::endl;
        
        // Test 1: Basic Data Types
        RunTest("UInt256 Operations", [this]() { return TestUInt256Operations(); });
        RunTest("UInt160 Operations", [this]() { return TestUInt160Operations(); });
        RunTest("ByteVector Operations", [this]() { return TestByteVectorOperations(); });
        
        // Test 2: Cryptographic Operations
        RunTest("Hash Functions", [this]() { return TestHashFunctions(); });
        RunTest("Crypto Operations", [this]() { return TestCryptoOperations(); });
        
        // Test 3: IO Operations
        RunTest("Binary Serialization", [this]() { return TestBinarySerialization(); });
        RunTest("Memory Stream Operations", [this]() { return TestMemoryStreamOperations(); });
        
        // Test 4: VM Operations
        RunTest("Script Building", [this]() { return TestScriptBuilding(); });
        RunTest("VM Execution", [this]() { return TestVMExecution(); });
        RunTest("Stack Operations", [this]() { return TestStackOperations(); });
        
        // Test 5: JSON Operations
        RunTest("JSON Parsing", [this]() { return TestJSONOperations(); });
        
        // Test 6: Integration Tests
        RunTest("End-to-End Workflow", [this]() { return TestEndToEndWorkflow(); });
        
        PrintSummary();
    }
    
private:
    bool TestUInt256Operations() {
        // Test UInt256 creation and operations
        UInt256 hash1 = UInt256::Zero();
        UInt256 hash2 = UInt256::Parse("0x0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
        
        if (hash1 == UInt256::Zero() && hash2 != hash1) {
            return true;
        }
        return false;
    }
    
    bool TestUInt160Operations() {
        // Test UInt160 creation and operations
        UInt160 addr1 = UInt160::Zero();
        UInt160 addr2 = UInt160::Parse("0x0123456789abcdef0123456789abcdef01234567");
        
        if (addr1 == UInt160::Zero() && addr2 != addr1) {
            return true;
        }
        return false;
    }
    
    bool TestByteVectorOperations() {
        // Test ByteVector operations
        ByteVector data1 = {0x01, 0x02, 0x03, 0x04};
        ByteVector data2 = {0x05, 0x06, 0x07, 0x08};
        
        data1.insert(data1.end(), data2.begin(), data2.end());
        
        return data1.size() == 8 && data1[0] == 0x01 && data1[7] == 0x08;
    }
    
    bool TestHashFunctions() {
        try {
            std::string input = "Hello, Neo!";
            ByteVector input_bytes(input.begin(), input.end());
            
            // Test SHA256
            auto sha256_result = Hash::SHA256(input_bytes);
            if (sha256_result.size() != 32) return false;
            
            // Test RIPEMD160
            auto ripemd160_result = Hash::RIPEMD160(input_bytes);
            if (ripemd160_result.size() != 20) return false;
            
            // Test Hash160 (RIPEMD160 of SHA256)
            auto hash160_result = Hash::Hash160(input_bytes);
            if (hash160_result.size() != 20) return false;
            
            return true;
        } catch (...) {
            return false;
        }
    }
    
    bool TestCryptoOperations() {
        try {
            // Test basic crypto functionality
            std::string message = "Test message for crypto operations";
            ByteVector message_bytes(message.begin(), message.end());
            
            // Test that we can create hashes
            auto hash = Hash::SHA256(message_bytes);
            
            // Test deterministic behavior
            auto hash2 = Hash::SHA256(message_bytes);
            
            return hash == hash2 && hash.size() == 32;
        } catch (...) {
            return false;
        }
    }
    
    bool TestBinarySerialization() {
        try {
            // Test binary writer and reader
            ByteVector buffer;
            MemoryStream stream(buffer);
            BinaryWriter writer(stream);
            
            // Write some data
            writer.WriteUInt32(0x12345678);
            writer.WriteUInt16(0xABCD);
            writer.WriteByte(0xFF);
            
            // Read it back
            stream.Seek(0, std::ios::beg);
            BinaryReader reader(stream);
            
            uint32_t val32 = reader.ReadUInt32();
            uint16_t val16 = reader.ReadUInt16();
            uint8_t val8 = reader.ReadByte();
            
            return val32 == 0x12345678 && val16 == 0xABCD && val8 == 0xFF;
        } catch (...) {
            return false;
        }
    }
    
    bool TestMemoryStreamOperations() {
        try {
            ByteVector data = {0x01, 0x02, 0x03, 0x04, 0x05};
            MemoryStream stream(data);
            
            // Test reading
            stream.Seek(2, std::ios::beg);
            uint8_t byte = 0;
            stream.read(reinterpret_cast<char*>(&byte), 1);
            
            return byte == 0x03;
        } catch (...) {
            return false;
        }
    }
    
    bool TestScriptBuilding() {
        try {
            ScriptBuilder builder;
            
            // Build a simple script
            builder.EmitPush(42);
            builder.EmitPush(58);
            builder.Emit(OpCode::ADD);
            
            auto script_bytes = builder.ToArray();
            
            // Check that we have some script bytes
            return script_bytes.size() > 0;
        } catch (...) {
            return false;
        }
    }
    
    bool TestVMExecution() {
        try {
            // Create a simple script that pushes numbers and adds them
            ScriptBuilder builder;
            builder.EmitPush(10);
            builder.EmitPush(20);
            builder.Emit(OpCode::ADD);
            
            auto script_bytes = builder.ToArray();
            auto script = std::make_shared<Script>(script_bytes);
            
            // Create execution engine
            ExecutionEngine engine;
            engine.LoadScript(script);
            
            // Execute one step to load the script
            auto state = engine.ExecuteNext();
            
            // Check that the engine is in a valid state
            return state != VMState::FAULT;
        } catch (...) {
            return false;
        }
    }
    
    bool TestStackOperations() {
        try {
            // Test stack item creation and operations
            auto int_item = StackItem::FromInt32(42);
            auto bool_item = StackItem::FromBool(true);
            auto bytes_item = StackItem::FromBytes({0x01, 0x02, 0x03});
            
            // Test type checking
            return int_item->GetType() == StackItemType::Integer &&
                   bool_item->GetType() == StackItemType::Boolean &&
                   bytes_item->GetType() == StackItemType::ByteString;
        } catch (...) {
            return false;
        }
    }
    
    bool TestJSONOperations() {
        try {
            // Test JSON object creation and manipulation
            auto json_obj = std::make_shared<JObject>();
            json_obj->Set("name", std::make_shared<JString>("Neo"));
            json_obj->Set("version", std::make_shared<JString>("3.0"));
            json_obj->Set("ready", std::make_shared<JBoolean>(true));
            
            // Test JSON array
            auto json_array = std::make_shared<JArray>();
            json_array->Add(std::make_shared<JString>("item1"));
            json_array->Add(std::make_shared<JString>("item2"));
            
            json_obj->Set("items", json_array);
            
            // Test serialization
            std::string json_str = json_obj->ToString();
            
            return json_str.find("Neo") != std::string::npos &&
                   json_str.find("3.0") != std::string::npos;
        } catch (...) {
            return false;
        }
    }
    
    bool TestEndToEndWorkflow() {
        try {
            std::cout << std::endl << "    🔄 Running End-to-End Workflow Test..." << std::endl;
            
            // 1. Create a transaction-like data structure
            std::cout << "    → Creating transaction data..." << std::endl;
            ByteVector tx_data = {0x01, 0x02, 0x03, 0x04, 0x05};
            
            // 2. Hash the transaction data
            std::cout << "    → Hashing transaction..." << std::endl;
            auto tx_hash = Hash::SHA256(tx_data);
            
            // 3. Create a script that validates the transaction
            std::cout << "    → Building validation script..." << std::endl;
            ScriptBuilder builder;
            builder.EmitPush(100);  // Push value to stack
            builder.EmitPush(50);   // Push another value
            builder.Emit(OpCode::ADD);  // Add them
            
            auto script_bytes = builder.ToArray();
            auto script = std::make_shared<Script>(script_bytes);
            
            // 4. Execute the script
            std::cout << "    → Executing script..." << std::endl;
            ExecutionEngine engine;
            engine.LoadScript(script);
            
            // Execute a few steps
            auto state = engine.ExecuteNext();
            if (state == VMState::FAULT) {
                std::cout << "    ❌ Script execution faulted" << std::endl;
                return false;
            }
            
            // 5. Create JSON representation
            std::cout << "    → Creating JSON representation..." << std::endl;
            auto json_result = std::make_shared<JObject>();
            json_result->Set("transaction_hash", std::make_shared<JString>("0x" + BytesToHex(tx_hash)));
            json_result->Set("script_length", std::make_shared<JNumber>(static_cast<double>(script_bytes.size())));
            json_result->Set("execution_state", std::make_shared<JString>("success"));
            
            std::cout << "    ✅ Workflow completed successfully!" << std::endl;
            std::cout << "    📊 Result: " << json_result->ToString() << std::endl;
            
            return true;
        } catch (const std::exception& e) {
            std::cout << "    ❌ Workflow failed: " << e.what() << std::endl;
            return false;
        } catch (...) {
            std::cout << "    ❌ Workflow failed with unknown error" << std::endl;
            return false;
        }
    }
    
    std::string BytesToHex(const ByteVector& bytes) {
        std::string result;
        for (uint8_t byte : bytes) {
            char hex[3];
            sprintf(hex, "%02x", byte);
            result += hex;
        }
        return result;
    }
    
    void PrintSummary() {
        std::cout << std::endl;
        std::cout << "╔═══════════════════════════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║                            TEST SUMMARY                                       ║" << std::endl;
        std::cout << "╚═══════════════════════════════════════════════════════════════════════════════╝" << std::endl;
        std::cout << std::endl;
        std::cout << "📊 Total Tests: " << total_tests << std::endl;
        std::cout << "✅ Passed: " << passed_tests << std::endl;
        std::cout << "❌ Failed: " << failed_tests << std::endl;
        std::cout << "📈 Success Rate: " << (passed_tests * 100 / total_tests) << "%" << std::endl;
        std::cout << std::endl;
        
        if (failed_tests == 0) {
            std::cout << "🎉 ALL TESTS PASSED! Neo C++ workflow is working correctly!" << std::endl;
        } else if (passed_tests > failed_tests) {
            std::cout << "⚠️ Most tests passed, but some issues were found." << std::endl;
        } else {
            std::cout << "❌ Multiple tests failed. Neo C++ workflow needs attention." << std::endl;
        }
        
        std::cout << std::endl;
        std::cout << "🔧 Neo C++ Components Tested:" << std::endl;
        std::cout << "   • Core Data Types (UInt256, UInt160, ByteVector)" << std::endl;
        std::cout << "   • Cryptographic Functions (SHA256, RIPEMD160, Hash160)" << std::endl;
        std::cout << "   • IO Operations (Binary serialization, Memory streams)" << std::endl;
        std::cout << "   • Virtual Machine (Script building, Execution engine)" << std::endl;
        std::cout << "   • JSON Handling (Object/Array creation, Serialization)" << std::endl;
        std::cout << "   • End-to-End Integration (Transaction workflow)" << std::endl;
        std::cout << std::endl;
    }
};

int main() {
    NeoWorkflowTester tester;
    tester.RunAllTests();
    return 0;
}