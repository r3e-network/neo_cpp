# Neo C# to C++ Test Coverage Report

## Executive Summary

| Metric | C# Tests | C++ Tests | Coverage |
|--------|----------|-----------|----------|
| **Total Tests** | 1594 | 5413 | 48.6% |
| **Categories** | 11 | 12 | - |

## Category Coverage

| Category | C# Tests | C++ Tests | Coverage | Status |
|----------|----------|-----------|----------|--------|
| **Consensus** | 6 | 179 | 100.0% | ✅ |
| **Cryptography** | 211 | 494 | 37.9% | ❌ |
| **Extensions** | 0 | 54 | 100.0% | ✅ |
| **IO** | 288 | 1309 | 42.7% | ❌ |
| **Json** | 92 | 134 | 41.3% | ❌ |
| **Ledger** | 94 | 239 | 60.6% | ❌ |
| **Network** | 105 | 633 | 57.1% | ❌ |
| **Other** | 206 | 497 | 48.5% | ❌ |
| **Persistence** | 45 | 154 | 33.3% | ❌ |
| **RPC** | 68 | 250 | 36.8% | ❌ |
| **SmartContract** | 328 | 1229 | 58.2% | ❌ |
| **Wallet** | 151 | 241 | 53.0% | ❌ |

**Recent progress (2025-11-05):** Wrapped the ApplicationLogs plugin parity work by wiring `getapplicationlog` through the production plugin stack, added configurable caching via `MaxCachedLogs`, and ported the RPC `getrawmempool` variants to the C++ suite. JSON helpers (`JArray` and friends) now mirror the C# semantics for null-handling and copy/insert operations, and the RPC client utilities (`GetKeyPair`, `ToBigInteger`, `RpcStack`) have smoke coverage to guard against regressions. Added `sendrawtransaction` happy-path and failure semantics (invalid format, signature, script, policy, duplication) plus the `submitblock` duplicate/queued future-block validation coverage to match the C# `RpcServer.Node` tests. The blockchain replay loop now uses cached headers so staged blocks persist automatically once missing parents arrive, mirroring the C# header cache behaviour.

## Missing Test Conversions

### Cryptography (131 missing)

- `TestAddCheck`
- `TestBloomFIlterConstructorGetKMTweak`
- `TestGetBits`
- `TestInvalidArguments`
- `DeriveKeyTest`
- `TestTrim`
- `TestBase58CheckDecode`
- `TestMurmurReadOnlySpan`
- `TestSha512`
- `TestTest`
- ... and 121 more

### IO (165 missing)

- `Test_StackItemState`
- `Test_ExecutionState`
- `TestIsReadOnly`
- `TestSetAndGetItem`
- `TestGetKeys`
- `TestGetValues`
- `TestTryGetValue`
- `TestCollectionAddAndContains`
- `TestCollectionCopyTo`
- `TestCollectionRemove`
- ... and 155 more

### Json (54 missing)

- `TestGetEnumerator`
- `TestInvalidIndexAccess`
- `TestImplicitConversionFromJTokenArray`
- `TestReadOnlyBehavior`
- `TestSetNull`
- `TestInsertNull`
- `TestRemoveNull`
- `TestContainsNull`
- `TestCopyToWithNull`
- `TestFromStringWithNull`
- ... and 44 more

### Ledger (37 missing)

- `TestGetContractState_Native_CaseInsensitive`
- `TestGetContractState_InvalidFormat`
- `TestGetRawMemPool_Empty`
- `TestGetRawMemPool_MixedVerifiedUnverified`
- `TestGetRawTransaction`
- `TestGetRawTransaction_Confirmed`
- `TestGetStorage`
- `TestFindStorage`
- `TestStorage_NativeContractName`
- `TestFindStorage_Pagination`
- ... and 27 more

### Network (45 missing)

- `GetTimeOut`
- `NoService`
- `RemoteNode_Test_Abort_DifferentNetwork`
- `RemoteNode_Test_Accept_IfSameNetwork`
- `TestDefaults`
- `ProcessesTcpConnectedAfterConfigArrives`
- `TaskManager_Test_IsHighPriority`
- `RemoteNode_Test_IsHighPriority`
- `Size_Get`
- `Size_Get`
- ... and 35 more

### Other (106 missing)

- `TestFilter`
- `TestGetStateHeight_Basic`
- `TestGetStateRoot_WithInvalidIndex_ShouldThrowRpcException`
- `TestGetProof_WithInvalidKey_ShouldThrowRpcException`
- `TestGetStateRoot_WithMockData_ShouldReturnStateRoot`
- `TestGetProof_WithMockData_ShouldReturnProof`
- `TestGetState_WithMockData_ShouldReturnValue`
- `TestFindStates_WithMockData_ShouldReturnResults`
- `TestCachedFind_Between`
- `TestCachedFind_Last`
- ... and 96 more

### Persistence (30 missing)

- `TestLevelDbDatabase`
- `TestLevelDb`
- `TestRocksDb`
- `TestImplicit`
- `TestGeneratorAndDispose`
- `TestKeyAndValueAndNext`
- `TestReadOnlyStoreView`
- `TestAccessByKey`
- `TestAccessByNotFoundKey`
- `TestAccessByDeletedKey`
- ... and 20 more

### RPC (43 missing)

- `TestTransactionAttribute`
- `TestWitnessRule`
- `TestConstructorByUrlAndDispose`
- `TestSendRawTransaction_Normal`
- `TestSendRawTransaction_InvalidTransactionFormat`
- `TestSendRawTransaction_InsufficientBalance`
- `TestSendRawTransaction_InvalidScript`
- `TestSendRawTransaction_InvalidAttribute`
- `TestSendRawTransaction_Oversized`
- `TestSendRawTransaction_Expired`
- ... and 33 more

### SmartContract (137 missing)

- `TestMinIntegerAbs`
- `TestInvokeFunction`
- `TestInvokeFunctionInvalid`
- `TestInvokeScript`
- `TestInvokeFunction_FaultState`
- `TestInvokeScript_FaultState`
- `TestInvokeScript_GasLimitExceeded`
- `TestInvokeFunction_InvalidWitnessInvocation`
- `TestInvokeFunction_InvalidWitnessVerification`
- `TestInvokeFunction_InvalidContractParameter`
- ... and 127 more

### Wallet (71 missing)

- `TestOpenInvalidWallet`
- `TestDumpPrivKey`
- `TestDumpPrivKey_AddressNotInWallet`
- `TestDumpPrivKey_InvalidAddressFormat`
- `TestGetNewAddress`
- `TestGetWalletBalance`
- `TestGetWalletBalanceInvalidAsset`
- `TestGetWalletBalance_InvalidAssetIdFormat`
- `TestGetWalletUnclaimedGas`
- `TestImportPrivKey`
- ... and 61 more


## Recommendations

### Priority Actions
1. **Current Coverage**: 48.6%
2. **Target Coverage**: 90%+
3. **Tests to Convert**: 819

### Priority Categories for Conversion

- **Persistence**: 33.3% coverage (30 tests missing)
- **RPC**: 36.8% coverage (43 tests missing)
- **Cryptography**: 37.9% coverage (131 tests missing)
- **Json**: 41.3% coverage (54 tests missing)
- **IO**: 42.7% coverage (165 tests missing)
- **Other**: 48.5% coverage (106 tests missing)
- **Wallet**: 53.0% coverage (71 tests missing)
- **Network**: 57.1% coverage (45 tests missing)
- **SmartContract**: 58.2% coverage (137 tests missing)
- **Ledger**: 60.6% coverage (37 tests missing)
