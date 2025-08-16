# Neo C# to C++ Test Coverage Report

## Executive Summary

| Metric | C# Tests | C++ Tests | Coverage |
|--------|----------|-----------|----------|
| **Total Tests** | 1484 | 4648 | 46.8% |
| **Categories** | 11 | 12 | - |

## Category Coverage

| Category | C# Tests | C++ Tests | Coverage | Status |
|----------|----------|-----------|----------|--------|
| **Consensus** | 6 | 121 | 0.0% | ❌ |
| **Cryptography** | 211 | 442 | 37.9% | ❌ |
| **Extensions** | 0 | 54 | 100.0% | ✅ |
| **IO** | 261 | 1200 | 44.4% | ❌ |
| **Json** | 92 | 108 | 31.5% | ❌ |
| **Ledger** | 94 | 211 | 52.1% | ❌ |
| **Network** | 104 | 529 | 57.7% | ❌ |
| **Other** | 164 | 429 | 47.6% | ❌ |
| **Persistence** | 45 | 150 | 31.1% | ❌ |
| **RPC** | 63 | 140 | 17.5% | ❌ |
| **SmartContract** | 333 | 1078 | 59.2% | ❌ |
| **Wallet** | 111 | 186 | 54.1% | ❌ |

## Missing Test Conversions

### Consensus (6 missing)

- `TestConsensusServiceCreation`
- `TestConsensusServiceStart`
- `TestConsensusServiceReceivesBlockchainMessages`
- `TestConsensusServiceHandlesExtensiblePayload`
- `TestConsensusServiceHandlesValidConsensusMessage`
- `TestConsensusServiceRejectsInvalidPayload`

### Cryptography (131 missing)

- `TestToBytes`
- `TestFromBytes`
- `TestFromBytesWideR2`
- `TestFromBytesWideNegativeOne`
- `TestFromBytesWideMaximum`
- `TestSquaring`
- `TestInversion`
- `TestInvertIsPow`
- `TestSqrt`
- `TestFromRaw`
- ... and 121 more

### IO (145 missing)

- `Test_StackItemState`
- `Test_ExecutionState`
- `TestGetVarSizeInt`
- `TestGetVarSizeGeneric`
- `TestToTimestampMS`
- `TestGetVarSizeString`
- `TestGetVarSizeInt`
- `TestGetStrictUTF8String`
- `TestTrimStartIgnoreCase`
- `TestGetVarSizeGeneric`
- ... and 135 more

### Json (63 missing)

- `TestSetItem`
- `TestCopyTo`
- `TestInsert`
- `TestIndexOf`
- `TestIsReadOnly`
- `TestGetEnumerator`
- `TestInvalidIndexAccess`
- `TestEmptyEnumeration`
- `TestImplicitConversionFromJTokenArray`
- `TestAddNullValues`
- ... and 53 more

### Ledger (45 missing)

- `TestGetBlock_Genesis`
- `TestGetBlock_NoTransactions`
- `TestGetBlockCount`
- `TestGetBlockHeaderCount`
- `TestGetBlockHeader`
- `TestGetContractState`
- `TestGetContractState_Native_CaseInsensitive`
- `TestGetContractState_InvalidFormat`
- `TestGetRawMemPool`
- `TestGetRawMemPool_Empty`
- ... and 35 more

### Network (44 missing)

- `GetTimeOut`
- `NoService`
- `TestDefaults`
- `RemoteNode_Test_Abort_DifferentNetwork`
- `RemoteNode_Test_Accept_IfSameNetwork`
- `TaskManager_Test_IsHighPriority`
- `RemoteNode_Test_IsHighPriority`
- `Size_Get`
- `Size_Get`
- `Size_Get`
- ... and 34 more

### Other (86 missing)

- `TestGetAccountStatus`
- `TestGetVsockAddress`
- `TestInvalidEndpoint`
- `TestMinimumValidatorConsensus`
- `TestMaximumByzantineFailures`
- `TestStressConsensusMultipleRounds`
- `TestLargeTransactionSetConsensus`
- `TestConcurrentViewChanges`
- `TestPrimaryFailureDuringConsensus`
- `TestByzantineValidatorSendsConflictingMessages`
- ... and 76 more

### Persistence (31 missing)

- `TestLevelDbDatabase`
- `TestLevelDb`
- `TestRocksDb`
- `TestAddInternal`
- `TestDeleteInternal`
- `TestFindInternal`
- `TestGetInternal`
- `TestTryGetInternal`
- `TestUpdateInternal`
- `TestCacheOverrideIssue2572`
- ... and 21 more

### RPC (52 missing)

- `TestCheckAuth_ValidCredentials_ReturnsTrue`
- `TestCheckAuth`
- `TestDuplicateTransactionErrorCode`
- `TestTargetInvocationExceptionUnwrapping`
- `TestDynamicInvokeDelegateExceptionUnwrapping`
- `TestAggregateExceptionUnwrapping`
- `TestUInt160`
- `TestUInt256`
- `TestInteger`
- `TestBoolean`
- ... and 42 more

### SmartContract (136 missing)

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
- ... and 126 more

### Wallet (51 missing)

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
- ... and 41 more


## Recommendations

### Priority Actions
1. **Current Coverage**: 46.8%
2. **Target Coverage**: 90%+
3. **Tests to Convert**: 790

### Priority Categories for Conversion

- **Consensus**: 0.0% coverage (6 tests missing)
- **RPC**: 17.5% coverage (52 tests missing)
- **Persistence**: 31.1% coverage (31 tests missing)
- **Json**: 31.5% coverage (63 tests missing)
- **Cryptography**: 37.9% coverage (131 tests missing)
- **IO**: 44.4% coverage (145 tests missing)
- **Other**: 47.6% coverage (86 tests missing)
- **Ledger**: 52.1% coverage (45 tests missing)
- **Wallet**: 54.1% coverage (51 tests missing)
- **Network**: 57.7% coverage (44 tests missing)
- **SmartContract**: 59.2% coverage (136 tests missing)
