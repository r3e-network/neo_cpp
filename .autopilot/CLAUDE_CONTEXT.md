# AutoClaude Project Context

Generated at: 2025-07-30T08:14:20.519Z

---

# Project Context

## Workspace
- **Root**: /Users/jinghuiliao/git/r3e/neo_cpp
- **Type**: single
- **Last Updated**: 2025-07-30T08:13:52.687Z

## Statistics
- **Total Files**: 2854
- **Estimated Lines**: 381372
- **Average File Size**: 9482 bytes

## Languages
- **csharp**: 672300 files
- **cpp**: 549160 files
- **json**: 398147 files
- **markdown**: 29879 files
- **shellscript**: 11952 files
- **python**: 11952 files
- **javascript**: 5229 files
- **yaml**: 747 files

## Project Structure
- **Main Languages**: Not detected
- **Frameworks**: None detected
- **Test Frameworks**: None detected
- **Build Tools**: None detected

## Configuration Files



## Largest Files
- tests/unit/io/test_io (2828KB)
- tests/unit/json/test_json (2295KB)
- tests/unit/extensions/test_extensions (1518KB)
- tests/unit/cryptography/test_cryptography (1374KB)
- include/nlohmann/json.hpp (898KB)
- neo_csharp/src/Neo.GUI/neo.ico (361KB)
- third_party/httplib/httplib.h (355KB)
- duplicate_analysis.json (161KB)
- neo_csharp/benchmarks/Neo.Json.Benchmarks/Data/RpcTestCases.json (115KB)
- neo_csharp/tests/Neo.RpcClient.Tests/RpcTestCases.json (115KB)


---

# Task Summary

## Overall Statistics
- **Total Tasks**: 0
- **Pending**: 0
- **In Progress**: 0
- **Completed**: 0
- **Failed**: 0

## Current Session
- **Session ID**: msg_mdpj09ow_k7cxsg
- **Started**: 2025-07-30T05:29:20.144Z
- **Tasks in Session**: 0

## Recent Tasks



---

## Unfinished Tasks
No unfinished tasks

---

## Recent Changes

### Git Status
```
M  .autoclaude/scripts/ai-code-review.js
M  .autoclaude/scripts/build-check.js
A  .autoclaude/scripts/build-check.sh
M  .autoclaude/scripts/doc-generator.js
M  .autoclaude/scripts/format-check.js
A  .autoclaude/scripts/format-check.sh
M  .autoclaude/scripts/github-actions.js
A  .autoclaude/scripts/github-actions.sh
M  .autoclaude/scripts/production-readiness.js
A  .autoclaude/scripts/production-readiness.sh
M  .autoclaude/scripts/tdd-automation.js
M  .autoclaude/scripts/test-check.js
AM .autoclaude/scripts/test-check.sh
 M apps/CMakeLists.txt
 M apps/neo_node.cpp
 M apps/neo_node_production.cpp
 M include/neo/blockchain/header.h
 M include/neo/cryptography/ecc/secp256r1.h
 M include/neo/ledger/block.h
 M include/neo/ledger/signer.h
 M include/neo/ledger/witness.h
 M include/neo/network/p2p/payloads/block_payload.h
 M include/neo/network/p2p/payloads/extensible_payload.h
 M include/neo/network/p2p/payloads/neo3_transaction.h
 M include/neo/vm/compound_items.h
 M src/console_service/service_proxy.cpp
 M src/cryptography/CMakeLists.txt
 M src/cryptography/bls12_381.cpp
 M src/cryptography/ecc/keypair.cpp
 M src/cryptography/ecc/secp256r1.cpp
 M src/network/CMakeLists.txt
 M src/network/p2p/payloads/block_payload.cpp
 M src/network/p2p/payloads/extensible_payload.cpp
 M src/network/p2p/payloads/neo3_transaction.cpp
 M src/network/p2p/protocol_handler.cpp
 M src/smartcontract/CMakeLists.txt
 M src/smartcontract/contract_parameters_context.cpp
 M src/smartcontract/native/CMakeLists.txt
 M src/vm/compound_items.cpp
 M src/vm/execution_engine.cpp
 M src/vm/jump_table_arithmetic_bitwise.cpp
 M src/vm/jump_table_arithmetic_numeric.cpp
 M src/vm/jump_table_compound_array.cpp
 M src/vm/jump_table_compound_map.cpp
 M src/vm/jump_table_stack.cpp
 M src/wallets/CMakeLists.txt
 M tests/plugins/CMakeLists.txt
 M tests/unit/console_service/CMakeLists.txt
 M tests/unit/cryptography/test_bls12_381_complete.cpp
 M tests/unit/extensions/CMakeLists.txt
 M tests/unit/extensions/test_integer_extensions.cpp
 M tests/unit/json/CMakeLists.txt
 M tests/unit/ledger/test_block.cpp
 M tests/unit/ledger/test_header_cache.cpp
 M tests/unit/ledger/test_transaction.cpp
 M tests/unit/network/test_peer_discovery.cpp
 M tests/unit/smartcontract/native/test_contract_management.cpp
 M tests/unit/smartcontract/test_contract_abi.cpp
 M tests/unit/smartcontract/test_contract_events.cpp
 M tests/unit/smartcontract/test_contract_groups.cpp
 M tests/unit/smartcontract/test_contract_manifest.cpp
 M tests/unit/smartcontract/test_contract_methods.cpp
 M tests/unit/smartcontract/test_contract_parameters.cpp
 M tests/unit/smartcontract/test_contract_permissions.cpp
 M tests/unit/smartcontract/test_contract_state.cpp
 M tests/unit/smartcontract/test_interop_service.cpp
 M tests/unit/smartcontract/test_nef_file.cpp
 M tests/unit/vm/opcodes/CMakeLists.txt
 M tests/unit/vm/opcodes/test_opcodes_arithmetic.cpp
 M tests/unit/vm/script_converter.h
 M tests/unit/vm/test_reference_counter.cpp
 M tests/unit/wallets/test_asset_descriptor.cpp
 M tools/CMakeLists.txt
 M tools/test_rpc_server.cpp
?? .autoclaude/scripts/code-understanding-check.sh
?? .autoclaude/scripts/context-check.sh
?? .autoclaude/scripts/dependency-check.sh
?? .autoclaude/scripts/integration-testing-check.sh
?? .autoclaude/scripts/performance-check.sh
?? .autoclaude/scripts/security-audit-check.sh
?? .autopilot/
?? TEST_COVERAGE_ANALYSIS.md
?? src/cryptography/ecc/secp256r1_simple.cpp
?? test_array_size.cpp
?? test_reference_counter_debug.cpp

```

### Recent Commits
```
a311db9e refactor: enhance C# consistency and error handling
243f39de refactor: implement production readiness fixes and code cleanup
c209ba57 fix: resolve remaining compilation errors in native contract tests
4d40057f fix: use DataCache instead of MemoryStoreView in tests
ebcbd3c0 fix: resolve remaining compilation errors in tests
19952a72 fix: resolve compilation errors in native contract tests
2457df8e fix: resolve namespace and API issues in native contract tests
e238f34a fix: correct include path and add missing IsReadOnly method
b3413969 fix: add missing std::list include to cache.h
0850356f fix: complete block test Neo 3 compatibility updates

```

---

## Current File
No file currently open