#include <fstream>
#include <gtest/gtest.h>
#include <neo/io/byte_span.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/opcode.h>
#include <neo/vm/script.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/stack_item.h>
#include <nlohmann/json.hpp>

using namespace neo::vm;
using namespace neo::io;
using json = nlohmann::json;

class UT_OpCodes_Control : public testing::Test
{
  protected:
    void SetUp() override {}
    void TearDown() override {}

    void RunJsonTest(const std::string& jsonPath)
    {
        std::ifstream file(jsonPath);
        if (!file.is_open())
        {
            GTEST_SKIP() << "Test file not found: " << jsonPath;
            return;
        }

        json testData;
        file >> testData;

        // Parse and execute test based on JSON structure
        // This follows the same pattern as VM JSON tests
        for (const auto& test : testData["tests"])
        {
            ExecutionEngine engine;

            // Load script
            auto scriptHex = test["script"].get<std::string>();
            auto scriptBytes = ParseHex(scriptHex);
            Script script(scriptBytes);
            engine.LoadScript(script);

            // Execute
            auto result = engine.Execute();

            // Verify result
            auto expectedState = test["state"].get<std::string>();
            if (expectedState == "HALT")
            {
                EXPECT_EQ(result, VMState::Halt);
            }
            else if (expectedState == "FAULT")
            {
                EXPECT_EQ(result, VMState::Fault);
            }

            // Verify stack if specified
            if (test.contains("result_stack"))
            {
                // TODO: Implement stack verification
            }
        }
    }

    neo::vm::internal::ByteVector ParseHex(const std::string& hex)
    {
        neo::vm::internal::ByteVector result;
        for (size_t i = 0; i < hex.length(); i += 2)
        {
            std::string byte = hex.substr(i, 2);
            result.Push(static_cast<uint8_t>(std::stoul(byte, nullptr, 16)));
        }
        return result;
    }
};

TEST_F(UT_OpCodes_Control, TRY_CATCH_FINALLY6)
{
    // Test for TRY_CATCH_FINALLY6 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/"
                "TRY_CATCH_FINALLY6.json");
}

TEST_F(UT_OpCodes_Control, JMPLE_L)
{
    // Test for JMPLE_L opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/JMPLE_L.json");
}

TEST_F(UT_OpCodes_Control, JMPEQ_L)
{
    // Test for JMPEQ_L opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/JMPEQ_L.json");
}

TEST_F(UT_OpCodes_Control, JMPLE)
{
    // Test for JMPLE opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/JMPLE.json");
}

TEST_F(UT_OpCodes_Control, JMPNE)
{
    // Test for JMPNE opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/JMPNE.json");
}

TEST_F(UT_OpCodes_Control, TRY_CATCH_FINALLY7)
{
    // Test for TRY_CATCH_FINALLY7 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/"
                "TRY_CATCH_FINALLY7.json");
}

TEST_F(UT_OpCodes_Control, ASSERTMSG)
{
    // Test for ASSERTMSG opcode
    RunJsonTest(
        "/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/ASSERTMSG.json");
}

TEST_F(UT_OpCodes_Control, JMPGT)
{
    // Test for JMPGT opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/JMPGT.json");
}

TEST_F(UT_OpCodes_Control, JMP_L)
{
    // Test for JMP_L opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/JMP_L.json");
}

TEST_F(UT_OpCodes_Control, JMPIF_L)
{
    // Test for JMPIF_L opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/JMPIF_L.json");
}

TEST_F(UT_OpCodes_Control, TRY_FINALLY)
{
    // Test for TRY_FINALLY opcode
    RunJsonTest(
        "/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/TRY_FINALLY.json");
}

TEST_F(UT_OpCodes_Control, JMPNE_L)
{
    // Test for JMPNE_L opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/JMPNE_L.json");
}

TEST_F(UT_OpCodes_Control, JMPIFNOT)
{
    // Test for JMPIFNOT opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/JMPIFNOT.json");
}

TEST_F(UT_OpCodes_Control, ABORTMSG)
{
    // Test for ABORTMSG opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/ABORTMSG.json");
}

TEST_F(UT_OpCodes_Control, CALL)
{
    // Test for CALL opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/CALL.json");
}

TEST_F(UT_OpCodes_Control, CALL_L)
{
    // Test for CALL_L opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/CALL_L.json");
}

TEST_F(UT_OpCodes_Control, JMPGE_L)
{
    // Test for JMPGE_L opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/JMPGE_L.json");
}

TEST_F(UT_OpCodes_Control, RET)
{
    // Test for RET opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/RET.json");
}

TEST_F(UT_OpCodes_Control, JMPGT_L)
{
    // Test for JMPGT_L opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/JMPGT_L.json");
}

TEST_F(UT_OpCodes_Control, JMPEQ)
{
    // Test for JMPEQ opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/JMPEQ.json");
}

TEST_F(UT_OpCodes_Control, SYSCALL)
{
    // Test for SYSCALL opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/SYSCALL.json");
}

TEST_F(UT_OpCodes_Control, CALLA)
{
    // Test for CALLA opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/CALLA.json");
}

TEST_F(UT_OpCodes_Control, ASSERT)
{
    // Test for ASSERT opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/ASSERT.json");
}

TEST_F(UT_OpCodes_Control, TRY_CATCH_FINALLY2)
{
    // Test for TRY_CATCH_FINALLY2 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/"
                "TRY_CATCH_FINALLY2.json");
}

TEST_F(UT_OpCodes_Control, TRY_CATCH_FINALLY3)
{
    // Test for TRY_CATCH_FINALLY3 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/"
                "TRY_CATCH_FINALLY3.json");
}

TEST_F(UT_OpCodes_Control, NOP)
{
    // Test for NOP opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/NOP.json");
}

TEST_F(UT_OpCodes_Control, JMPGE)
{
    // Test for JMPGE opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/JMPGE.json");
}

TEST_F(UT_OpCodes_Control, TRY_CATCH_FINALLY10)
{
    // Test for TRY_CATCH_FINALLY10 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/"
                "TRY_CATCH_FINALLY10.json");
}

TEST_F(UT_OpCodes_Control, JMPLT)
{
    // Test for JMPLT opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/JMPLT.json");
}

TEST_F(UT_OpCodes_Control, JMPIF)
{
    // Test for JMPIF opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/JMPIF.json");
}

TEST_F(UT_OpCodes_Control, ABORT)
{
    // Test for ABORT opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/ABORT.json");
}

TEST_F(UT_OpCodes_Control, JMPIFNOT_L)
{
    // Test for JMPIFNOT_L opcode
    RunJsonTest(
        "/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/JMPIFNOT_L.json");
}

TEST_F(UT_OpCodes_Control, TRY_CATCH)
{
    // Test for TRY_CATCH opcode
    RunJsonTest(
        "/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/TRY_CATCH.json");
}

TEST_F(UT_OpCodes_Control, TRY_CATCH_FINALLY4)
{
    // Test for TRY_CATCH_FINALLY4 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/"
                "TRY_CATCH_FINALLY4.json");
}

TEST_F(UT_OpCodes_Control, JMP)
{
    // Test for JMP opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/JMP.json");
}

TEST_F(UT_OpCodes_Control, TRY_CATCH_FINALLY8)
{
    // Test for TRY_CATCH_FINALLY8 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/"
                "TRY_CATCH_FINALLY8.json");
}

TEST_F(UT_OpCodes_Control, TRY_CATCH_FINALLY9)
{
    // Test for TRY_CATCH_FINALLY9 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/"
                "TRY_CATCH_FINALLY9.json");
}

TEST_F(UT_OpCodes_Control, JMPLT_L)
{
    // Test for JMPLT_L opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/JMPLT_L.json");
}

TEST_F(UT_OpCodes_Control, TRY_CATCH_FINALLY)
{
    // Test for TRY_CATCH_FINALLY opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/"
                "TRY_CATCH_FINALLY.json");
}

TEST_F(UT_OpCodes_Control, TRY_CATCH_FINALLY5)
{
    // Test for TRY_CATCH_FINALLY5 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/"
                "TRY_CATCH_FINALLY5.json");
}

TEST_F(UT_OpCodes_Control, THROW)
{
    // Test for THROW opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Control/THROW.json");
}
