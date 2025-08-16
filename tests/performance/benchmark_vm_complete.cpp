/**
 * @file benchmark_vm_complete.cpp
 * @brief Comprehensive VM execution performance benchmarks
 */

#include <benchmark/benchmark.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/opcode.h>
#include <neo/vm/script.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/vm_state.h>
#include <neo/io/byte_vector.h>
#include <random>
#include <vector>

using namespace neo::vm;
using namespace neo::io;

// ============================================================================
// Helper Functions
// ============================================================================

static Script CreateScript(const ByteVector& scriptData) {
    return Script(scriptData);
}

static ByteVector GenerateRandomBytes(size_t size) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    std::vector<uint8_t> data(size);
    for (size_t i = 0; i < size; ++i) {
        data[i] = static_cast<uint8_t>(dis(gen));
    }
    return ByteVector(data);
}

// ============================================================================
// Basic Opcode Execution Benchmarks
// ============================================================================

static void BM_VM_PushPop(benchmark::State& state) {
    for (auto _ : state) {
        ScriptBuilder sb;
        for (int i = 0; i < 100; ++i) {
            sb.EmitPush(static_cast<int64_t>(i));
        }
        for (int i = 0; i < 100; ++i) {
            sb.Emit(OpCode::DROP);
        }
        
        ExecutionEngine engine;
        auto script = CreateScript(sb.ToArray());
        engine.LoadScript(script);
        auto result = engine.Execute();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * 200); // 100 push + 100 pop
}
BENCHMARK(BM_VM_PushPop);

static void BM_VM_Arithmetic_Add(benchmark::State& state) {
    for (auto _ : state) {
        ScriptBuilder sb;
        sb.EmitPush(static_cast<int64_t>(100));
        sb.EmitPush(static_cast<int64_t>(200));
        sb.Emit(OpCode::ADD);
        
        ExecutionEngine engine;
        auto script = CreateScript(sb.ToArray());
        engine.LoadScript(script);
        auto result = engine.Execute();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_VM_Arithmetic_Add);

static void BM_VM_Arithmetic_Complex(benchmark::State& state) {
    for (auto _ : state) {
        ScriptBuilder sb;
        // (((10 + 20) * 3) - 15) / 5 = 15
        sb.EmitPush(static_cast<int64_t>(10));
        sb.EmitPush(static_cast<int64_t>(20));
        sb.Emit(OpCode::ADD);
        sb.EmitPush(static_cast<int64_t>(3));
        sb.Emit(OpCode::MUL);
        sb.EmitPush(static_cast<int64_t>(15));
        sb.Emit(OpCode::SUB);
        sb.EmitPush(static_cast<int64_t>(5));
        sb.Emit(OpCode::DIV);
        
        ExecutionEngine engine;
        auto script = CreateScript(sb.ToArray());
        engine.LoadScript(script);
        auto result = engine.Execute();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_VM_Arithmetic_Complex);

// ============================================================================
// Control Flow Benchmarks
// ============================================================================

static void BM_VM_ConditionalJump(benchmark::State& state) {
    for (auto _ : state) {
        ScriptBuilder sb;
        sb.EmitPush(true);
        sb.EmitJump(OpCode::JMPIF, 5);
        sb.EmitPush(static_cast<int64_t>(100)); // Skipped
        sb.EmitPush(static_cast<int64_t>(200)); // Jump target
        
        ExecutionEngine engine;
        auto script = CreateScript(sb.ToArray());
        engine.LoadScript(script);
        auto result = engine.Execute();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_VM_ConditionalJump);

static void BM_VM_Loop_Small(benchmark::State& state) {
    for (auto _ : state) {
        ScriptBuilder sb;
        // Simple loop: sum 1 to 10
        sb.EmitPush(static_cast<int64_t>(0)); // sum
        sb.EmitPush(static_cast<int64_t>(1)); // counter
        
        int loop_start = sb.GetPosition();
        sb.Emit(OpCode::DUP); // Duplicate counter
        sb.EmitPush(static_cast<int64_t>(10));
        sb.Emit(OpCode::GT); // counter > 10?
        sb.EmitJump(OpCode::JMPIF, 10); // Exit if true
        
        sb.Emit(OpCode::DUP); // Duplicate counter
        sb.Emit(OpCode::ROT); // Rotate stack
        sb.Emit(OpCode::ADD); // Add to sum
        sb.Emit(OpCode::INC); // Increment counter
        sb.EmitJump(OpCode::JMP, loop_start - sb.GetPosition());
        
        sb.Emit(OpCode::DROP); // Drop counter
        
        ExecutionEngine engine;
        auto script = CreateScript(sb.ToArray());
        engine.LoadScript(script);
        auto result = engine.Execute();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * 10); // 10 loop iterations
}
BENCHMARK(BM_VM_Loop_Small);

static void BM_VM_Loop_Large(benchmark::State& state) {
    for (auto _ : state) {
        ScriptBuilder sb;
        // Loop 100 times
        sb.EmitPush(static_cast<int64_t>(0)); // sum
        sb.EmitPush(static_cast<int64_t>(1)); // counter
        
        int loop_start = sb.GetPosition();
        sb.Emit(OpCode::DUP); // Duplicate counter
        sb.EmitPush(static_cast<int64_t>(100));
        sb.Emit(OpCode::GT); // counter > 100?
        sb.EmitJump(OpCode::JMPIF, 10); // Exit if true
        
        sb.Emit(OpCode::DUP); // Duplicate counter
        sb.Emit(OpCode::ROT); // Rotate stack
        sb.Emit(OpCode::ADD); // Add to sum
        sb.Emit(OpCode::INC); // Increment counter
        sb.EmitJump(OpCode::JMP, loop_start - sb.GetPosition());
        
        sb.Emit(OpCode::DROP); // Drop counter
        
        ExecutionEngine engine;
        auto script = CreateScript(sb.ToArray());
        engine.LoadScript(script);
        auto result = engine.Execute();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * 100); // 100 loop iterations
}
BENCHMARK(BM_VM_Loop_Large);

// ============================================================================
// Stack Manipulation Benchmarks
// ============================================================================

static void BM_VM_Stack_DupDrop(benchmark::State& state) {
    for (auto _ : state) {
        ScriptBuilder sb;
        sb.EmitPush(static_cast<int64_t>(42));
        for (int i = 0; i < 50; ++i) {
            sb.Emit(OpCode::DUP);
            sb.Emit(OpCode::DROP);
        }
        
        ExecutionEngine engine;
        auto script = CreateScript(sb.ToArray());
        engine.LoadScript(script);
        auto result = engine.Execute();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * 100); // 50 dup + 50 drop
}
BENCHMARK(BM_VM_Stack_DupDrop);

static void BM_VM_Stack_Swap(benchmark::State& state) {
    for (auto _ : state) {
        ScriptBuilder sb;
        sb.EmitPush(static_cast<int64_t>(100));
        sb.EmitPush(static_cast<int64_t>(200));
        for (int i = 0; i < 50; ++i) {
            sb.Emit(OpCode::SWAP);
        }
        
        ExecutionEngine engine;
        auto script = CreateScript(sb.ToArray());
        engine.LoadScript(script);
        auto result = engine.Execute();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * 50);
}
BENCHMARK(BM_VM_Stack_Swap);

static void BM_VM_Stack_Rotate(benchmark::State& state) {
    for (auto _ : state) {
        ScriptBuilder sb;
        sb.EmitPush(static_cast<int64_t>(100));
        sb.EmitPush(static_cast<int64_t>(200));
        sb.EmitPush(static_cast<int64_t>(300));
        for (int i = 0; i < 30; ++i) {
            sb.Emit(OpCode::ROT);
        }
        
        ExecutionEngine engine;
        auto script = CreateScript(sb.ToArray());
        engine.LoadScript(script);
        auto result = engine.Execute();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * 30);
}
BENCHMARK(BM_VM_Stack_Rotate);

// ============================================================================
// Array Operations Benchmarks
// ============================================================================

static void BM_VM_Array_Create(benchmark::State& state) {
    for (auto _ : state) {
        ScriptBuilder sb;
        sb.EmitPush(static_cast<int64_t>(10));
        sb.Emit(OpCode::NEWARRAY);
        
        ExecutionEngine engine;
        auto script = CreateScript(sb.ToArray());
        engine.LoadScript(script);
        auto result = engine.Execute();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_VM_Array_Create);

static void BM_VM_Array_Access(benchmark::State& state) {
    for (auto _ : state) {
        ScriptBuilder sb;
        // Create array with 10 elements
        sb.EmitPush(static_cast<int64_t>(10));
        sb.Emit(OpCode::NEWARRAY);
        
        // Access elements
        for (int i = 0; i < 10; ++i) {
            sb.Emit(OpCode::DUP);
            sb.EmitPush(static_cast<int64_t>(i));
            sb.Emit(OpCode::PICKITEM);
            sb.Emit(OpCode::DROP);
        }
        
        ExecutionEngine engine;
        auto script = CreateScript(sb.ToArray());
        engine.LoadScript(script);
        auto result = engine.Execute();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * 10);
}
BENCHMARK(BM_VM_Array_Access);

// ============================================================================
// String Operations Benchmarks
// ============================================================================

static void BM_VM_String_Concat(benchmark::State& state) {
    for (auto _ : state) {
        ScriptBuilder sb;
        sb.EmitPush("Hello, ");
        sb.EmitPush("World!");
        sb.Emit(OpCode::CAT);
        
        ExecutionEngine engine;
        auto script = CreateScript(sb.ToArray());
        engine.LoadScript(script);
        auto result = engine.Execute();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_VM_String_Concat);

static void BM_VM_String_Substring(benchmark::State& state) {
    for (auto _ : state) {
        ScriptBuilder sb;
        sb.EmitPush("Hello, World!");
        sb.EmitPush(static_cast<int64_t>(7)); // Start index
        sb.EmitPush(static_cast<int64_t>(5));  // Length
        sb.Emit(OpCode::SUBSTR);
        
        ExecutionEngine engine;
        auto script = CreateScript(sb.ToArray());
        engine.LoadScript(script);
        auto result = engine.Execute();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_VM_String_Substring);

// ============================================================================
// Logical Operations Benchmarks
// ============================================================================

static void BM_VM_Logic_And(benchmark::State& state) {
    for (auto _ : state) {
        ScriptBuilder sb;
        sb.EmitPush(true);
        sb.EmitPush(false);
        sb.Emit(OpCode::BOOLAND);
        
        ExecutionEngine engine;
        auto script = CreateScript(sb.ToArray());
        engine.LoadScript(script);
        auto result = engine.Execute();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_VM_Logic_And);

static void BM_VM_Logic_Complex(benchmark::State& state) {
    for (auto _ : state) {
        ScriptBuilder sb;
        // (true AND false) OR (true AND true)
        sb.EmitPush(true);
        sb.EmitPush(false);
        sb.Emit(OpCode::BOOLAND);
        sb.EmitPush(true);
        sb.EmitPush(true);
        sb.Emit(OpCode::BOOLAND);
        sb.Emit(OpCode::BOOLOR);
        
        ExecutionEngine engine;
        auto script = CreateScript(sb.ToArray());
        engine.LoadScript(script);
        auto result = engine.Execute();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_VM_Logic_Complex);

// ============================================================================
// Complex Script Execution Benchmarks
// ============================================================================

static void BM_VM_Fibonacci(benchmark::State& state) {
    for (auto _ : state) {
        ScriptBuilder sb;
        // Calculate Fibonacci(10)
        sb.EmitPush(static_cast<int64_t>(10)); // n
        sb.EmitPush(static_cast<int64_t>(0));  // fib(0)
        sb.EmitPush(static_cast<int64_t>(1));  // fib(1)
        
        // Loop n-1 times
        int loop_start = sb.GetPosition();
        sb.Emit(OpCode::ROT); // Rotate n to top
        sb.Emit(OpCode::DEC); // n--
        sb.Emit(OpCode::DUP); // Duplicate n
        sb.EmitPush(static_cast<int64_t>(0));
        sb.Emit(OpCode::GT); // n > 0?
        sb.EmitJump(OpCode::JMPIFNOT, 15); // Exit if false
        
        sb.Emit(OpCode::ROT); // Rotate fib(n-2) to top
        sb.Emit(OpCode::ROT); // Rotate fib(n-1) to top
        sb.Emit(OpCode::DUP);
        sb.Emit(OpCode::ROT);
        sb.Emit(OpCode::ADD); // fib(n) = fib(n-1) + fib(n-2)
        sb.EmitJump(OpCode::JMP, loop_start - sb.GetPosition());
        
        sb.Emit(OpCode::DROP); // Drop n
        sb.Emit(OpCode::NIP);  // Remove fib(n-2)
        
        ExecutionEngine engine;
        auto script = CreateScript(sb.ToArray());
        engine.LoadScript(script);
        auto result = engine.Execute();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_VM_Fibonacci);

static void BM_VM_ComplexCalculation(benchmark::State& state) {
    for (auto _ : state) {
        ScriptBuilder sb;
        
        // Complex calculation with multiple operations
        for (int i = 1; i <= 20; ++i) {
            sb.EmitPush(static_cast<int64_t>(i));
            if (i > 1) {
                if (i % 2 == 0) {
                    sb.Emit(OpCode::ADD);
                } else {
                    sb.Emit(OpCode::MUL);
                }
            }
        }
        
        ExecutionEngine engine;
        auto script = CreateScript(sb.ToArray());
        engine.LoadScript(script);
        auto result = engine.Execute();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * 20);
}
BENCHMARK(BM_VM_ComplexCalculation);

// ============================================================================
// Script Loading and Initialization Benchmarks
// ============================================================================

static void BM_VM_ScriptLoad_Small(benchmark::State& state) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(42));
    auto scriptData = sb.ToArray();
    
    for (auto _ : state) {
        ExecutionEngine engine;
        auto script = CreateScript(scriptData);
        engine.LoadScript(script);
        benchmark::DoNotOptimize(engine.GetState());
    }
    
    state.SetBytesProcessed(state.iterations() * scriptData.Size());
}
BENCHMARK(BM_VM_ScriptLoad_Small);

static void BM_VM_ScriptLoad_Large(benchmark::State& state) {
    ScriptBuilder sb;
    for (int i = 0; i < 1000; ++i) {
        sb.EmitPush(static_cast<int64_t>(i));
        sb.Emit(OpCode::DROP);
    }
    auto scriptData = sb.ToArray();
    
    for (auto _ : state) {
        ExecutionEngine engine;
        auto script = CreateScript(scriptData);
        engine.LoadScript(script);
        benchmark::DoNotOptimize(engine.GetState());
    }
    
    state.SetBytesProcessed(state.iterations() * scriptData.Size());
}
BENCHMARK(BM_VM_ScriptLoad_Large);

// ============================================================================
// Exception Handling Benchmarks
// ============================================================================

static void BM_VM_DivisionByZero(benchmark::State& state) {
    for (auto _ : state) {
        ScriptBuilder sb;
        sb.EmitPush(static_cast<int64_t>(10));
        sb.EmitPush(static_cast<int64_t>(0));
        sb.Emit(OpCode::DIV);
        
        ExecutionEngine engine;
        auto script = CreateScript(sb.ToArray());
        engine.LoadScript(script);
        auto result = engine.Execute();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_VM_DivisionByZero);

static void BM_VM_StackOverflow(benchmark::State& state) {
    for (auto _ : state) {
        ScriptBuilder sb;
        // Try to push many items (but stay within test limits)
        for (int i = 0; i < 100; ++i) {
            sb.EmitPush(static_cast<int64_t>(i));
        }
        
        ExecutionEngine engine;
        auto script = CreateScript(sb.ToArray());
        engine.LoadScript(script);
        auto result = engine.Execute();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * 100);
}
BENCHMARK(BM_VM_StackOverflow);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();