#include <iostream>
#include <exception>

// Minimal test to narrow down the issue
int main() {
    try {
        std::cout << "Testing exception handling..." << std::endl;
        
        // Test 1: Basic exception
        try {
            throw std::runtime_error("Test exception");
        } catch (const std::exception& e) {
            std::cout << "Caught specific exception: " << e.what() << std::endl;
        } catch (...) {
            std::cout << "Caught unknown exception in catch(...)" << std::endl;
        }
        
        // Test 2: Unknown exception type
        try {
            throw 42;  // throw an int
        } catch (const std::exception& e) {
            std::cout << "Caught specific exception: " << e.what() << std::endl;
        } catch (...) {
            std::cout << "Caught unknown exception (int) in catch(...)" << std::endl;
        }
        
        std::cout << "Exception handling test completed" << std::endl;
        return 0;
    } catch (...) {
        std::cout << "Exception in main" << std::endl;
        return 1;
    }
}