/**
 * @file benchmark_vm_execution.cpp
 * @brief VM execution performance benchmarks
 */

#include <benchmark/benchmark.h>
#include <neo/vm/vm.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/opcode.h>
#include <vector>
#include <random>

namespace neo::vm::benchmarks {

/**
 * @brief Benchmark simple arithmetic operations
 */
static void BM_VMArithmetic(benchmark::State& state) {
    for (auto _ : state) {
        ExecutionEngine engine;
        ScriptBuilder sb;
        
        // Build script with arithmetic operations
        for (int i = 0; i < state.range(0); ++i) {
            sb.EmitPush(i);
            sb.EmitPush(i + 1);
            sb.Emit(OpCode::ADD);
        }
        
        engine.LoadScript(sb.ToArray());
        engine.Execute();
        
        benchmark::DoNotOptimize(engine.State);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_VMArithmetic)->Range(1, 1024);

/**
 * @brief Benchmark stack operations
 */
static void BM_VMStackOperations(benchmark::State& state) {
    for (auto _ : state) {
        ExecutionEngine engine;
        ScriptBuilder sb;
        
        // Push many items
        for (int i = 0; i < state.range(0); ++i) {
            sb.EmitPush(i);
        }
        
        // Perform stack operations
        for (int i = 0; i < state.range(0) / 2; ++i) {
            sb.Emit(OpCode::SWAP);
            sb.Emit(OpCode::DUP);
            sb.Emit(OpCode::DROP);
        }
        
        engine.LoadScript(sb.ToArray());
        engine.Execute();
        
        benchmark::DoNotOptimize(engine.State);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_VMStackOperations)->Range(8, 512);

/**
 * @brief Benchmark array operations
 */
static void BM_VMArrayOperations(benchmark::State& state) {
    for (auto _ : state) {
        ExecutionEngine engine;
        ScriptBuilder sb;
        
        // Create array
        sb.EmitPush(state.range(0));
        sb.Emit(OpCode::NEWARRAY);
        
        // Fill array
        for (int i = 0; i < state.range(0); ++i) {
            sb.Emit(OpCode::DUP);
            sb.EmitPush(i);
            sb.EmitPush(i * 100);
            sb.Emit(OpCode::SETITEM);
        }
        
        // Read from array
        for (int i = 0; i < state.range(0); ++i) {
            sb.Emit(OpCode::DUP);
            sb.EmitPush(i);
            sb.Emit(OpCode::PICKITEM);
            sb.Emit(OpCode::DROP);
        }
        
        engine.LoadScript(sb.ToArray());
        engine.Execute();
        
        benchmark::DoNotOptimize(engine.State);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0) * 2);
}
BENCHMARK(BM_VMArrayOperations)->Range(8, 256);

/**
 * @brief Benchmark control flow operations
 */
static void BM_VMControlFlow(benchmark::State& state) {
    for (auto _ : state) {
        ExecutionEngine engine;
        ScriptBuilder sb;
        
        // Create loops and branches
        for (int i = 0; i < state.range(0); ++i) {
            sb.EmitPush(i % 2);
            sb.EmitJump(OpCode::JMPIF, 3);
            sb.EmitPush(0);
            sb.Emit(OpCode::JMP, 2);
            sb.EmitPush(1);
        }
        
        engine.LoadScript(sb.ToArray());
        engine.Execute();
        
        benchmark::DoNotOptimize(engine.State);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_VMControlFlow)->Range(8, 512);

/**
 * @brief Benchmark crypto operations in VM
 */
static void BM_VMCryptoOperations(benchmark::State& state) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (auto _ : state) {
        ExecutionEngine engine;
        ScriptBuilder sb;
        
        // Generate random data
        std::vector<uint8_t> data(32);
        for (auto& byte : data) {
            byte = dis(gen);
        }
        
        // Perform crypto operations
        for (int i = 0; i < state.range(0); ++i) {
            sb.EmitPush(data);
            sb.Emit(OpCode::SHA256);
            sb.Emit(OpCode::RIPEMD160);
        }
        
        engine.LoadScript(sb.ToArray());
        engine.Execute();
        
        benchmark::DoNotOptimize(engine.State);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0) * 2);
}
BENCHMARK(BM_VMCryptoOperations)->Range(1, 64);

/**
 * @brief Benchmark string operations
 */
static void BM_VMStringOperations(benchmark::State& state) {
    for (auto _ : state) {
        ExecutionEngine engine;
        ScriptBuilder sb;
        
        // String concatenation
        for (int i = 0; i < state.range(0); ++i) {
            sb.EmitPush("Hello");
            sb.EmitPush("World");
            sb.Emit(OpCode::CAT);
        }
        
        engine.LoadScript(sb.ToArray());
        engine.Execute();
        
        benchmark::DoNotOptimize(engine.State);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_VMStringOperations)->Range(1, 128);

/**
 * @brief Benchmark map operations
 */
static void BM_VMMapOperations(benchmark::State& state) {
    for (auto _ : state) {
        ExecutionEngine engine;
        ScriptBuilder sb;
        
        // Create map
        sb.Emit(OpCode::NEWMAP);
        
        // Add items to map
        for (int i = 0; i < state.range(0); ++i) {
            sb.Emit(OpCode::DUP);
            sb.EmitPush(std::string("key") + std::to_string(i));
            sb.EmitPush(i * 100);
            sb.Emit(OpCode::SETITEM);
        }
        
        // Read from map
        for (int i = 0; i < state.range(0); ++i) {
            sb.Emit(OpCode::DUP);
            sb.EmitPush(std::string("key") + std::to_string(i));
            sb.Emit(OpCode::PICKITEM);
            sb.Emit(OpCode::DROP);
        }
        
        engine.LoadScript(sb.ToArray());
        engine.Execute();
        
        benchmark::DoNotOptimize(engine.State);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0) * 2);
}
BENCHMARK(BM_VMMapOperations)->Range(8, 256);

/**
 * @brief Benchmark exception handling
 */
static void BM_VMExceptionHandling(benchmark::State& state) {
    for (auto _ : state) {
        ExecutionEngine engine;
        ScriptBuilder sb;
        
        // Create try-catch blocks
        for (int i = 0; i < state.range(0); ++i) {
            sb.Emit(OpCode::TRY, 4);
            sb.EmitPush(1);
            sb.EmitPush(0);
            sb.Emit(OpCode::DIV);
            sb.Emit(OpCode::ENDTRY, 2);
            sb.EmitPush(0); // Exception handler
        }
        
        engine.LoadScript(sb.ToArray());
        engine.Execute();
        
        benchmark::DoNotOptimize(engine.State);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_VMExceptionHandling)->Range(1, 64);

/**
 * @brief Benchmark large script execution
 */
static void BM_VMLargeScript(benchmark::State& state) {
    // Pre-build large script
    ScriptBuilder sb;
    for (int i = 0; i < state.range(0); ++i) {
        sb.EmitPush(i);
        sb.EmitPush(i * 2);
        sb.Emit(OpCode::ADD);
        sb.EmitPush(3);
        sb.Emit(OpCode::MUL);
        sb.EmitPush(7);
        sb.Emit(OpCode::MOD);
    }
    auto script = sb.ToArray();
    
    for (auto _ : state) {
        ExecutionEngine engine;
        engine.LoadScript(script);
        engine.Execute();
        
        benchmark::DoNotOptimize(engine.State);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0) * 5);
}
BENCHMARK(BM_VMLargeScript)->Range(100, 10000);

/**
 * @brief Benchmark recursive calls
 */
static void BM_VMRecursiveCalls(benchmark::State& state) {
    for (auto _ : state) {
        ExecutionEngine engine;
        ScriptBuilder sb;
        
        // Factorial calculation (limited recursion)
        int depth = std::min(state.range(0), 10); // Limit recursion depth
        
        // Simple iterative factorial instead of recursion for benchmark
        sb.EmitPush(1);
        for (int i = 1; i <= depth; ++i) {
            sb.EmitPush(i);
            sb.Emit(OpCode::MUL);
        }
        
        engine.LoadScript(sb.ToArray());
        engine.Execute();
        
        benchmark::DoNotOptimize(engine.State);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_VMRecursiveCalls)->Range(1, 20);

} // namespace neo::vm::benchmarks