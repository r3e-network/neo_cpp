# Consensus Engine Porting Plan

This document outlines the work required to restore dBFT consensus parity between
the Neo C++ node and the canonical C# implementation (Neo 3.x branch). It serves
as the master checklist for the porting effort, breaking the problem down into
phases with concrete deliverables.

## Phase 1 – Requirements & Architecture Alignment

### 1. Inventory of Components

| Area | C# Reference | Notes |
| ---- | ------------ | ----- |
| Consensus context/state | `ConsensusContext`, `ConsensusStateSnapshot` | Tracks validators, view, signatures, proposal payload, recovery cache. |
| Message payloads | `ChangeView`, `PrepareRequest`, `PrepareResponse`, `Commit`, `RecoveryMessage`, `RecoveryRequest` | Need complete serialization/deserialization and data members. |
| Service orchestration | `ConsensusService` | Runs consensus loop, timers, integrates with networking and blockchain. |
| Payload building helpers | `ConsensusPayload`, `ConsensusPayloadBuilder`, `ConsensusMessageFactory` | Required for hashing/signing and for LocalNode broadcasts. |
| Cryptographic helpers | EC key handling, multi-sign, `Crypto.Default` equivalents | Need to ensure C++ layer has matching helpers (likely in existing ecc module). |
| Storage integration | `ConsensusDataProvider`, recovery caches in datastore | Determine whether to persist consensus info across restarts (optional). |
| Configuration | `ProtocolConfiguration.ValidatorsCount`, `TimePerBlock`, `MaxBlockChangeView` | Already exposed by `neo::ProtocolSettings`, must be read by consensus. |
| Thread/timer primitives | `System.Timers`, `CancellationToken` | Need C++ equivalents – likely `std::thread`, `std::condition_variable`. |
| Logging/metrics | `ILogger`, metrics counters | Map to C++ logging subsystem. |

### 2. External Integration Points

* **NeoSystem** – Owner of the consensus service. Responsible for:
  * Constructing `ConsensusService` when consensus is enabled via config.
  * Passing blockchain, mempool, protocol settings, wallet/key info.
  * Starting/stopping consensus on node lifecycle events.
* **LocalNode** – Network front end:
  * Registers consensus message handlers.
  * Broadcasts consensus payloads produced by `ConsensusService`.
  * Forwards inbound consensus payloads to the service.
* **Blockchain / Ledger**:
  * Provides current block index, validators list, block persistence.
  * Offers transaction verification for proposals.
* **Wallet / Key Management**:
  * Supplies private key for signing consensus messages.
  * C# uses unlockable wallet; C++ may initially rely on simple key pair config.
* **Persistence (optional in initial cut)**:
  * Recovery caches stored in `DataCache` so recovery after restart is possible.

### 3. Prerequisites / Dependencies

| Dependency | Status | Action |
| ---------- | ------ | ------ |
| ECC / signing | `neo::cryptography::ecc` exists | Validate interface parity with C# `KeyPair`. |
| Block/transaction serialization | Available in ledger | Confirm compatibility with consensus payload expectations. |
| Threading primitives | `std::thread`, `std::condition_variable` | Identify whether additional timers/clock abstractions required. |
| Reliable timer | Not yet abstracted | Likely need helper class for consensus timeouts (view change, recovery). |
| Networking broadcast | `LocalNode::Broadcast` etc. | Ensure methods exist or extend to handle consensus payloads. |
| Configuration access | `neo::ProtocolSettings` | Extend if more consensus-related settings required. |

### 4. Test Strategy (Target)

* **Unit Tests**
  * Message serialization round-trips for each consensus message type.
  * `ConsensusContext` state transitions (e.g., update view, record response, quorum detection).
  * Timeout calculation (`GetTimeout`) vs. expected values.
* **Integration Tests (deterministic)**
  * Simulated 4-validator scenario using in-memory networking harness to validate:
    * Happy path: primary proposes, backups respond, block commits.
    * View change when primary stalls.
    * Recovery after network partition.
  * Tests can run single-threaded with fake timers to avoid flakiness.
* **Manual / CLI**
  * Spin up multi-node network (memory store) and confirm consensus produces blocks.
  * Optional: cross-validate block hashes with C# node for identical inputs.

---

## Phase 2 – Core Infrastructure (High-Level Summary)

1. **CMake / Build Graph**
   * Add consensus sources back to `neo_consensus` target.
   * Replace incomplete stubs with compilable scaffolding (no logic yet) so build succeeds.

2. **Data Structures**
   * Port `ConsensusContext` skeleton (members, getters/setters, basic invariants).
   * Implement full message classes with serialization semantics matching C#.
   * Introduce helper types (payload builder, header struct) as needed.

Deliverable: `neo_consensus` compiles, unit tests verify message serialization and context initialization.

## Phase 3 – State Machine & Workflow (High-Level)

* Implement block proposal flow (primary).
* Backup vote handling (prepare response signatures).
* Commit path (collect signatures, produce block, interact with blockchain).
* View-change logic (timeouts, ChangeView messages).
* Recovery protocol (recovery request/response for resynchronization).

Deliverable: deterministic tests covering nominal round, view change, recovery logic.

## Phase 4 – Networking & Integration

* Wire `LocalNode` to deliver consensus payloads and broadcast responses.
* Modify `NeoSystem` to instantiate `ConsensusService` based on configuration.
* Integrate key management (load key pair, handle wallet if applicable).
* Ensure thread lifecycle properly managed during start/stop.

Deliverable: simplified multi-node integration test verifying real networking path (e.g., 4 nodes using memory store).

## Phase 5 – Validation & Hardening

* Expand tests for edge cases (double proposals, duplicate messages, malicious responses).
* Add logging/metrics parity where useful.
* (Optional) Persist consensus cache to storage for faster restart recovery.
* Document configuration knobs and operational considerations.

Deliverable: feature-complete consensus engine consistent with C# behaviour.

---



## Current Manual Control Summary *(2025-02-18)*

| Command/API                        | Status                                   | Notes | Remaining Work |
|-----------------------------------|-------------------------------------------|-------|----------------|
| RPC `startconsensus`/`stop`/`restart` | ✅ Implemented                         | Uses `ConsensusService::StartManually()` / `Stop()` hooks shared with NeoNode, returns bool like C# RPC | ✅ Covered by integration tests |
| Console `consensus start/stop/restart` | ✅ Implemented                        | Delegates to same service lifecycle; respects auto-start flag | ✅ Operator usage documented + CLI help |
| Console `consensus status`            | ✅ Implemented                         | Reports running flag, block, view, phase, auto-start state | Enhance output once consensus exposes extra diagnostics |
| Console `consensus autostart on/off` | ✅ Implemented                        | Persists to `ConfigurationManager::GetConsensusConfig().auto_start` and updates live service | ✅ Tests exercise service/config sync |
| Auto-start flag propagation           | ✅ Implemented                        | `ConsensusService::SetAutoStartEnabled()` and `NeoNode` sync flag on start | Ensure DBFT plugin/other tooling keep configuration in sync |


## Manual Control (Current Implementation) *(2025-02-18 Update)*

- `startconsensus`, `stopconsensus`, and `restartconsensus` RPC endpoints now call the shared `ConsensusService::StartManually()` / `Stop()` helpers, matching the behaviour of the C# node.
- The console command `consensus start|stop|restart|status` wires into the same lifecycle. Status output includes running flag, current block, view number, consensus phase, and whether auto-start is enabled.
- `consensus autostart on|off` toggles `ConfigurationManager::GetConsensusConfig().auto_start` and updates the live service immediately.
- `ConsensusService` exposes `SetAutoStartEnabled()` / `IsAutoStartEnabled()` so tooling and tests can query or adjust the flag atomically.
### Operator Quick Reference

| Command | Description | Behaviour |
|---------|-------------|-----------|
| `consensus start` | Manually starts dBFT when auto-start is disabled. Requires networking (`LocalNode`) to be online. | Invokes `ConsensusService::StartManually()` and re-registers with the `LocalNode`. |
| `consensus stop` | Stops the running consensus engine. | Calls `ConsensusService::Stop()` and detaches from the `LocalNode`. |
| `consensus restart` | Convenience wrapper combining stop and manual start. | Ensures networking is running, then re-initialises the consensus threads. |
| `consensus status` | Displays current consensus health snapshot. | Mirrors the C# `consensus status` output: running flag, block, view, phase, vote counts, and auto-start state. |
| `consensus autostart on|off` | Persists the auto-start preference through `ConfigurationManager`. | Updates the live service immediately and the in-memory configuration used on restart. |

RPC equivalents are exposed as `startconsensus`, `stopconsensus`, and `restartconsensus`. Each call returns a simple boolean (`true` on success, `false` otherwise), matching the Neo C# RPC contract. The `ManualConsensusControl` integration test covers the RPC lifecycle and verifies that `ConfigurationManager::GetConsensusConfig().auto_start` remains in sync with `ConsensusService::SetAutoStartEnabled`.
## Next Actions

* Review and confirm the Phase 1 plan (this document).
* Once approved, start Phase 2 by implementing the message classes and consensus context scaffolding, accompanied by serialization unit tests.

## Current Unit-Test Coverage

A lightweight payload helper smoke test is available behind the `NEO_ENABLE_CONSENSUS_UNIT_TESTS`
CMake option. Enable it with:

```bash
cmake -S . -B build -DNEO_ENABLE_CONSENSUS_UNIT_TESTS=ON
cmake --build build --target test_consensus
```

The test focuses on `ConsensusPayloadHelper` round-trips. Legacy gMock-based suites have been
disabled for now because they depend on still-missing consensus components; re-enable them only
after the full dBFT workflow lands.

---

## Status Update – 2025-02-15

Recent build fixes restored `neo_consensus`, `neo_rpc`, and the lightweight consensus tests, but parity gaps
remain before the C++ node matches the C# implementation:

- **Witness verification helpers** – `ApplicationEngine` still contains placeholder implementations for
  `IsCommitteeHash`, `VerifyCommitteeConsensus`, `VerifyMultiSignatureHash`, and the committee script builders.
  Port the corresponding logic from the C# engine so committee signatures and multi-sig scripts are validated
  correctly.
- **RPC iterator/session plumbing** – `rpc_methods.cpp` continues to use a local stub session manager. Replace it
  with the shared `neo::rpc::RpcSessionManager` and re-enable iterator/session RPCs (`traverseiterator`,
  `terminatesession`, etc.) with deterministic tests.
- **Consensus service coverage** – `getconsensusstate` now reports the “service unavailable” path, but a running
  service cannot yet be exercised in isolation. Once the DBFT facade is operational, add a positive-path unit test
  that wires a mock consensus service into the RPC layer.
- **Storage backends** – LevelDB/RocksDB targets link, yet the file-backed fallback is still the default provider.
  Finish wiring the provider factory to the vendored engines and add persistence tests that execute block import /
  retrieval through each backend.

### Recommended Next Steps

1. Port the missing witness/committee helpers in `ApplicationEngine`, mirroring the behaviour of
   `Neo.SmartContract.ApplicationEngine` in the C# node.
2. Swap the stub session manager out for `neo::rpc::RpcSessionManager`, then reinstate iterator/session RPC
   handling and unit coverage.
3. Extend the consensus regression tests to cover the success path once a controllable consensus service can be
   instantiated by the DBFT plugin.
4. Enable LevelDB/RocksDB by default and add storage regression tests so all supported backends stay in sync with
   the reference node.

## 2025-02-16 – Mempool Event Wiring Status

- `ConsensusService` now subscribes to both `MemoryPoolEvents::SubscribeTransactionAdded` and
  `SubscribeTransactionRemoved`.  Added transactions are forwarded to `DbftConsensus::AddTransaction`, while
  removals invoke `DbftConsensus::RemoveCachedTransaction` to keep the consensus cache aligned with the live
  mempool.
- The existing `LocalNode::OnTransactionMessageReceived` path calls `MemoryPool::TryAdd`, which raises the same
  event; block persistence and eviction paths rely on `MemoryPool::Remove`, so no extra LocalNode hooks were needed.
- Regression coverage lives in `tests/unit/consensus/test_consensus_service_status.cpp` to ensure cached entries
  are cleared when removals fire.  Integration tests are still TODO once a full consensus harness is available.

Documenting these items keeps the port aligned with the multi-phase roadmap and clarifies which pieces should be
prioritised next.
