#!/bin/bash

# Test script for working Neo node
echo "Testing Working Neo Node..."

# Start the node in background and send commands
(
    sleep 2
    echo "help"
    sleep 1
    echo "stats"
    sleep 1
    echo "store 01 0123456789"
    sleep 1
    echo "get 01"
    sleep 1
    echo "exec 10"  # PUSH0 opcode
    sleep 1
    echo "block"
    sleep 1
    echo "stats"
    sleep 1
    echo "quit"
) | ./working_neo_node

echo "Test completed!"