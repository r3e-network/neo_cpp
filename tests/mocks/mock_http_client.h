#pragma once

#include <gmock/gmock.h>
#include <string>

namespace neo::tests {

class MockHttpClient {
public:
    MOCK_METHOD(std::string, Post, (const std::string& url, const std::string& data), ());
    MOCK_METHOD(std::string, Get, (const std::string& url), ());
    MOCK_METHOD(void, SetHeaders, (const std::map<std::string, std::string>& headers), ());
    MOCK_METHOD(void, SetTimeout, (int timeout_ms), ());
};

} // namespace neo::tests