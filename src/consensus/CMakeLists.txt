# Create the consensus library
add_library(neo_consensus OBJECT
    consensus_message.cpp
    change_view_message.cpp
    prepare_request.cpp
    prepare_response.cpp
    commit_message.cpp
    recovery_message.cpp
    recovery_request.cpp
    consensus_context.cpp
    consensus_payload_helper.cpp
)

# Include directories
target_include_directories(neo_consensus
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(neo_consensus
    PUBLIC
        neo_io
        neo_cryptography
        neo_ledger
        neo_node
        neo_smartcontract
)
