#include <iostream>
#include <neo/smartcontract/application_engine.h>
#include <neo/vm/script.h>
#include <neo/vm/stack_item.h>

using namespace neo::vm;
using namespace neo::smartcontract;

int main()
{
    // Create a simple script that calls a system runtime function
    std::vector<uint8_t> script_bytes = {
        static_cast<uint8_t>(OpCode::SYSCALL),  // System call
        0x2d, 0x9b, 0x72, 0x27                  // System.Runtime.GetTime hash
    };

    // Create a script from the bytes
    auto script = std::make_shared<Script>(script_bytes);

    // Create an application engine
    ApplicationEngine engine(TriggerType::Application, nullptr, nullptr, 0);

    // Load the script into the engine
    engine.LoadScript(script);

    // Execute the script
    auto result = engine.Execute();

    // Check the result
    if (result == VMState::HALT)
    {
        // Get the result from the evaluation stack
        auto stack_item = engine.GetEvaluationStack().Pop();

        std::cout << "Execution succeeded" << std::endl;
    }
    else
    {
        std::cout << "Execution failed with state: " << static_cast<int>(result) << std::endl;
    }

    return 0;
}
