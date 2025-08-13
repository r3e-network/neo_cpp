#!/bin/bash

# Neo C++ TPS (Transactions Per Second) Test Script
# Measures transaction throughput performance

set -e

# Configuration
NODE_URL="${NODE_URL:-http://localhost:10332}"
TEST_DURATION="${TEST_DURATION:-60}"  # seconds
TARGET_TPS="${TARGET_TPS:-5000}"
PARALLEL_CLIENTS="${PARALLEL_CLIENTS:-10}"
OUTPUT_FILE="tps_result.txt"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo "======================================"
echo "Neo C++ TPS Performance Test"
echo "======================================"
echo "Node URL: $NODE_URL"
echo "Duration: $TEST_DURATION seconds"
echo "Target TPS: $TARGET_TPS"
echo "Parallel Clients: $PARALLEL_CLIENTS"
echo ""

# Function to generate test transaction
generate_transaction() {
    cat << EOF
{
    "jsonrpc": "2.0",
    "method": "sendrawtransaction",
    "params": ["00d11f4e96b4000000000000000000000000000000000000000001"],
    "id": 1
}
EOF
}

# Function to send transactions
send_transactions() {
    local client_id=$1
    local count=0
    local start_time=$(date +%s)
    local end_time=$((start_time + TEST_DURATION))
    
    while [ $(date +%s) -lt $end_time ]; do
        # Send transaction
        curl -s -X POST $NODE_URL \
            -H "Content-Type: application/json" \
            -d "$(generate_transaction)" > /dev/null 2>&1
        
        count=$((count + 1))
        
        # Progress indicator every 100 transactions
        if [ $((count % 100)) -eq 0 ]; then
            echo -ne "\rClient $client_id: $count transactions sent..."
        fi
    done
    
    echo $count > "/tmp/tps_client_${client_id}.txt"
}

# Check if node is running
check_node() {
    echo "Checking node availability..."
    if ! curl -s -X POST $NODE_URL \
        -H "Content-Type: application/json" \
        -d '{"jsonrpc":"2.0","method":"getversion","params":[],"id":1}' > /dev/null 2>&1; then
        echo -e "${RED}✗${NC} Node is not responding at $NODE_URL"
        exit 1
    fi
    echo -e "${GREEN}✓${NC} Node is running"
}

# Warm up the node
warmup() {
    echo ""
    echo "Warming up node..."
    for i in {1..100}; do
        curl -s -X POST $NODE_URL \
            -H "Content-Type: application/json" \
            -d "$(generate_transaction)" > /dev/null 2>&1
    done
    echo -e "${GREEN}✓${NC} Warmup complete"
}

# Run TPS test
run_test() {
    echo ""
    echo "Starting TPS test..."
    echo "Running for $TEST_DURATION seconds with $PARALLEL_CLIENTS parallel clients..."
    echo ""
    
    # Start timer
    START_TIME=$(date +%s)
    
    # Launch parallel clients
    for i in $(seq 1 $PARALLEL_CLIENTS); do
        send_transactions $i &
        PIDS[$i]=$!
    done
    
    # Wait for all clients to complete
    for i in $(seq 1 $PARALLEL_CLIENTS); do
        wait ${PIDS[$i]}
    done
    
    # Calculate total time
    END_TIME=$(date +%s)
    ACTUAL_DURATION=$((END_TIME - START_TIME))
    
    echo ""
    echo ""
    echo "Test complete!"
}

# Calculate results
calculate_results() {
    echo ""
    echo "Calculating results..."
    
    # Sum up all transactions
    TOTAL_TRANSACTIONS=0
    for i in $(seq 1 $PARALLEL_CLIENTS); do
        if [ -f "/tmp/tps_client_${i}.txt" ]; then
            CLIENT_TXS=$(cat "/tmp/tps_client_${i}.txt")
            TOTAL_TRANSACTIONS=$((TOTAL_TRANSACTIONS + CLIENT_TXS))
            rm "/tmp/tps_client_${i}.txt"
        fi
    done
    
    # Calculate TPS
    TPS=$((TOTAL_TRANSACTIONS / ACTUAL_DURATION))
    
    # Save result
    echo $TPS > $OUTPUT_FILE
    
    # Display results
    echo ""
    echo "======================================"
    echo "Test Results"
    echo "======================================"
    echo -e "Total Transactions: ${BLUE}$TOTAL_TRANSACTIONS${NC}"
    echo -e "Test Duration: ${BLUE}$ACTUAL_DURATION seconds${NC}"
    echo -e "Transactions Per Second: ${BLUE}$TPS${NC}"
    echo -e "Target TPS: ${BLUE}$TARGET_TPS${NC}"
    echo ""
    
    # Check if target met
    if [ $TPS -ge $TARGET_TPS ]; then
        echo -e "${GREEN}✓ SUCCESS: TPS target met! ($TPS >= $TARGET_TPS)${NC}"
        RESULT=0
    else
        echo -e "${RED}✗ FAILED: TPS below target ($TPS < $TARGET_TPS)${NC}"
        RESULT=1
    fi
    
    echo ""
    echo "======================================"
    
    # Generate detailed report
    generate_report
    
    return $RESULT
}

# Generate detailed report
generate_report() {
    cat > tps_report.json << EOF
{
    "timestamp": "$(date -Iseconds)",
    "configuration": {
        "node_url": "$NODE_URL",
        "test_duration": $TEST_DURATION,
        "parallel_clients": $PARALLEL_CLIENTS,
        "target_tps": $TARGET_TPS
    },
    "results": {
        "total_transactions": $TOTAL_TRANSACTIONS,
        "actual_duration": $ACTUAL_DURATION,
        "tps": $TPS,
        "target_met": $([ $TPS -ge $TARGET_TPS ] && echo "true" || echo "false")
    }
}
EOF
    
    echo "Detailed report saved to tps_report.json"
}

# Cleanup function
cleanup() {
    echo ""
    echo "Cleaning up..."
    for i in $(seq 1 $PARALLEL_CLIENTS); do
        if [ -f "/tmp/tps_client_${i}.txt" ]; then
            rm "/tmp/tps_client_${i}.txt"
        fi
    done
}

# Trap cleanup on exit
trap cleanup EXIT

# Main execution
main() {
    check_node
    warmup
    run_test
    calculate_results
    exit $?
}

# Run if not sourced
if [ "${BASH_SOURCE[0]}" == "${0}" ]; then
    main "$@"
fi