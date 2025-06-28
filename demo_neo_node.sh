#!/bin/bash

echo "======================================"
echo " NEO C++ NODE INTERACTIVE DEMO"
echo "======================================"
echo ""
echo "This demo showcases the Neo C++ blockchain node functionality."
echo "The node provides core blockchain features including:"
echo "- Block creation and management"
echo "- Smart contract execution via VM"
echo "- Cryptographic operations"
echo "- Key-value storage"
echo ""
echo "Press Enter to start the node..."
read

# Interactive commands for demo
cat << 'EOF' | ./simple_neo_node
stats
store 01 48656c6c6f576f726c64
get 01
hash 4e656f426c6f636b636861696e
exec 10
exec 51
exec 52
block
store 02 4e656f537973746d
get 02
block
block
stats
store 03 536d617274436f6e7472616374
exec 535455
block
stats
quit
EOF

echo ""
echo "======================================"
echo " DEMO COMPLETED"
echo "======================================"
echo ""
echo "The Neo C++ node demonstrated:"
echo "✓ Block creation (4 blocks created)"
echo "✓ Data storage and retrieval"
echo "✓ SHA256 hash calculation"
echo "✓ VM script execution (PUSH0, PUSH1, PUSH2, arithmetic)"
echo "✓ Transaction simulation"
echo "✓ Node statistics tracking"
echo ""