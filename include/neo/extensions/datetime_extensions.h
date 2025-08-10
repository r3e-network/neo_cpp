#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace neo::extensions
{
/**
 * @brief Extensions for date and time operations.
 *
 * ## Overview
 * Provides utility methods for timestamp conversions, blockchain time operations,
 * and date/time formatting commonly used in Neo blockchain operations.
 *
 * ## API Reference
 * - **Conversion**: Unix timestamp conversions, millisecond operations
 * - **Blockchain**: Block time validation, timeout calculations
 * - **Formatting**: ISO string conversion, readable time formats
 *
 * ## Usage Examples
 * ```cpp
 * // Get current Unix timestamp
 * auto timestamp = DateTimeExtensions::GetUnixTimestamp();
 *
 * // Convert to readable format
 * auto readable = DateTimeExtensions::ToReadableString(timestamp);
 *
 * // Validate block time
 * bool valid = DateTimeExtensions::IsValidBlockTime(blockTime, previousTime);
 * ```
 */
class DateTimeExtensions
{
   public:
    /**
     * @brief Get current Unix timestamp in seconds
     * @return Current Unix timestamp
     */
    static uint64_t GetUnixTimestamp()
    {
        return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    }

    /**
     * @brief Get current Unix timestamp in milliseconds
     * @return Current Unix timestamp in milliseconds
     */
    static uint64_t GetUnixTimestampMilliseconds()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    }

    /**
     * @brief Convert Unix timestamp to system time point
     * @param timestamp Unix timestamp in seconds
     * @return System time point
     */
    static std::chrono::system_clock::time_point FromUnixTimestamp(uint64_t timestamp)
    {
        return std::chrono::system_clock::from_time_t(static_cast<time_t>(timestamp));
    }

    /**
     * @brief Convert Unix timestamp in milliseconds to system time point
     * @param timestampMs Unix timestamp in milliseconds
     * @return System time point
     */
    static std::chrono::system_clock::time_point FromUnixTimestampMilliseconds(uint64_t timestampMs)
    {
        return std::chrono::system_clock::time_point(std::chrono::milliseconds(timestampMs));
    }

    /**
     * @brief Convert system time point to Unix timestamp
     * @param timePoint System time point
     * @return Unix timestamp in seconds
     */
    static uint64_t ToUnixTimestamp(const std::chrono::system_clock::time_point& timePoint)
    {
        return std::chrono::duration_cast<std::chrono::seconds>(timePoint.time_since_epoch()).count();
    }

    /**
     * @brief Convert system time point to Unix timestamp in milliseconds
     * @param timePoint System time point
     * @return Unix timestamp in milliseconds
     */
    static uint64_t ToUnixTimestampMilliseconds(const std::chrono::system_clock::time_point& timePoint)
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(timePoint.time_since_epoch()).count();
    }

    /**
     * @brief Convert timestamp to ISO 8601 string
     * @param timestamp Unix timestamp in seconds
     * @return ISO 8601 formatted string
     */
    static std::string ToISO8601String(uint64_t timestamp);

    /**
     * @brief Convert timestamp to readable string
     * @param timestamp Unix timestamp in seconds
     * @return Human-readable time string
     */
    static std::string ToReadableString(uint64_t timestamp);

    /**
     * @brief Parse ISO 8601 string to Unix timestamp
     * @param iso8601 ISO 8601 formatted string
     * @return Unix timestamp in seconds
     */
    static uint64_t FromISO8601String(const std::string& iso8601);

    /**
     * @brief Check if a timestamp is valid for blockchain operations
     * @param timestamp Timestamp to validate
     * @param tolerance Tolerance in seconds (default: 900 = 15 minutes)
     * @return True if timestamp is within acceptable range
     */
    static bool IsValidBlockchainTimestamp(uint64_t timestamp, uint64_t tolerance = 900)
    {
        auto currentTime = GetUnixTimestamp();
        return (timestamp <= currentTime + tolerance) && (timestamp >= currentTime - tolerance);
    }

    /**
     * @brief Check if block time is valid relative to previous block
     * @param blockTime Current block timestamp
     * @param previousBlockTime Previous block timestamp
     * @param maxDelta Maximum allowed time difference (default: 7200 = 2 hours)
     * @return True if block time is valid
     */
    static bool IsValidBlockTime(uint64_t blockTime, uint64_t previousBlockTime, uint64_t maxDelta = 7200)
    {
        // Block time should be after previous block but not too far in the future
        return blockTime >= previousBlockTime && blockTime <= previousBlockTime + maxDelta &&
               IsValidBlockchainTimestamp(blockTime);
    }

    /**
     * @brief Calculate time remaining until timeout
     * @param startTime Start timestamp
     * @param timeoutSeconds Timeout duration in seconds
     * @return Seconds remaining (0 if timed out)
     */
    static uint64_t GetTimeRemaining(uint64_t startTime, uint64_t timeoutSeconds)
    {
        auto currentTime = GetUnixTimestamp();
        auto endTime = startTime + timeoutSeconds;

        if (currentTime >= endTime) return 0;

        return endTime - currentTime;
    }

    /**
     * @brief Check if a timeout has occurred
     * @param startTime Start timestamp
     * @param timeoutSeconds Timeout duration in seconds
     * @return True if timed out
     */
    static bool HasTimedOut(uint64_t startTime, uint64_t timeoutSeconds)
    {
        return GetTimeRemaining(startTime, timeoutSeconds) == 0;
    }

    /**
     * @brief Get timestamp for beginning of day (UTC)
     * @param timestamp Input timestamp
     * @return Timestamp for start of day
     */
    static uint64_t GetStartOfDay(uint64_t timestamp);

    /**
     * @brief Get timestamp for end of day (UTC)
     * @param timestamp Input timestamp
     * @return Timestamp for end of day
     */
    static uint64_t GetEndOfDay(uint64_t timestamp);

    /**
     * @brief Add seconds to timestamp
     * @param timestamp Base timestamp
     * @param seconds Seconds to add
     * @return New timestamp
     */
    static uint64_t AddSeconds(uint64_t timestamp, int64_t seconds)
    {
        return static_cast<uint64_t>(static_cast<int64_t>(timestamp) + seconds);
    }

    /**
     * @brief Add minutes to timestamp
     * @param timestamp Base timestamp
     * @param minutes Minutes to add
     * @return New timestamp
     */
    static uint64_t AddMinutes(uint64_t timestamp, int64_t minutes) { return AddSeconds(timestamp, minutes * 60); }

    /**
     * @brief Add hours to timestamp
     * @param timestamp Base timestamp
     * @param hours Hours to add
     * @return New timestamp
     */
    static uint64_t AddHours(uint64_t timestamp, int64_t hours) { return AddSeconds(timestamp, hours * 3600); }

    /**
     * @brief Add days to timestamp
     * @param timestamp Base timestamp
     * @param days Days to add
     * @return New timestamp
     */
    static uint64_t AddDays(uint64_t timestamp, int64_t days) { return AddSeconds(timestamp, days * 86400); }
};
}  // namespace neo::extensions
