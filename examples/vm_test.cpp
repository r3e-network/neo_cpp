#include <iostream>
#include <gtest/gtest.h>
#include <neo/vm/opcode.h>

using namespace neo::vm;

// Simple test to verify that the VM module works
TEST(VMTest, OpCodeEnumeration) {
    // Test that the OpCode enumeration is correctly defined
    EXPECT_EQ(static_cast<uint8_t>(OpCode::PUSH0), 0x10);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::PUSH1), 0x11);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::ADD), 0x9E);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::RET), 0x40);
}

int main(int argc, char** argv) {
    std::cout << "Running VM test..." << std::endl;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
