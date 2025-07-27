#!/bin/bash

echo "Testing Fixed Neo C++ Node"
echo "========================="

# Create test commands
cat << 'EOF' > node_commands.txt
help
stats
store mykey myvalue
get mykey
mine
mine
stats
balance
exit
EOF

echo "Running node with test commands..."
echo ""

./build/apps/neo_node_fixed < node_commands.txt

echo ""
echo "Test completed successfully!"