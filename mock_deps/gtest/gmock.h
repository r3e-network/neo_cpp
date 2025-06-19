#pragma once

#define MOCK_METHOD(return_type, method_name, args, ...) \
    virtual return_type method_name args { return {}; }
