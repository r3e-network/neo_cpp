#include <iostream>
#include <neo/ledger/transaction_attribute.h>

using namespace neo::ledger;

int main() {
    TransactionAttribute::Usage usage = static_cast<TransactionAttribute::Usage>(0x20);
    
    std::cout << "Testing deserialization logic for usage 0x20...\n";
    
    // The deserialization code checks Script first
    if (usage == TransactionAttribute::Usage::Script) {
        std::cout << "Would read 20 bytes (Script)\n";
    }
    else if (usage == TransactionAttribute::Usage::NotValidBefore) {
        std::cout << "Would read 4 bytes (NotValidBefore)\n";
    }
    
    // But both are equal to 0x20!
    std::cout << "\nDirect comparisons:\n";
    std::cout << "(usage == Script): " << (usage == TransactionAttribute::Usage::Script) << "\n";
    std::cout << "(usage == NotValidBefore): " << (usage == TransactionAttribute::Usage::NotValidBefore) << "\n";
    
    // The issue might be in how the enum values are compared
    std::cout << "\nUnderlying values:\n";
    std::cout << "static_cast<uint8_t>(usage): 0x" << std::hex << static_cast<int>(static_cast<uint8_t>(usage)) << "\n";
    std::cout << "static_cast<uint8_t>(Script): 0x" << std::hex << static_cast<int>(static_cast<uint8_t>(TransactionAttribute::Usage::Script)) << "\n";
    std::cout << "static_cast<uint8_t>(NotValidBefore): 0x" << std::hex << static_cast<int>(static_cast<uint8_t>(TransactionAttribute::Usage::NotValidBefore)) << "\n";
    
    return 0;
}