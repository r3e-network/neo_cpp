# Plan to Fix Simplified/Mock Implementations

## Critical Priority (Must Fix Immediately)

### 1. Cryptographic Functions
**File**: `src/cryptography/ecc/secp256r1.cpp`
- **Issue**: Using std::hash instead of real ECC operations
- **Solution**: Replace with OpenSSL-based implementation (see secp256r1_proper.cpp)
- **Impact**: Security critical - affects all signature verification

### 2. Key Pair Operations  
**File**: `src/wallets/key_pair.cpp`
- **Issue**: Simplified ECDSA signatures and key generation
- **Solution**: Use proper cryptographic libraries
- **Impact**: Wallet security compromised

## High Priority

### 3. BLS12-381 Operations
**File**: `src/cryptography/bls12_381.cpp`
- **Issue**: Simplified field operations and pairing
- **Solution**: Integrate proper BLS12-381 library (e.g., MCL or BLST)
- **Impact**: Smart contract cryptographic operations incorrect

### 4. MPTTrie Implementation
**File**: `src/cryptography/mpttrie/trie.cpp`
- **Issue**: Marked as "mock implementation"
- **Solution**: Already appears complete - just remove misleading comments
- **Impact**: State storage integrity

### 5. Transaction Verification
**File**: `src/smartcontract/transaction_verifier.cpp`
- **Issue**: Simplified signature and witness verification
- **Solution**: Implement proper cryptographic verification
- **Impact**: Transaction security

## Medium Priority

### 6. Storage Keys
**File**: `src/persistence/storage_key.cpp`
- **Issue**: Simplified UInt160 to contract ID conversion
- **Solution**: Implement proper contract ID mapping
- **Impact**: Storage inconsistency

### 7. IPv6 Support
**File**: `src/extensions/ipaddress_extensions.cpp`
- **Issue**: Simplified IPv6 parsing
- **Solution**: Use proper IPv6 parsing libraries
- **Impact**: Network connectivity limitations

### 8. Base58 Encoding
**File**: `src/wallets/key_pair.cpp`
- **Issue**: Simplified Base58 implementation
- **Solution**: Use proper Base58 library
- **Impact**: Address encoding/decoding errors

## Implementation Strategy

### Phase 1: Critical Security Fixes (Week 1)
1. Replace secp256r1.cpp with proper implementation
2. Fix key_pair.cpp cryptographic operations
3. Update transaction verification

### Phase 2: Core Functionality (Week 2)
1. Integrate proper BLS12-381 library
2. Fix storage key conversions
3. Remove misleading MPTTrie comments

### Phase 3: Supporting Features (Week 3)
1. Implement full IPv6 support
2. Replace all Base58 operations
3. Fix remaining simplified implementations

## Testing Requirements

### For Each Fix:
1. Unit tests comparing output with C# implementation
2. Integration tests with full node operation
3. Performance benchmarks
4. Security audit for cryptographic changes

## Dependencies

### Required Libraries:
- OpenSSL 3.0+ (for ECC operations)
- MCL or BLST (for BLS12-381)
- Proper Base58 library
- IPv6 parsing library

### Build System Updates:
- Update CMakeLists.txt for new dependencies
- Add feature detection for crypto libraries
- Update CI/CD for new requirements

## Risk Mitigation

1. **Backward Compatibility**: Ensure all changes maintain protocol compatibility
2. **Performance**: Benchmark against C# implementation
3. **Security**: Third-party audit for cryptographic implementations
4. **Testing**: Comprehensive test suite before deployment

## Success Criteria

- All "simplified" and "mock" comments removed
- All cryptographic operations use proper libraries
- Test coverage > 95% for affected modules
- Performance within 10% of C# implementation
- Security audit passed