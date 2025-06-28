#include <iostream>
#include <neo/ledger/transaction_attribute.h>

using namespace neo::ledger;

int main() {
    std::cout << "TransactionAttribute Usage values:\n";
    std::cout << "Script = 0x" << std::hex << static_cast<int>(TransactionAttribute::Usage::Script) << "\n";
    std::cout << "NotValidBefore = 0x" << std::hex << static_cast<int>(TransactionAttribute::Usage::NotValidBefore) << "\n";
    
    // Check if they're equal
    if (static_cast<uint8_t>(TransactionAttribute::Usage::Script) == 
        static_cast<uint8_t>(TransactionAttribute::Usage::NotValidBefore)) {
        std::cout << "ERROR: Script and NotValidBefore have the same value!\n";
    }
    
    return 0;
}