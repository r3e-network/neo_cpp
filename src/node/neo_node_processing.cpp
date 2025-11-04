/**
 * @file neo_node_processing.cpp
 * @brief Neo Node status loop and diagnostics
 */

#include <neo/node/neo_node.h>

#include <chrono>
#include <thread>
#include <exception>
#include <string>

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#else
#include <fstream>
#include <sstream>
#endif

namespace neo::node
{
void NeoNode::MainLoop()
{
    logger_->Info("Status monitor loop started");

    const auto statusInterval = std::chrono::minutes(1);
    while (running_ && !shutdownRequested_)
    {
        try
        {
            ReportStatus();
        }
        catch (const std::exception& ex)
        {
            logger_->Warning(std::string("Failed to gather status metrics: ") + ex.what());
        }

        auto slept = std::chrono::seconds(0);
        while (slept < statusInterval && running_ && !shutdownRequested_)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            slept += std::chrono::seconds(1);
        }
    }

    logger_->Info("Status monitor loop stopped");
}

void NeoNode::ReportStatus()
{
    logger_->Info("=== Neo Node Status ===");
    logger_->Info("Block Height: " + std::to_string(GetBlockHeight()));
    logger_->Info("Connected Peers: " + std::to_string(GetConnectedPeersCount()));
    logger_->Info("Memory Pool: " + std::to_string(GetMemoryPoolCount()) + " transactions");

    if (neoSystem_)
    {
        logger_->Info(std::string("System Running: ") + (neoSystem_->IsRunning() ? "true" : "false"));
    }

    const auto memoryUsage = GetMemoryUsage();
    if (memoryUsage > 0)
    {
        const double rssMb = static_cast<double>(memoryUsage) / 1024.0 / 1024.0;
        logger_->Info("Process RSS: " + std::to_string(rssMb) + " MB");
    }
}

size_t NeoNode::GetMemoryUsage() const
{
    try
    {
#if defined(_WIN32)
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        {
            return static_cast<size_t>(pmc.WorkingSetSize);
        }
#elif defined(__APPLE__)
        mach_task_basic_info info;
        mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
        if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info), &infoCount) ==
            KERN_SUCCESS)
        {
            return static_cast<size_t>(info.resident_size);
        }
#elif defined(__linux__)
        std::ifstream statusFile("/proc/self/status");
        std::string line;
        while (std::getline(statusFile, line))
        {
            if (line.rfind("VmRSS:", 0) == 0)
            {
                std::istringstream iss(line);
                std::string label, value, unit;
                iss >> label >> value >> unit;
                return static_cast<size_t>(std::stoull(value)) * 1024;  // Convert kB to bytes
            }
        }
#endif
    }
    catch (const std::exception&)
    {
        // Ignore errors, fall through to returning 0
    }

    return 0;
}
}  // namespace neo::node
