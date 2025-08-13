# Neo C++ Code Documentation Guidelines

## Overview
This document establishes the commenting and documentation standards for the Neo C++ project. All code must be thoroughly documented to ensure maintainability, clarity, and ease of understanding for current and future developers.

## Documentation Standards

### 1. File Headers
Every source file (.h, .cpp) must begin with:
```cpp
/**
 * @file filename.h
 * @brief Brief description of the file's purpose
 * @details Extended description if needed
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */
```

### 2. Class Documentation
```cpp
/**
 * @class ClassName
 * @brief Brief description of the class purpose
 * @details Detailed explanation of the class functionality,
 *          its role in the system, and usage patterns
 * 
 * @tparam T Template parameter description (if applicable)
 * 
 * Example usage:
 * @code
 * ClassName obj;
 * obj.method();
 * @endcode
 */
class ClassName {
```

### 3. Function/Method Documentation
```cpp
/**
 * @brief Brief description of what the function does
 * @details Extended description if the function is complex
 * 
 * @param paramName Description of the parameter
 * @param[in] inputParam Input parameter description  
 * @param[out] outputParam Output parameter description
 * @param[in,out] inOutParam Input/output parameter description
 * 
 * @return Description of the return value
 * 
 * @throws ExceptionType Description of when this exception is thrown
 * 
 * @pre Preconditions that must be true
 * @post Postconditions that will be true
 * 
 * @note Important notes about the function
 * @warning Warnings about function usage
 * 
 * @complexity O(n) where n is...
 */
ReturnType functionName(ParamType paramName);
```

### 4. Member Variable Documentation
```cpp
class Example {
private:
    /// @brief Brief description of the member variable
    int member_var_;
    
    /**
     * @brief Complex member requiring detailed documentation
     * @details Extended explanation of the member's purpose,
     *          valid ranges, relationships with other members
     */
    ComplexType complex_member_;
};
```

### 5. Enum Documentation
```cpp
/**
 * @enum EnumName
 * @brief Brief description of the enum purpose
 */
enum class EnumName {
    /// @brief Description of first value
    FIRST_VALUE = 0,
    
    /// @brief Description of second value  
    SECOND_VALUE = 1
};
```

### 6. Namespace Documentation
```cpp
/**
 * @namespace neo::ledger
 * @brief Contains blockchain ledger management components
 * @details This namespace includes all classes and functions
 *          related to blockchain state management, block processing,
 *          and transaction handling
 */
namespace neo::ledger {
```

## Implementation File Comments

### 1. Complex Algorithm Documentation
```cpp
// Algorithm: QuickSort with 3-way partitioning
// Time Complexity: O(n log n) average, O(n²) worst case
// Space Complexity: O(log n) for recursion stack
// 
// The algorithm partitions the array into three sections:
// 1. Elements less than pivot
// 2. Elements equal to pivot  
// 3. Elements greater than pivot
//
// This optimization handles duplicate elements efficiently
```

### 2. Implementation Notes
```cpp
// NOTE: Using RocksDB for persistence layer
// - Provides ACID guarantees
// - Supports atomic batch operations
// - Handles concurrent reads efficiently
```

### 3. TODO Comments (only with implementation plan)
```cpp
// TODO(#issue-123): Optimize memory allocation
// Current implementation allocates on each call.
// Plan: Implement object pool with pre-allocated buffers
// Target: v2.1.0
```

### 4. Bug Workarounds
```cpp
// WORKAROUND: OpenSSL 3.0 compatibility issue
// OpenSSL 3.0 changed the EVP API. This code maintains
// compatibility with both 1.1.x and 3.0.x versions.
// Can be removed when minimum version is 3.0+
```

## Special Documentation Tags

### Critical Sections
```cpp
/**
 * @critical
 * @brief This section handles consensus voting
 * @details Must be atomic to prevent double-voting attacks.
 *          Protected by consensus_mutex_
 */
```

### Performance Critical
```cpp
/**
 * @performance
 * @brief Hot path - called for every transaction
 * @details Optimized for minimal allocations and cache locality.
 *          Benchmarked at 50k TPS on reference hardware
 */
```

### Security Sensitive
```cpp
/**
 * @security
 * @brief Validates cryptographic signatures
 * @details Uses constant-time comparison to prevent timing attacks.
 *          All inputs are validated against buffer overflow
 */
```

### Thread Safety
```cpp
/**
 * @thread_safety Thread-safe
 * @brief Can be called concurrently from multiple threads
 * @details Protected by internal mutex. Lock ordering: 
 *          1. blockchain_mutex_
 *          2. mempool_mutex_
 *          3. network_mutex_
 */
```

## Documentation Quality Checklist

### Mandatory Documentation
- [ ] All public APIs have complete Doxygen comments
- [ ] All classes have @brief and @details descriptions
- [ ] All functions have parameter and return documentation
- [ ] All exceptions are documented with @throws
- [ ] Thread safety is documented for all public methods
- [ ] Complex algorithms have complexity analysis

### Best Practices
- [ ] Use complete sentences with proper grammar
- [ ] Include examples for non-trivial usage
- [ ] Document assumptions and invariants
- [ ] Explain "why" not just "what"
- [ ] Keep comments synchronized with code changes
- [ ] Use consistent terminology throughout

### Anti-Patterns to Avoid
- ❌ Redundant comments: `i++; // increment i`
- ❌ Outdated comments that don't match code
- ❌ Commenting out code (use version control instead)
- ❌ TODO without actionable plan or tracking
- ❌ Vague descriptions: "handles stuff"
- ❌ Missing error condition documentation

## Doxygen Configuration

The project uses Doxygen for API documentation generation:

```bash
# Generate documentation
doxygen docs/Doxyfile

# View generated docs
open docs/html/index.html
```

Key Doxygen settings:
- `EXTRACT_ALL = YES` - Document all entities
- `GENERATE_TREEVIEW = YES` - Navigation sidebar
- `HAVE_DOT = YES` - Generate UML diagrams
- `CALL_GRAPH = YES` - Function call graphs
- `SOURCE_BROWSER = YES` - Link to source code

## Examples

### Well-Documented Header File
```cpp
/**
 * @file blockchain.h
 * @brief Core blockchain implementation
 * @author Neo C++ Team
 * @date 2025
 */

#pragma once

namespace neo::ledger {

/**
 * @class Blockchain
 * @brief Manages the blockchain state and block processing
 * @details The Blockchain class is the central component that:
 *          - Validates and persists blocks
 *          - Manages the mempool
 *          - Handles transaction verification
 *          - Emits events for block and transaction processing
 * 
 * @thread_safety Thread-safe for all public methods
 * @performance Block processing optimized for parallel validation
 */
class Blockchain {
public:
    /**
     * @brief Adds a new block to the blockchain
     * @param block The block to add
     * @return true if block was successfully added
     * @throws InvalidBlockException if block validation fails
     * @complexity O(n) where n is number of transactions
     */
    bool AddBlock(std::shared_ptr<Block> block);
    
private:
    /// @brief Current blockchain height
    std::atomic<uint32_t> height_;
    
    /// @brief Thread-safe memory pool for pending transactions
    std::unique_ptr<MemoryPool> mempool_;
};

} // namespace neo::ledger
```

## Enforcement

1. **Code Review**: All PRs must have proper documentation
2. **CI Checks**: Automated Doxygen warnings as errors
3. **Coverage Report**: Documentation coverage metrics
4. **Style Guide**: Enforced by clang-format and clang-tidy

## Tools

### Documentation Generation
```bash
make docs              # Generate Doxygen documentation
make docs-check        # Check for documentation warnings
```

### Documentation Linting
```bash
./scripts/check_documentation.sh   # Verify documentation completeness
```

## References

- [Doxygen Manual](https://www.doxygen.nl/manual/)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html#Comments)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)