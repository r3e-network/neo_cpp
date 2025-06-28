#include <iostream>
#include <neo/ledger/transaction_attribute.h>

using namespace neo::ledger;

int main() {
    // Test enum comparison behavior when two enum values are the same
    TransactionAttribute::Usage usage = static_cast<TransactionAttribute::Usage>(0x20);
    
    std::cout << "Testing enum comparison with duplicate values...\n";
    std::cout << "usage value: 0x" << std::hex << static_cast<int>(usage) << "\n\n";
    
    // Test the exact order from the deserialization code
    if (usage == TransactionAttribute::Usage::HighPriority) {
        std::cout << "Matched: HighPriority\n";
    }
    else if (usage == TransactionAttribute::Usage::OracleResponse) {
        std::cout << "Matched: OracleResponse\n";
    }
    else if (usage == TransactionAttribute::Usage::Script) {
        std::cout << "Matched: Script (should be this one!)\n";
    }
    else if (usage == TransactionAttribute::Usage::NotValidBefore) {
        std::cout << "Matched: NotValidBefore (shouldn't reach here!)\n";
    }
    else {
        std::cout << "No match\n";
    }
    
    // Direct comparison tests
    std::cout << "\nDirect comparisons:\n";
    std::cout << "usage == Script: " << (usage == TransactionAttribute::Usage::Script) << "\n";
    std::cout << "usage == NotValidBefore: " << (usage == TransactionAttribute::Usage::NotValidBefore) << "\n";
    
    // Check if compiler sees them as the same
    std::cout << "\nCompiler view:\n";
    std::cout << "Script == NotValidBefore: " << (TransactionAttribute::Usage::Script == TransactionAttribute::Usage::NotValidBefore) << "\n";
    
    return 0;
}