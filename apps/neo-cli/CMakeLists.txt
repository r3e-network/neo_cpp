# Neo C++ CLI Application
cmake_minimum_required(VERSION 3.15)

# Define executable
add_executable(neo-cli
    main.cpp
    cli_service.cpp
    # cli_service_production.cpp  # Commented out due to compilation errors
    commands/command_registry.cpp
    plugins/plugin_manager.cpp
    services/console_service_neo.cpp
)

# Include directories
target_include_directories(neo-cli PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Link libraries
target_link_libraries(neo-cli PRIVATE
    neo_cpp
    neo_plugins
    ${ROCKSDB_LIBRARIES}
    ${Boost_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${CMAKE_DL_LIBS}  # For dynamic loading
    pthread
)

# Compile features
target_compile_features(neo-cli PRIVATE cxx_std_17)

# Installation
install(TARGETS neo-cli
    RUNTIME DESTINATION bin
)

# Install configuration files
install(FILES
    ${CMAKE_SOURCE_DIR}/config/mainnet.json
    ${CMAKE_SOURCE_DIR}/config/testnet.json
    ${CMAKE_SOURCE_DIR}/config/privnet.json
    DESTINATION etc/neo
)

# Create plugin directory
install(DIRECTORY DESTINATION lib/neo/plugins)

# Package CLI as standalone distribution
if(BUILD_PACKAGE)
    # CPack configuration for creating packages
    set(CPACK_PACKAGE_NAME "neo-cli")
    set(CPACK_PACKAGE_VERSION "3.6.0")
    set(CPACK_PACKAGE_DESCRIPTION "Neo blockchain command-line interface")
    set(CPACK_PACKAGE_VENDOR "Neo Project")
    
    # Include all dependencies
    install(CODE "
        include(BundleUtilities)
        fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/bin/neo-cli\" \"\" \"\")
    ")
endif()