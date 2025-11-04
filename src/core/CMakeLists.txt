# Core module CMakeLists.txt

# Set policy for FindBoost module
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Find required dependencies
find_package(Boost REQUIRED)

# Create core library
add_library(neo_core OBJECT
    big_decimal.cpp
    fixed8.cpp
    neo_system.cpp
    neo_system_factory.cpp  # Factory for proper NeoSystem initialization
    configuration_manager.cpp
    logging.cpp
    common_logging.cpp     # Common logging implementation
    exceptions.cpp         # Comprehensive exception framework
    validation.cpp         # Input validation and security framework
    error_recovery.cpp     # Error recovery and fault tolerance framework
    memory_pool.cpp        # Memory pool allocators
    performance_config.cpp # Performance optimization settings
    string_intern.cpp      # String interning for efficiency
    ../settings.cpp        # Settings implementation
    protocol_settings.cpp
    ../protocol_settings.cpp
    ../protocol_settings_json.cpp
    ../hardfork.cpp
    # network_stubs.cpp  # Disabled - duplicate symbols with network/stubs.cpp
)

target_include_directories(neo_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(neo_core PUBLIC
    nlohmann_json::nlohmann_json
    Boost::headers
)

# Add Boost multiprecision support
target_compile_definitions(neo_core PUBLIC
    BOOST_MULTIPRECISION_ENABLE_CPP_INT
)

# Set C++20 standard
target_compile_features(neo_core PUBLIC cxx_std_20)
