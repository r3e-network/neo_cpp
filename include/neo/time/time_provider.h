/**
 * @file time_provider.h
 * @brief Time Provider
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#ifndef NEO_TIME_TIME_PROVIDER_H
#define NEO_TIME_TIME_PROVIDER_H

#include <chrono>
#include <cstdint>

namespace neo
{
namespace time
{

/**
 * @brief Provides time-related functionality for the Neo blockchain.
 */
class TimeProvider
{
   public:
    /**
     * @brief Gets the current UTC time as milliseconds since Unix epoch.
     * @return Current time in milliseconds
     */
    static uint64_t GetUtcNow()
    {
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    }

    /**
     * @brief Gets the current UTC time as seconds since Unix epoch.
     * @return Current time in seconds
     */
    static uint64_t GetUtcNowSeconds()
    {
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    }

    /**
     * @brief Converts milliseconds since Unix epoch to seconds.
     * @param milliseconds Time in milliseconds
     * @return Time in seconds
     */
    static uint64_t MillisecondsToSeconds(uint64_t milliseconds) { return milliseconds / 1000; }

    /**
     * @brief Converts seconds since Unix epoch to milliseconds.
     * @param seconds Time in seconds
     * @return Time in milliseconds
     */
    static uint64_t SecondsToMilliseconds(uint64_t seconds) { return seconds * 1000; }
};

}  // namespace time
}  // namespace neo

#endif  // NEO_TIME_TIME_PROVIDER_H