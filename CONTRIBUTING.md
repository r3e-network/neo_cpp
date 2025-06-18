# Contributing to Neo C++

Thank you for your interest in contributing to the Neo C++ blockchain node implementation! This document provides guidelines and information for contributors.

## 🌟 How to Contribute

We welcome contributions of all kinds:

- **Bug Reports**: Help us identify and fix issues
- **Feature Requests**: Suggest new functionality
- **Code Contributions**: Implement features, fix bugs, improve performance
- **Documentation**: Improve guides, API docs, and examples
- **Testing**: Add test cases, improve test coverage
- **Performance**: Optimize existing code and algorithms

## 🚀 Getting Started

### Prerequisites

Before contributing, ensure you have:

- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- CMake 3.20+
- Git
- vcpkg (for dependency management)
- Understanding of Neo blockchain concepts

### Setting Up Development Environment

1. **Fork the Repository**
   ```bash
   # Fork the repo on GitHub, then clone your fork
   git clone https://github.com/your-username/neo-cpp.git
   cd neo-cpp
   ```

2. **Set Up Upstream Remote**
   ```bash
   git remote add upstream https://github.com/neo-project/neo-cpp.git
   ```

3. **Initialize Dependencies**
   ```bash
   # Initialize vcpkg
   git submodule update --init --recursive
   ./vcpkg/bootstrap-vcpkg.sh  # or .bat on Windows
   ```

4. **Build the Project**
   ```bash
   mkdir build && cd build
   cmake .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
   cmake --build . --config Debug
   ```

5. **Run Tests**
   ```bash
   ctest --output-on-failure
   ```

## 🔧 Development Workflow

### 1. Create a Feature Branch

```bash
# Update your main branch
git checkout main
git pull upstream main

# Create a new feature branch
git checkout -b feature/your-feature-name
```

### 2. Make Changes

- Write clean, well-documented code
- Follow the project's coding standards
- Add tests for new functionality
- Update documentation as needed

### 3. Test Your Changes

```bash
# Run all tests
ctest

# Run specific test categories
ctest -L unit
ctest -L integration

# Check for memory leaks (Linux)
valgrind --leak-check=full ./your-test

# Static analysis
make lint
```

### 4. Commit Your Changes

```bash
# Stage your changes
git add .

# Commit with a descriptive message
git commit -m "feat: add new consensus mechanism feature

- Implement new dBFT optimization
- Add comprehensive unit tests
- Update documentation
- Improve performance by 15%"
```

### 5. Push and Create Pull Request

```bash
# Push to your fork
git push origin feature/your-feature-name

# Create a pull request on GitHub
```

## 📝 Coding Standards

### C++ Style Guidelines

- **Standard**: Follow C++20 best practices
- **Naming Conventions**:
  - Classes: `PascalCase` (e.g., `BlockChain`, `Transaction`)
  - Functions/Methods: `PascalCase` (e.g., `ProcessBlock()`, `ValidateTransaction()`)
  - Variables: `camelCase` (e.g., `blockHeight`, `transactionHash`)
  - Member variables: `camelCase_` (e.g., `blockHeight_`, `isRunning_`)
  - Constants: `UPPER_SNAKE_CASE` (e.g., `MAX_BLOCK_SIZE`)
- **Headers**: Use `#pragma once` instead of include guards
- **Memory Management**: Use smart pointers (`std::shared_ptr`, `std::unique_ptr`)
- **Error Handling**: Use exceptions for error conditions
- **Threading**: Use standard library threading primitives

### Code Organization

```cpp
// Example header file structure
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace neo::ledger {

/**
 * @brief Represents a blockchain transaction
 * 
 * This class encapsulates all the data and functionality
 * required for Neo N3 transactions.
 */
class Transaction {
public:
    Transaction() = default;
    ~Transaction() = default;
    
    // Copy and move semantics
    Transaction(const Transaction&) = default;
    Transaction& operator=(const Transaction&) = default;
    Transaction(Transaction&&) = default;
    Transaction& operator=(Transaction&&) = default;
    
    /**
     * @brief Validates the transaction
     * @return true if valid, false otherwise
     */
    bool Validate() const;
    
private:
    std::string hash_;
    std::vector<uint8_t> script_;
    uint64_t networkFee_;
    uint64_t systemFee_;
};

} // namespace neo::ledger
```

### Documentation Standards

- Use Doxygen-style comments for all public APIs
- Include parameter descriptions and return value information
- Provide usage examples for complex functions
- Document thread safety guarantees
- Explain algorithmic complexity where relevant

## Commit Message Convention

We follow the [Conventional Commits](https://www.conventionalcommits.org/) specification for commit messages:

```
<type>(<scope>): <description>

[optional body]

[optional footer]
```

Where `type` is one of:
- `feat`: A new feature
- `fix`: A bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, etc.)
- `refactor`: Code refactoring
- `test`: Adding or modifying tests
- `chore`: Changes to the build process or tools

## Branch Naming Convention

- `feature/feature-name`: For new features
- `bugfix/bug-name`: For bug fixes
- `refactor/refactor-name`: For code refactoring
- `docs/docs-name`: For documentation changes
- `test/test-name`: For test changes

## Testing

We use Google Test for unit testing. All new code should have corresponding unit tests. Significant changes should also include integration tests.

To run the tests:

```bash
cd build
cmake --build . --target test
```

## Documentation

We use Doxygen for API documentation. All public APIs should be documented using Doxygen-style comments.

To generate the documentation:

```bash
cd build
cmake --build . --target docs
```

## Reporting Bugs

We use GitHub issues to track public bugs. Report a bug by opening a new issue; it's that easy!

### Bug Report Template

```
## Bug Description

A clear and concise description of what the bug is.

## Steps to Reproduce

1. Go to '...'
2. Click on '....'
3. Scroll down to '....'
4. See error

## Expected Behavior

A clear and concise description of what you expected to happen.

## Actual Behavior

A clear and concise description of what actually happened.

## Screenshots

If applicable, add screenshots to help explain your problem.

## Environment

- OS: [e.g. Ubuntu 20.04]
- Compiler: [e.g. GCC 9.3.0]
- Version: [e.g. 0.1.0]

## Additional Context

Add any other context about the problem here.
```

## Feature Requests

We use GitHub issues to track feature requests. Request a feature by opening a new issue with the "feature request" template.

### Feature Request Template

```
## Feature Description

A clear and concise description of the feature you're requesting.

## Problem Statement

A clear and concise description of what the problem is. Ex. I'm always frustrated when [...]

## Proposed Solution

A clear and concise description of what you want to happen.

## Alternative Solutions

A clear and concise description of any alternative solutions or features you've considered.

## Additional Context

Add any other context or screenshots about the feature request here.
```

## License

By contributing, you agree that your contributions will be licensed under the project's MIT License.

## References

This document was adapted from the open-source contribution guidelines for [Facebook's Draft](https://github.com/facebook/draft-js/blob/master/CONTRIBUTING.md).
