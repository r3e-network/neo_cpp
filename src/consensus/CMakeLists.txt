# Neo Consensus Module

# Create consensus library
add_library(neo_consensus OBJECT
    dbft_consensus.cpp
    consensus_state.cpp
    consensus_message.cpp
    # recovery_message.cpp  # Disabled - API mismatch with consensus message types
)

# Set include directories
target_include_directories(neo_consensus PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(neo_consensus PUBLIC
    neo_core
    neo_cryptography
    neo_io
    neo_persistence
    neo_network
    neo_ledger
)