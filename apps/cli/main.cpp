#include "main_service.h"
#include <iostream>
#include <vector>
#include <string>

int main(int argc, char** argv)
{
    try
    {
        // Create main service
        neo::cli::MainService service;
        
        // Run with command line arguments
        std::vector<std::string> args;
        for (int i = 1; i < argc; i++)
        {
            args.push_back(argv[i]);
        }
        
        // Run the service
        service.Run(args);
        
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
