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

class UT_OpCodes_Slot : public testing::Test
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

TEST_F(UT_OpCodes_Slot, STSFLD6)
{
    // Test for STSFLD6 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/STSFLD6.json");
}

TEST_F(UT_OpCodes_Slot, INITSLOT)
{
    // Test for INITSLOT opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/INITSLOT.json");
}

TEST_F(UT_OpCodes_Slot, LDLOC4)
{
    // Test for LDLOC4 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/LDLOC4.json");
}

TEST_F(UT_OpCodes_Slot, LDARG)
{
    // Test for LDARG opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/LDARG.json");
}

TEST_F(UT_OpCodes_Slot, STARG0)
{
    // Test for STARG0 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/STARG0.json");
}

TEST_F(UT_OpCodes_Slot, LDSFLD0)
{
    // Test for LDSFLD0 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/LDSFLD0.json");
}

TEST_F(UT_OpCodes_Slot, LDARG4)
{
    // Test for LDARG4 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/LDARG4.json");
}

TEST_F(UT_OpCodes_Slot, LDARG5)
{
    // Test for LDARG5 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/LDARG5.json");
}

TEST_F(UT_OpCodes_Slot, LDSFLD1)
{
    // Test for LDSFLD1 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/LDSFLD1.json");
}

TEST_F(UT_OpCodes_Slot, LDLOC)
{
    // Test for LDLOC opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/LDLOC.json");
}

TEST_F(UT_OpCodes_Slot, STARG1)
{
    // Test for STARG1 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/STARG1.json");
}

TEST_F(UT_OpCodes_Slot, LDLOC5)
{
    // Test for LDLOC5 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/LDLOC5.json");
}

TEST_F(UT_OpCodes_Slot, LDSFLD6)
{
    // Test for LDSFLD6 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/LDSFLD6.json");
}

TEST_F(UT_OpCodes_Slot, LDARG2)
{
    // Test for LDARG2 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/LDARG2.json");
}

TEST_F(UT_OpCodes_Slot, STSFLD0)
{
    // Test for STSFLD0 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/STSFLD0.json");
}

TEST_F(UT_OpCodes_Slot, LDLOC2)
{
    // Test for LDLOC2 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/LDLOC2.json");
}

TEST_F(UT_OpCodes_Slot, STARG6)
{
    // Test for STARG6 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/STARG6.json");
}

TEST_F(UT_OpCodes_Slot, LDLOC3)
{
    // Test for LDLOC3 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/LDLOC3.json");
}

TEST_F(UT_OpCodes_Slot, STSFLD1)
{
    // Test for STSFLD1 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/STSFLD1.json");
}

TEST_F(UT_OpCodes_Slot, INITSSLOT)
{
    // Test for INITSSLOT opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/INITSSLOT.json");
}

TEST_F(UT_OpCodes_Slot, LDARG3)
{
    // Test for LDARG3 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/LDARG3.json");
}

TEST_F(UT_OpCodes_Slot, LDSFLD)
{
    // Test for LDSFLD opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/LDSFLD.json");
}

TEST_F(UT_OpCodes_Slot, LDARG0)
{
    // Test for LDARG0 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/LDARG0.json");
}

TEST_F(UT_OpCodes_Slot, LDSFLD4)
{
    // Test for LDSFLD4 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/LDSFLD4.json");
}

TEST_F(UT_OpCodes_Slot, STARG4)
{
    // Test for STARG4 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/STARG4.json");
}

TEST_F(UT_OpCodes_Slot, LDLOC0)
{
    // Test for LDLOC0 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/LDLOC0.json");
}

TEST_F(UT_OpCodes_Slot, STSFLD2)
{
    // Test for STSFLD2 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/STSFLD2.json");
}

TEST_F(UT_OpCodes_Slot, STSFLD3)
{
    // Test for STSFLD3 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/STSFLD3.json");
}

TEST_F(UT_OpCodes_Slot, LDLOC1)
{
    // Test for LDLOC1 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/LDLOC1.json");
}

TEST_F(UT_OpCodes_Slot, STARG5)
{
    // Test for STARG5 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/STARG5.json");
}

TEST_F(UT_OpCodes_Slot, LDSFLD5)
{
    // Test for LDSFLD5 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/LDSFLD5.json");
}

TEST_F(UT_OpCodes_Slot, LDARG1)
{
    // Test for LDARG1 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/LDARG1.json");
}

TEST_F(UT_OpCodes_Slot, STARG2)
{
    // Test for STARG2 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/STARG2.json");
}

TEST_F(UT_OpCodes_Slot, LDLOC6)
{
    // Test for LDLOC6 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/LDLOC6.json");
}

TEST_F(UT_OpCodes_Slot, STARG)
{
    // Test for STARG opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/STARG.json");
}

TEST_F(UT_OpCodes_Slot, STSFLD4)
{
    // Test for STSFLD4 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/STSFLD4.json");
}

TEST_F(UT_OpCodes_Slot, LDARG6)
{
    // Test for LDARG6 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/LDARG6.json");
}

TEST_F(UT_OpCodes_Slot, LDSFLD2)
{
    // Test for LDSFLD2 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/LDSFLD2.json");
}

TEST_F(UT_OpCodes_Slot, LDSFLD3)
{
    // Test for LDSFLD3 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/LDSFLD3.json");
}

TEST_F(UT_OpCodes_Slot, STLOC)
{
    // Test for STLOC opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/STLOC.json");
}

TEST_F(UT_OpCodes_Slot, STSFLD)
{
    // Test for STSFLD opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/STSFLD.json");
}

TEST_F(UT_OpCodes_Slot, STSFLD5)
{
    // Test for STSFLD5 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/STSFLD5.json");
}

TEST_F(UT_OpCodes_Slot, STARG3)
{
    // Test for STARG3 opcode
    RunJsonTest("/Users/jinghuiliao/git/r3e/neo_cpp/neo_csharp/tests/Neo.VM.Tests/Tests/OpCodes/Slot/STARG3.json");
}
