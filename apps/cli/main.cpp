#include "main_service.h"
#include <exception>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
    try
    {
        // Create main service
        neo::cli::MainService service;

        // Convert command line arguments
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
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return 1;
    }
}
