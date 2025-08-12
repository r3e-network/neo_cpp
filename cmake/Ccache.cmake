# Ccache integration for faster builds
# This module enables ccache if available

find_program(CCACHE_PROGRAM ccache)

if(CCACHE_PROGRAM)
    message(STATUS "Found ccache: ${CCACHE_PROGRAM}")
    
    # Set up ccache as compiler launcher
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    
    # Optional: Configure ccache settings
    if(DEFINED ENV{CCACHE_DIR})
        message(STATUS "Using ccache directory: $ENV{CCACHE_DIR}")
    else()
        set(ENV{CCACHE_DIR} "${CMAKE_SOURCE_DIR}/.ccache")
        message(STATUS "Using default ccache directory: ${CMAKE_SOURCE_DIR}/.ccache")
    endif()
    
    # Set ccache configuration
    set(ENV{CCACHE_COMPRESS} "1")
    set(ENV{CCACHE_COMPRESSLEVEL} "6")
    set(ENV{CCACHE_MAXSIZE} "5G")
    
    # Print ccache statistics at the start
    execute_process(
        COMMAND ${CCACHE_PROGRAM} -s
        OUTPUT_VARIABLE CCACHE_STATS
        ERROR_QUIET
    )
    message(STATUS "Ccache stats:\n${CCACHE_STATS}")
    
    # Add target to show ccache stats
    add_custom_target(ccache-stats
        COMMAND ${CCACHE_PROGRAM} -s
        COMMENT "Showing ccache statistics"
    )
    
    # Add target to clear ccache
    add_custom_target(ccache-clean
        COMMAND ${CCACHE_PROGRAM} -C
        COMMENT "Clearing ccache"
    )
else()
    message(STATUS "ccache not found - builds will not be cached")
    message(STATUS "Install ccache for faster incremental builds: brew install ccache")
endif()