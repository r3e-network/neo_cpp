#include <iostream>
#include <gtest/gtest.h>
#include <neo/vm/opcode.h>
#include <neo/vm/script.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/stack_item.h>

using namespace neo::vm;

// Test Script
TEST(VMComprehensiveTest, Script) {
    // Create a script
    Script script;
    script.EmitPush(123);
    script.EmitPush(456);
    script.Emit(OpCode::ADD);
    script.Emit(OpCode::RET);

    // Check the script
    EXPECT_EQ(script.GetScript().Size(), 7);
    EXPECT_EQ(script.GetScript()[0], static_cast<uint8_t>(OpCode::PUSHINT8));
    EXPECT_EQ(script.GetScript()[1], 123);
    EXPECT_EQ(script.GetScript()[2], static_cast<uint8_t>(OpCode::PUSHINT16));
    EXPECT_EQ(script.GetScript()[3], 200);
    EXPECT_EQ(script.GetScript()[4], 1);
    EXPECT_EQ(script.GetScript()[5], static_cast<uint8_t>(OpCode::ADD));
    EXPECT_EQ(script.GetScript()[6], static_cast<uint8_t>(OpCode::RET));
}

// Test ExecutionEngine
TEST(VMComprehensiveTest, ExecutionEngine) {
    // Create a script that adds two numbers
    Script script;
    script.EmitPush(123);
    script.EmitPush(456);
    script.Emit(OpCode::ADD);
    script.Emit(OpCode::RET);

    // Create an execution engine
    ExecutionEngine engine;
    
    // Load the script
    engine.LoadScript(script.GetScript());
    
    // Execute the script
    EXPECT_TRUE(engine.Execute());
    
    // Check the result
    EXPECT_EQ(engine.GetResultStack().size(), 1);
    EXPECT_EQ(engine.GetResultStack()[0]->GetInteger(), 579);
}

// Test Stack Items
TEST(VMComprehensiveTest, StackItems) {
    // Test Boolean
    auto boolItem = BooleanItem(true);
    EXPECT_EQ(boolItem.GetType(), StackItemType::Boolean);
    EXPECT_TRUE(boolItem.GetBoolean());
    EXPECT_EQ(boolItem.GetInteger(), 1);
    
    // Test Integer
    auto intItem = IntegerItem(123);
    EXPECT_EQ(intItem.GetType(), StackItemType::Integer);
    EXPECT_TRUE(intItem.GetBoolean());
    EXPECT_EQ(intItem.GetInteger(), 123);
    
    // Test ByteString
    neo::io::ByteVector bytes = { 0x01, 0x02, 0x03 };
    auto byteStringItem = ByteStringItem(bytes);
    EXPECT_EQ(byteStringItem.GetType(), StackItemType::ByteString);
    EXPECT_TRUE(byteStringItem.GetBoolean());
    EXPECT_EQ(byteStringItem.GetByteArray().ToHexString(), "010203");
    
    // Test Buffer
    auto bufferItem = BufferItem(bytes);
    EXPECT_EQ(bufferItem.GetType(), StackItemType::Buffer);
    EXPECT_TRUE(bufferItem.GetBoolean());
    EXPECT_EQ(bufferItem.GetByteArray().ToHexString(), "010203");
    
    // Test Array
    std::vector<std::shared_ptr<StackItem>> items;
    items.push_back(std::make_shared<BooleanItem>(true));
    items.push_back(std::make_shared<IntegerItem>(123));
    auto arrayItem = ArrayItem(items);
    EXPECT_EQ(arrayItem.GetType(), StackItemType::Array);
    EXPECT_TRUE(arrayItem.GetBoolean());
    EXPECT_EQ(arrayItem.GetArray().size(), 2);
    EXPECT_TRUE(arrayItem.GetArray()[0]->GetBoolean());
    EXPECT_EQ(arrayItem.GetArray()[1]->GetInteger(), 123);
    
    // Test Struct
    auto structItem = StructItem(items);
    EXPECT_EQ(structItem.GetType(), StackItemType::Struct);
    EXPECT_TRUE(structItem.GetBoolean());
    EXPECT_EQ(structItem.GetArray().size(), 2);
    EXPECT_TRUE(structItem.GetArray()[0]->GetBoolean());
    EXPECT_EQ(structItem.GetArray()[1]->GetInteger(), 123);
    
    // Test Map
    std::map<std::shared_ptr<StackItem>, std::shared_ptr<StackItem>> map;
    map[std::make_shared<IntegerItem>(1)] = std::make_shared<BooleanItem>(true);
    map[std::make_shared<IntegerItem>(2)] = std::make_shared<IntegerItem>(123);
    auto mapItem = MapItem(map);
    EXPECT_EQ(mapItem.GetType(), StackItemType::Map);
    EXPECT_TRUE(mapItem.GetBoolean());
    EXPECT_EQ(mapItem.GetMap().size(), 2);
    EXPECT_TRUE(mapItem.Get(std::make_shared<IntegerItem>(1))->GetBoolean());
    EXPECT_EQ(mapItem.Get(std::make_shared<IntegerItem>(2))->GetInteger(), 123);
}

// Test Complex Script
TEST(VMComprehensiveTest, ComplexScript) {
    // Create a script that calculates factorial of 5
    Script script;
    
    // Initialize n = 5
    script.EmitPush(5);
    
    // Initialize result = 1
    script.EmitPush(1);
    
    // Loop start
    script.Emit(OpCode::SWAP);     // Swap n and result
    script.Emit(OpCode::DUP);      // Duplicate n
    script.EmitPush(1);            // Push 1
    script.Emit(OpCode::LE);       // Check if n <= 1
    script.EmitJump(OpCode::JMPIF, 9); // Jump to end if n <= 1
    
    // Loop body
    script.Emit(OpCode::SWAP);     // Swap n and result
    script.Emit(OpCode::DUP);      // Duplicate result
    script.Emit(OpCode::ROT);      // Rotate n to top
    script.Emit(OpCode::MUL);      // result = result * n
    script.Emit(OpCode::SWAP);     // Swap n and result
    script.EmitPush(1);            // Push 1
    script.Emit(OpCode::SUB);      // n = n - 1
    script.EmitJump(OpCode::JMP, -14); // Jump back to loop start
    
    // End
    script.Emit(OpCode::DROP);     // Drop n
    script.Emit(OpCode::RET);      // Return result
    
    // Create an execution engine
    ExecutionEngine engine;
    
    // Load the script
    engine.LoadScript(script.GetScript());
    
    // Execute the script
    EXPECT_TRUE(engine.Execute());
    
    // Check the result (factorial of 5 = 120)
    EXPECT_EQ(engine.GetResultStack().size(), 1);
    EXPECT_EQ(engine.GetResultStack()[0]->GetInteger(), 120);
}

int main(int argc, char** argv) {
    std::cout << "Running VM comprehensive test..." << std::endl;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
