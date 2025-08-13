/**
 * @file string_intern.h
 * @brief String Intern
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_set>

namespace neo::core
{
/**
 * @brief Interned string handle for efficient string comparison and storage
 */
class InternedString
{
   public:
    InternedString() = default;

    const std::string& str() const { return ptr_ ? *ptr_ : empty_string(); }
    std::string_view view() const { return ptr_ ? std::string_view(*ptr_) : std::string_view(); }
    const char* c_str() const { return ptr_ ? ptr_->c_str() : ""; }
    size_t size() const { return ptr_ ? ptr_->size() : 0; }
    bool empty() const { return !ptr_ || ptr_->empty(); }

    // Fast pointer comparison for equality
    bool operator==(const InternedString& other) const { return ptr_ == other.ptr_; }
    bool operator!=(const InternedString& other) const { return ptr_ != other.ptr_; }
    bool operator<(const InternedString& other) const { return ptr_ < other.ptr_; }

    // String comparison
    bool operator==(std::string_view other) const { return view() == other; }
    bool operator!=(std::string_view other) const { return view() != other; }

    explicit operator bool() const { return ptr_ != nullptr; }

   private:
    friend class StringInterner;
    explicit InternedString(std::shared_ptr<const std::string> ptr) : ptr_(std::move(ptr)) {}

    static const std::string& empty_string()
    {
        static const std::string empty;
        return empty;
    }

    std::shared_ptr<const std::string> ptr_;
};

/**
 * @brief Thread-safe string interning pool
 */
class StringInterner
{
   public:
    static StringInterner& instance()
    {
        static StringInterner instance;
        return instance;
    }

    /**
     * @brief Interns a string, returning a handle to the shared instance
     * @param str The string to intern
     * @return Handle to the interned string
     */
    InternedString intern(std::string_view str)
    {
        if (str.empty()) return InternedString();

        std::lock_guard<std::mutex> lock(mutex_);

        // Try to find existing string by creating a temporary shared_ptr for lookup
        auto temp = std::make_shared<const std::string>(str);
        auto it = pool_.find(temp);
        if (it != pool_.end())
        {
            return InternedString(*it);
        }

        // Not found, insert the new string
        pool_.insert(temp);
        return InternedString(temp);
    }

    /**
     * @brief Gets the number of interned strings
     * @return Number of unique strings in the pool
     */
    size_t size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return pool_.size();
    }

    /**
     * @brief Clears the intern pool (use with caution)
     */
    void clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pool_.clear();
    }

    /**
     * @brief Reports interning statistics
     */
    void report_stats() const;

   private:
    StringInterner() = default;

    struct StringPtrHash
    {
        size_t operator()(const std::shared_ptr<const std::string>& ptr) const
        {
            return std::hash<std::string>()(*ptr);
        }
    };

    struct StringPtrEqual
    {
        bool operator()(const std::shared_ptr<const std::string>& a, const std::shared_ptr<const std::string>& b) const
        {
            return *a == *b;
        }
    };

    // Custom comparator for string_view lookup
    struct StringViewEqual
    {
        using is_transparent = void;

        bool operator()(const std::shared_ptr<const std::string>& a, std::string_view b) const { return *a == b; }

        bool operator()(std::string_view a, const std::shared_ptr<const std::string>& b) const { return a == *b; }

        bool operator()(const std::shared_ptr<const std::string>& a, const std::shared_ptr<const std::string>& b) const
        {
            return *a == *b;
        }
    };

    mutable std::mutex mutex_;
    std::unordered_set<std::shared_ptr<const std::string>, StringPtrHash, StringViewEqual> pool_;
};

/**
 * @brief Helper function to intern a string
 */
inline InternedString intern(std::string_view str) { return StringInterner::instance().intern(str); }

}  // namespace neo::core