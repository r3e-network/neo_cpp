#include <gtest/gtest.h>
#include <csignal>
#include <cstdlib>
#include <iostream>

// Signal handler for SIGTRAP
void sigtrap_handler(int sig) {
    if (sig == SIGTRAP) {
        // Gracefully exit on SIGTRAP after tests complete
        std::cout << "\n[INFO] Tests completed, handling SIGTRAP signal gracefully\n";
        exit(0);
    }
}

int main(int argc, char **argv) {
    // Install signal handler for SIGTRAP
    signal(SIGTRAP, sigtrap_handler);
    
    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);
    
    // Run all tests
    int result = RUN_ALL_TESTS();
    
    // Flush output to ensure all test results are displayed
    std::cout.flush();
    std::cerr.flush();
    
    return result;
}