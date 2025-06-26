# Core module CMakeLists.txt

# Find required dependencies
find_package(Boost REQUIRED)

# Create core library
add_library(neo_core OBJECT
    big_decimal.cpp
    fixed8.cpp
    neo_system_simple.cpp
    logging.cpp
    ../protocol_settings.cpp
    ../protocol_settings_json.cpp
    ../hardfork.cpp
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