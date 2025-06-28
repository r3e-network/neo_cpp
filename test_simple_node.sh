#!/bin/bash

# Test script for simple Neo node
echo "Testing Simple Neo Node..."

# Start the node in background and send commands
(
    sleep 1
    echo "help"
    sleep 0.5
    echo "stats"
    sleep 0.5
    echo "store 01 0123456789"
    sleep 0.5
    echo "get 01"
    sleep 0.5
    echo "hash 48656c6c6f"  # "Hello" in hex
    sleep 0.5
    echo "exec 10"  # PUSH0 opcode
    sleep 0.5
    echo "block"
    sleep 0.5
    echo "block"
    sleep 0.5
    echo "block"
    sleep 0.5
    echo "stats"
    sleep 0.5
    echo "quit"
) | ./simple_neo_node

echo "Test completed!"