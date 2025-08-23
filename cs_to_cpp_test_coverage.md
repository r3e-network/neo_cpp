# Neo C# to C++ Test Coverage Report

## Executive Summary

| Metric | C# Tests | C++ Tests | Coverage |
|--------|----------|-----------|----------|
| **Total Tests** | 1515 | 5291 | 48.4% |
| **Categories** | 11 | 12 | - |

## Category Coverage

| Category | C# Tests | C++ Tests | Coverage | Status |
|----------|----------|-----------|----------|--------|
| **Consensus** | 6 | 169 | 100.0% | ✅ |
| **Cryptography** | 211 | 495 | 37.9% | ❌ |
| **Extensions** | 0 | 54 | 100.0% | ✅ |
| **IO** | 261 | 1291 | 46.4% | ❌ |
| **Json** | 92 | 113 | 31.5% | ❌ |
| **Ledger** | 94 | 237 | 60.6% | ❌ |
| **Network** | 104 | 629 | 57.7% | ❌ |
| **Other** | 200 | 497 | 51.5% | ❌ |
| **Persistence** | 45 | 154 | 33.3% | ❌ |
| **RPC** | 67 | 183 | 16.4% | ❌ |
| **SmartContract** | 324 | 1229 | 58.6% | ❌ |
| **Wallet** | 111 | 240 | 55.0% | ❌ |

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

### IO (140 missing)

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
- ... and 130 more

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

### Network (44 missing)

- `GetTimeOut`
- `NoService`
- `RemoteNode_Test_Abort_DifferentNetwork`
- `RemoteNode_Test_Accept_IfSameNetwork`
- `TestDefaults`
- `TaskManager_Test_IsHighPriority`
- `RemoteNode_Test_IsHighPriority`
- `Size_Get`
- `Size_Get`
- `Size_Get`
- ... and 34 more

### Other (97 missing)

- `TestFilter`
- `TestCachedFind_Between`
- `TestCachedFind_Last`
- `TestCachedFind_Empty`
- `TestBigDecimalConstructor`
- `TestGetDecimals`
- `TestGernerator1`
- `TestGernerator2`
- `TestGernerator3`
- `TestOperatorEqual`
- ... and 87 more

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

### RPC (56 missing)

- `TestRpcStack`
- `TestGetKeyPair`
- `TestTransactionAttribute`
- `TestWitnessRule`
- `TestToBigInteger`
- `TestConstructorByUrlAndDispose`
- `TestConstructorWithBasicAuth`
- `TestGetPeers_NoUnconnected`
- `TestGetPeers_NoConnected`
- `TestGetVersion_HardforksStructure`
- ... and 46 more

### SmartContract (134 missing)

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
- ... and 124 more

### Wallet (50 missing)

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
- ... and 40 more


## Recommendations

### Priority Actions
1. **Current Coverage**: 48.4%
2. **Target Coverage**: 90%+
3. **Tests to Convert**: 782

### Priority Categories for Conversion

- **RPC**: 16.4% coverage (56 tests missing)
- **Json**: 31.5% coverage (63 tests missing)
- **Persistence**: 33.3% coverage (30 tests missing)
- **Cryptography**: 37.9% coverage (131 tests missing)
- **IO**: 46.4% coverage (140 tests missing)
- **Other**: 51.5% coverage (97 tests missing)
- **Wallet**: 55.0% coverage (50 tests missing)
- **Network**: 57.7% coverage (44 tests missing)
- **SmartContract**: 58.6% coverage (134 tests missing)
- **Ledger**: 60.6% coverage (37 tests missing)
