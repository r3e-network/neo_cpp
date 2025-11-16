# C# → C++ Test Porting Status

This document tracks the parity effort between the upstream C# test suites (under `neo_csharp/tests`) and the C++ equivalents (under `tests/unit` and `tests/integration`). It is the working checklist for the “port every C# test” initiative.

## Inventory of C# Test Suites

| C# Test Project | Scope |
| --- | --- |
| `Neo.UnitTests` | Core types, ledger, crypto, smart contracts, network |
| `Neo.VM.Tests` | Virtual machine opcode/engine coverage |
| `Neo.Json.UnitTests` | JSON parsing utilities |
| `Neo.Extensions.Tests` | Extension helpers (collections, numerics, strings) |
| `Neo.Cryptography.MPTTrie.Tests` | Patricia trie + storage proofs |
| `Neo.Cryptography.BLS12_381.Tests` | BLS cryptography |
| `Neo.ConsoleService.Tests` | Base console service behaviors |
| `Neo.CLI.Tests` | CLI command handling |
| `Neo.Plugins.*.Tests` | ApplicationLogs, DBFTPlugin, Oracle, RestServer, RpcServer, SignClient, SQLiteWallet, StateService, Storage |
| `Neo.RpcClient.Tests` | SDK RPC client + transaction builder |

## Current C++ Coverage Snapshot

| C++ Test Target(s) | Coverage Status |
| --- | --- |
| `tests/unit/core`, `tests/unit/extensions` | Partial parity with `Neo.UnitTests` (core types, UInt160/256, helpers) |
| `tests/unit/io`, `tests/unit/json` | Covers basic BinaryReader/BinaryWriter and JSON helpers; missing some advanced cases |
| `tests/unit/cryptography`, `tests/unit/smartcontract` | Partial ECC/VM coverage; VM opcodes still incomplete |
| `tests/unit/ledger`, `tests/unit/persistence` | Partial coverage for transactions/blocks/mempool |
| `tests/unit/network`, `tests/unit/network/p2p` | Many payload/LocalNode tests already ported; remaining peer discovery/integration tests pending |
| `tests/unit/rpc`, `tests/unit/plugins`, `tests/unit/cli` | RPC method, consensus, and RpcClient suites ported; plugin/CLI suites still partial |
| `tests/unit/settings` (new) | Exercises JSON configuration loader |

## Gap Analysis (High-Level)

1. **Core/Extensions/IO** – Mostly ported, but remaining helper/edge-case tests need parity.
2. **VM/SmartContract** – Opcode tables and ApplicationEngine tests not fully ported (see Phase 2 table).
3. **Ledger/Persistence** – Need deeper transaction verification, block persistence, storage snapshots.
4. **Network/P2P** – Peer discovery, peer reputation, and BlockSync integration tests missing.
5. **Plugins/RPC** – Minimal coverage; need dedicated suites per plugin.
6. **CLI/ConsoleService** – Partial command tests only.
7. **RPC Client / SDK** – RpcClient happy-path/error tests landed; SDK builder tests still pending.

## Next Steps (Phase 1 – Core & IO)

1. **Audit `Neo.UnitTests` Core Classes**
   - Enumerate the remaining C# test classes under `Neo.UnitTests` (core helpers, UT_* files).
   - Track their status in the table below and create C++ counterparts for anything marked “Missing”.
2. **Complete IO & JSON Coverage**
   - Port residual BinaryReader/BinaryWriter edge cases (endian swaps, var-bytes limits).
   - Mirror `Neo.Json.UnitTests` behaviors in `tests/unit/json`.
3. **Shared Fixtures**
   - Add reusable helpers for temporary files, deterministic random data, and exception assertions to speed up future ports.
4. **Tracking**
   - For each C# test file ported, log it in this document (append a checklist) so progress is measurable.

Once Phase 1 is complete, we’ll proceed to Phase 2 (Cryptography + VM) using the same approach.

### Phase 1 Progress (Core / IO)

| C# Test File | Status | C++ Equivalent / Notes |
| --- | --- | --- |
| `UT_BigDecimal.cs` | ✅ Ported | `tests/unit/core/test_big_decimal.cpp` |
| `UT_DataCache.cs` | ✅ Ported | `tests/unit/persistence/test_data_cache.cpp` |
| `UT_Helper.cs` | ✅ Ported | `tests/unit/core/test_helper.cpp` |
| `UT_NeoSystem.cs` | ✅ Ported | `tests/unit/core/test_neo_system.cpp` |
| `UT_ProtocolSettings.cs` | ✅ Ported | `tests/unit/test_protocol_settings.cpp` & `_all_methods_complete.cpp` |
| `UT_UInt160.cs` | ✅ Ported | `tests/unit/core/test_core_uint160_complete.cpp` |
| `UT_UInt256.cs` | ✅ Ported | `tests/unit/core/test_core_uint256_complete.cpp` |
| `TestBlockchain.cs` | ✅ Ported | `tests/unit/core/test_blockchain.cpp` + ledger suites |
| `TestWalletAccount.cs` | ✅ Ported | `tests/unit/wallets/test_wallet_account.cpp` |
| `UT_ContractState.cs` | ✅ Ported | Covered by `tests/unit/smartcontract/test_contract.cpp` |
| `TestVerifiable.cs` | n/a | Helper class only (C++ tests use existing mocks) |
| `IO/UT_IOHelper.cs` | ✅ Ported | `tests/unit/io/test_io_helper.cpp` |
| `IO/UT_MemoryReader.cs` | ✅ Ported | `tests/unit/io/test_io_extended.cpp` |
| `IO/Caching/UT_Cache.cs` | ✅ Ported | `tests/unit/io/caching/test_cache.cpp` |
| `IO/Caching/UT_ECPointCache.cs` | ✅ Ported | `tests/unit/io/caching/test_ecpointcache.cpp` |
| `IO/Caching/UT_HashSetCache.cs` | ✅ Ported | `tests/unit/io/caching/test_hashsetcache.cpp` |
| `IO/Caching/UT_KeyedCollectionSlim.cs` | ✅ Ported | `tests/unit/io/caching/test_keyedcollectionslim.cpp` |
| `IO/Caching/UT_LRUCache.cs` | ✅ Ported | `tests/unit/io/caching/test_lrucache.cpp` |
| `IO/Caching/UT_ReflectionCache.cs` | ✅ Ported | `tests/unit/io/caching/test_reflectioncache.cpp` |
| `IO/Caching/UT_IndexedQueue.cs` | ✅ Ported | `tests/unit/io/caching/test_indexedqueue.cpp` |
| `IO/Caching/UT_RelayCache.cs` | ✅ Ported | `tests/unit/io/caching/test_relaycache.cpp` |

## Phase 2 – Cryptography & VM

- **Cryptography**: All `Neo.UnitTests/Cryptography/UT_*.cs` files have matching C++ tests under `tests/unit/cryptography`.
- **VM**: Most VM tests are ported; the table below tracks the remaining files to double-check.

| C# VM Test File | Status | C++ Equivalent / Notes |
| --- | --- | --- |
| `UT_Debugger.cs` | ✅ Ported | `tests/unit/vm/test_debugger*.cpp` |
| `UT_EvaluationStack.cs` | ✅ Ported | `tests/unit/vm/test_evaluation_stack*.cpp` |
| `UT_ExecutionContext.cs` | ✅ Ported | `tests/unit/vm/test_execution_context*.cpp` |
| `UT_ReferenceCounter.cs` | ✅ Ported | `tests/unit/vm/test_reference_counter*.cpp` |
| `UT_Script.cs` | ✅ Ported | `tests/unit/vm/test_script*.cpp` |
| `UT_ScriptBuilder.cs` | ✅ Ported | `tests/unit/vm/test_script_builder*.cpp` |
| `UT_Slot.cs` | ✅ Ported | `tests/unit/vm/test_slot*.cpp` |
| `UT_StackItem.cs` | ✅ Ported | `tests/unit/vm/test_stack_item*.cpp` |
| `UT_Struct.cs` | ✅ Ported | `tests/unit/vm/test_struct*.cpp` |
| `UT_VMJson.cs` | ✅ Ported | `tests/unit/vm/test_vmjson.cpp` |

## Phase 3 – Ledger & Persistence

| C# Test File | Status | C++ Equivalent / Notes |
| --- | --- | --- |
| `Ledger/UT_Blockchain.cs` | ✅ Ported | `tests/unit/ledger/test_blockchain*.cpp` |
| `Ledger/UT_MemoryPool.cs` | ✅ Ported | `tests/unit/ledger/test_memorypool*.cpp` |
| `Ledger/UT_TrimmedBlock.cs` | ✅ Ported | `tests/unit/ledger/test_trimmedblock.cpp` |
| `Persistence/UT_CloneCache.cs` | ✅ Ported | `tests/unit/persistence/test_clonecache.cpp` |
| `Persistence/UT_DataCache.cs` | ✅ Ported | `tests/unit/persistence/test_data_cache.cpp` |
| `Persistence/UT_MemoryClonedCache.cs` | ✅ Ported | `tests/unit/persistence/test_memoryclonedcache.cpp` |

## Phase 5 – RPC Client & Plugins

| C# Test File | Status | C++ Equivalent / Notes |
| --- | --- | --- |
| `Neo.RpcClient.Tests/UT_RpcClient.cs` | ✅ Ported (core cases) | `tests/unit/rpc/test_rpc_client.cpp` with injectable `IHttpClient` mocks, async error coverage |
| `Neo.RpcClient.Tests/TestUtils.cs` | ✅ Consolidated | Inlined into `MockHttpClient` helpers (queued responses, error payloads) |
| `Neo.Network.RPC.Tests/RpcRequest + RpcResponse` | ✅ Ported | `tests/unit/rpc/test_rpc_request.cpp` / `test_rpc_response.cpp` back in the default build to validate serialization round-trips |
| `Neo.Network.RPC.Tests/RpcSessionManager` | ✅ Ported | `tests/unit/rpc/test_rpc_session_manager.cpp` now compiled with `test_rpc` to exercise iterator persistence |
| `Neo.Plugins.RpcServer.Tests/*` | ➖ Partial | `tests/unit/rpc/test_rpc_server.cpp` covers dispatcher/plug-in hooks and sessions-disabled error (-601) behavior; plugin suites (ApplicationLogs, Oracle, etc.) remain TODO |

**Notes:**
- The RPC unit tests now build by default (no longer hidden behind `NEO_ENABLE_EXTENDED_UNIT_TESTS`).
- `test_rpc_request.cpp`, `test_rpc_response.cpp`, `test_rpc_session_manager.cpp`, and the new `test_rpc_server.cpp` run by default, giving coverage to the JSON-RPC envelopes, iterator plumbing, and dispatcher/plug-in hooks.
- `neo::rpc::RpcClient` gained a proper async pipeline (`RpcSendAsync` → `SendAsync` → `IHttpClient::PostAsync`), mirroring the C# `HttpClient` behavior so futures can be tested deterministically.
- Remaining Phase 5 work includes porting the SDK transaction builder tests and plugin suites (ApplicationLogs, Oracle, RestServer, Storage, etc.).
