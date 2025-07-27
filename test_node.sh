#!/bin/bash

echo "Testing Neo C++ Node"
echo "==================="

# Test commands
(
echo "stats"
echo "store testkey testvalue"
echo "get testkey"
echo "exit"
) | ./build/apps/working_neo_node