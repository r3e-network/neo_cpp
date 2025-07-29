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

class UT_OpCodes_Arithmetic : public testing::Test
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

TEST_F(UT_OpCodes_Arithmetic, GE)
{
    // Test for GE opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arithmetic/GE.json");
}

TEST_F(UT_OpCodes_Arithmetic, LT)
{
    // Test for LT opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arithmetic/LT.json");
}

TEST_F(UT_OpCodes_Arithmetic, MODMUL)
{
    // Test for MODMUL opcode
    RunJsonTest(
        "/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arithmetic/MODMUL.json");
}

TEST_F(UT_OpCodes_Arithmetic, NUMNOTEQUAL)
{
    // Test for NUMNOTEQUAL opcode
    RunJsonTest(
        "/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arithmetic/NUMNOTEQUAL.json");
}

TEST_F(UT_OpCodes_Arithmetic, NOT)
{
    // Test for NOT opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arithmetic/NOT.json");
}

TEST_F(UT_OpCodes_Arithmetic, MODPOW)
{
    // Test for MODPOW opcode
    RunJsonTest(
        "/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arithmetic/MODPOW.json");
}

TEST_F(UT_OpCodes_Arithmetic, LE)
{
    // Test for LE opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arithmetic/LE.json");
}

TEST_F(UT_OpCodes_Arithmetic, SHL)
{
    // Test for SHL opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arithmetic/SHL.json");
}

TEST_F(UT_OpCodes_Arithmetic, GT)
{
    // Test for GT opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arithmetic/GT.json");
}

TEST_F(UT_OpCodes_Arithmetic, POW)
{
    // Test for POW opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arithmetic/POW.json");
}

TEST_F(UT_OpCodes_Arithmetic, NUMEQUAL)
{
    // Test for NUMEQUAL opcode
    RunJsonTest(
        "/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arithmetic/NUMEQUAL.json");
}

TEST_F(UT_OpCodes_Arithmetic, SIGN)
{
    // Test for SIGN opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arithmetic/SIGN.json");
}

TEST_F(UT_OpCodes_Arithmetic, SQRT)
{
    // Test for SQRT opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arithmetic/SQRT.json");
}

TEST_F(UT_OpCodes_Arithmetic, SHR)
{
    // Test for SHR opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Arithmetic/SHR.json");
}
