#pragma once

// Redirect to the main JSON interfaces
#include <neo/io/json.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>

// Re-export the main types for convenience
namespace neo::json
{
    using JsonReader = io::JsonReader;
    using JsonWriter = io::JsonWriter;
} 