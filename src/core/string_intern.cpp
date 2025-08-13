/**
 * @file string_intern.cpp
 * @brief String Intern
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/core/logging.h>
#include <neo/core/string_intern.h>

namespace neo::core
{
void StringInterner::report_stats() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    size_t total_memory = 0;
    size_t total_length = 0;

    for (const auto& str_ptr : pool_)
    {
        total_memory += str_ptr->size() + sizeof(std::string);
        total_length += str_ptr->size();
    }

    LOG_INFO("String Interning Statistics:");
    LOG_INFO("  Interned strings: {}", pool_.size());
    LOG_INFO("  Total characters: {}", total_length);
    LOG_INFO("  Memory usage: {} KB", total_memory / 1024);
    LOG_INFO("  Average string length: {:.2f}", pool_.empty() ? 0.0 : static_cast<double>(total_length) / pool_.size());
}
}  // namespace neo::core