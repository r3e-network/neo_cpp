# Neo N3 C# to C++ Conversion Checklist

## 🚨 CRITICAL COMPONENTS (Must Complete First)

### BigDecimal Implementation
- [ ] Create `include/neo/core/big_decimal.h`
- [ ] Implement `src/core/big_decimal.cpp`
- [ ] Add arithmetic operations (+, -, *, /, %)
- [ ] Add comparison operations (<, >, ==, !=, <=, >=)
- [ ] Add serialization/deserialization
- [ ] Convert `tests/unit/core/test_big_decimal.cpp` ✅
- [ ] All BigDecimal tests passing

### InteropService Implementation  
- [ ] Create `include/neo/smartcontract/interop_service.h`
- [ ] Implement `src/smartcontract/interop_service.cpp`
- [ ] Add system call handlers
- [ ] Add native contract interops
- [ ] Convert `tests/unit/smartcontract/test_interop_service.cpp` ✅
- [ ] All InteropService tests passing

### Core System Components
- [ ] Complete `include/neo/core/neo_system.h`
- [ ] Implement `src/core/neo_system.cpp`
- [ ] Complete `include/neo/core/protocol_settings.h`
- [ ] Implement `src/core/protocol_settings.cpp`
- [ ] Convert `tests/unit/core/test_neo_system.cpp` ✅
- [ ] Convert `tests/unit/core/test_protocol_settings.cpp` ✅

## 📋 UNIT TEST CONVERSION STATUS

### Core Tests (7 total)
- [ ] `test_big_decimal.cpp` ✅ (Generated)
- [ ] `test_neo_system.cpp` ✅ (Generated)
- [ ] `test_protocol_settings.cpp` ✅ (Generated)
- [ ] `test_helper.cpp` ✅ (Generated)
- [ ] `test_uint160.cpp` ✅ (Generated)
- [ ] `test_uint256.cpp` ✅ (Generated)
- [ ] `test_uintbenchmarks.cpp` ✅ (Generated)

### VM Tests (9 total)
- [ ] `test_debugger.cpp` ✅ (Generated)
- [ ] `test_evaluation_stack.cpp` ✅ (Generated)
- [ ] `test_execution_context.cpp` ✅ (Generated)
- [ ] `test_reference_counter.cpp` ✅ (Generated)
- [ ] `test_script.cpp` ✅ (Generated)
- [ ] `test_script_builder.cpp` ✅ (Generated)
- [ ] `test_slot.cpp` ✅ (Generated)
- [ ] `test_stack_item.cpp` ✅ (Generated)
- [ ] `test_struct.cpp` ✅ (Generated)

### SmartContract Tests (25+ total)
- [ ] `test_application_engine.cpp` ✅ (Generated)
- [ ] `test_interop_service.cpp` ✅ (Generated)
- [ ] `test_contract_parameter.cpp` ✅ (Generated)
- [ ] `test_nef_file.cpp` ✅ (Generated)
- [ ] `test_contract_manifest.cpp` ✅ (Generated)
- [ ] `test_binary_serializer.cpp` ✅ (Generated)
- [ ] `test_json_serializer.cpp` ✅ (Generated)
- [ ] `test_storage.cpp` ✅ (Generated)
- [ ] `test_syscalls.cpp` ✅ (Generated)
- [ ] ... (16 more generated)

### Network Tests (15+ total)
- [ ] `test_local_node.cpp` ✅ (Generated)
- [ ] `test_remote_node.cpp` ✅ (Generated)
- [ ] `test_message.cpp` ✅ (Generated)
- [ ] `test_upnp.cpp` ✅ (Generated)
- [ ] ... (11 more generated)

### Cryptography Tests (8+ total)
- [ ] `test_crypto.cpp` ✅ (Generated)
- [ ] `test_ec_point.cpp` ✅ (Generated)
- [ ] `test_keypair.cpp` ✅ (Generated)
- [ ] `test_merkle_tree.cpp` ✅ (Generated)
- [ ] ... (4 more generated)

## 🔧 IMPLEMENTATION TASKS

### Missing Sub-modules
- [ ] Implement `Builders/` module
  - [ ] TransactionBuilder
  - [ ] WitnessBuilder
  - [ ] SignerBuilder
- [ ] Implement `Sign/` module
  - [ ] Digital signature components
- [ ] Implement `IEventHandlers/` interfaces
  - [ ] Event handling system

### Plugin System
- [ ] Convert ApplicationLogs plugin tests
- [ ] Convert DBFTPlugin tests  
- [ ] Convert OracleService tests
- [ ] Convert RpcServer tests
- [ ] Convert Storage tests

## 📊 DAILY PROGRESS TRACKING

### Week 1 Goals
- [ ] Day 1: BigDecimal implementation started
- [ ] Day 2: BigDecimal tests passing
- [ ] Day 3: NeoSystem implementation started
- [ ] Day 4: NeoSystem tests passing
- [ ] Day 5: ProtocolSettings implementation
- [ ] Weekend: UInt160/UInt256 completion

### Week 2 Goals
- [ ] Day 1: InteropService implementation started
- [ ] Day 2-3: InteropService core functionality
- [ ] Day 4-5: InteropService tests passing
- [ ] Weekend: ApplicationEngine completion

## 🎯 COMPLETION METRICS

### Current Status
- **Modules Converted**: 10/12 (83.3%)
- **Tests Converted**: 4/16 (25.0%)
- **Files Converted**: 21/173 (12.1%)
- **Critical Components**: 0/2 (0%)

### Target Status (2 weeks)
- **Modules Converted**: 12/12 (100%)
- **Tests Converted**: 12/16 (75%)
- **Files Converted**: 100/173 (58%)
- **Critical Components**: 2/2 (100%)

### Target Status (6 weeks)
- **Modules Converted**: 12/12 (100%)
- **Tests Converted**: 16/16 (100%)
- **Files Converted**: 160/173 (92%)
- **Critical Components**: 2/2 (100%)

## 🚀 QUICK START COMMANDS

### Build and Test
```bash
# Build the project
mkdir build && cd build
cmake .. -DNEO_BUILD_TESTS=ON
make -j$(nproc)

# Run specific tests
./test_big_decimal
./test_neo_system
./test_interop_service

# Run all tests
ctest
```

### Validation Commands
```bash
# Check conversion status
python3 scripts/validate_conversion.py

# Generate missing tests (if needed)
python3 scripts/generate_missing_tests.py
```

## 📝 NOTES

### Generated Files Summary
- **Total Test Files Generated**: 173
- **CMakeLists.txt Entries**: Ready to copy
- **Test Templates**: Include TODO comments for implementation
- **File Structure**: Organized by module

### Key Reference Files
- **C# BigDecimal**: `neo/src/Neo/BigDecimal.cs`
- **C# NeoSystem**: `neo/src/Neo/NeoSystem.cs`
- **C# InteropService**: `neo/src/Neo/SmartContract/InteropService.*.cs`
- **C# Tests**: `neo/tests/Neo.UnitTests/UT_*.cs`

### Priority Order
1. 🚨 BigDecimal (blocks many other components)
2. 🚨 InteropService (critical for smart contracts)
3. ⚠️ NeoSystem (system orchestration)
4. ⚠️ ProtocolSettings (configuration)
5. ✅ Test conversion (validation)

---
**Last Updated**: $(date)
**Status**: Ready for implementation
**Next Action**: Start BigDecimal implementation