# Neo C++ Examples

This directory contains example code demonstrating how to use the Neo C++ blockchain node implementation.

## Available Examples

### Core Examples

- **consensus_example.cpp** - Demonstrates consensus mechanism usage
- **ledger_example.cpp** - Shows blockchain ledger operations
- **network_example.cpp** - Network communication examples
- **smartcontract_example.cpp** - Smart contract execution examples
- **vm_example.cpp** - Virtual machine usage examples

## Building Examples

Examples are built as part of the main project:

```bash
mkdir build && cd build
cmake ..
make

# Run an example
./examples/ledger_example
```

## Usage

Each example file contains:
- Clear documentation of what it demonstrates
- Step-by-step code with comments
- Expected output and results

## Contributing

When adding new examples:
1. Keep them focused on one concept
2. Include comprehensive comments
3. Follow the project coding standards
4. Test thoroughly before submitting