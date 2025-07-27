#include <cstdio>
#include <ctime>
#include <iomanip>
#include <neo/extensions/datetime_extensions.h>
#include <sstream>
#include <stdexcept>

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

namespace neo::extensions
{
std::string DateTimeExtensions::ToISO8601String(uint64_t timestamp)
{
    auto timePoint = FromUnixTimestamp(timestamp);
    auto time_t_val = std::chrono::system_clock::to_time_t(timePoint);

    struct tm* utc_tm = gmtime(&time_t_val);
    if (!utc_tm)
        throw std::runtime_error("Failed to convert timestamp to UTC time");

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02dZ", utc_tm->tm_year + 1900, utc_tm->tm_mon + 1,
             utc_tm->tm_mday, utc_tm->tm_hour, utc_tm->tm_min, utc_tm->tm_sec);

    return std::string(buffer);
}

std::string DateTimeExtensions::ToReadableString(uint64_t timestamp)
{
    auto timePoint = FromUnixTimestamp(timestamp);
    auto time_t_val = std::chrono::system_clock::to_time_t(timePoint);

    struct tm* utc_tm = gmtime(&time_t_val);
    if (!utc_tm)
        throw std::runtime_error("Failed to convert timestamp to UTC time");

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d UTC", utc_tm->tm_year + 1900, utc_tm->tm_mon + 1,
             utc_tm->tm_mday, utc_tm->tm_hour, utc_tm->tm_min, utc_tm->tm_sec);

    return std::string(buffer);
}

uint64_t DateTimeExtensions::FromISO8601String(const std::string& iso8601)
{
    struct tm tm = {};

    // Try to parse ISO 8601 format: YYYY-MM-DDTHH:MM:SSZ
    int year, month, day, hour, min, sec;
    if (sscanf(iso8601.c_str(), "%d-%d-%dT%d:%d:%dZ", &year, &month, &day, &hour, &min, &sec) == 6)
    {
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = hour;
        tm.tm_min = min;
        tm.tm_sec = sec;
    }
    // Try alternative format without 'T' and 'Z'
    else if (sscanf(iso8601.c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &min, &sec) == 6)
    {
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = hour;
        tm.tm_min = min;
        tm.tm_sec = sec;
    }
    else
    {
        throw std::invalid_argument("Invalid ISO 8601 date format");
    }

    // Convert to time_t (assuming UTC)
    auto time_t_val = mktime(&tm);
    if (time_t_val == -1)
    {
        throw std::invalid_argument("Invalid date/time values");
    }

    return static_cast<uint64_t>(time_t_val);
}

uint64_t DateTimeExtensions::GetStartOfDay(uint64_t timestamp)
{
    auto timePoint = FromUnixTimestamp(timestamp);
    auto time_t_val = std::chrono::system_clock::to_time_t(timePoint);

    struct tm* utc_tm = gmtime(&time_t_val);
    if (!utc_tm)
        throw std::runtime_error("Failed to convert timestamp to UTC time");

    struct tm start_tm = *utc_tm;

    // Set to start of day (00:00:00)
    start_tm.tm_hour = 0;
    start_tm.tm_min = 0;
    start_tm.tm_sec = 0;

    auto startOfDay = mktime(&start_tm);
    return static_cast<uint64_t>(startOfDay);
}

uint64_t DateTimeExtensions::GetEndOfDay(uint64_t timestamp)
{
    auto timePoint = FromUnixTimestamp(timestamp);
    auto time_t_val = std::chrono::system_clock::to_time_t(timePoint);

    struct tm* utc_tm = gmtime(&time_t_val);
    if (!utc_tm)
        throw std::runtime_error("Failed to convert timestamp to UTC time");

    struct tm end_tm = *utc_tm;

    // Set to end of day (23:59:59)
    end_tm.tm_hour = 23;
    end_tm.tm_min = 59;
    end_tm.tm_sec = 59;

    auto endOfDay = mktime(&end_tm);
    return static_cast<uint64_t>(endOfDay);
}
}  // namespace neo::extensions

#ifdef _WIN32
#pragma warning(pop)
#endif
