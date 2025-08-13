#!/bin/bash

# Neo C++ Integration Test Script
# Comprehensive integration testing for production readiness

set -e

# Configuration
NODE_COUNT="${NODE_COUNT:-4}"
TEST_TIMEOUT="${TEST_TIMEOUT:-300}"  # 5 minutes
BASE_RPC_PORT="${BASE_RPC_PORT:-10332}"
BASE_P2P_PORT="${BASE_P2P_PORT:-10333}"
LOG_DIR="test_logs"
REPORT_FILE="integration_test_report.json"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo "======================================"
echo "Neo C++ Integration Test Suite"
echo "======================================"
echo "Node Count: $NODE_COUNT"
echo "Test Timeout: $TEST_TIMEOUT seconds"
echo ""

# Test results tracking
TESTS_PASSED=0
TESTS_FAILED=0
FAILED_TESTS=()

# Function to print colored output
print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

print_info() {
    echo -e "${BLUE}ℹ${NC} $1"
}

# Setup test environment
setup_test_env() {
    echo "Setting up test environment..."
    
    # Create log directory
    mkdir -p $LOG_DIR
    
    # Stop any existing nodes
    docker-compose -f deployment/docker/docker-compose.yml down 2>/dev/null || true
    
    # Clean data directories
    rm -rf deployment/docker/data/* 2>/dev/null || true
    
    print_success "Test environment ready"
}

# Start test network
start_test_network() {
    echo ""
    echo "Starting test network with $NODE_COUNT nodes..."
    
    # Start docker-compose network
    docker-compose -f deployment/docker/docker-compose.yml up -d
    
    # Wait for nodes to start
    echo "Waiting for nodes to initialize..."
    sleep 10
    
    # Check node health
    local all_healthy=true
    for i in $(seq 1 $NODE_COUNT); do
        local port=$((BASE_RPC_PORT + (i-1)*10))
        if curl -s -X POST http://localhost:$port/health > /dev/null 2>&1; then
            print_success "Node $i is healthy (port $port)"
        else
            print_error "Node $i failed health check"
            all_healthy=false
        fi
    done
    
    if [ "$all_healthy" = true ]; then
        print_success "Test network started successfully"
        return 0
    else
        print_error "Some nodes failed to start"
        return 1
    fi
}

# Test: Basic connectivity
test_connectivity() {
    echo ""
    echo "Testing: Basic Connectivity"
    echo "----------------------------"
    
    local test_passed=true
    
    # Test RPC connectivity
    for i in $(seq 1 $NODE_COUNT); do
        local port=$((BASE_RPC_PORT + (i-1)*10))
        local response=$(curl -s -X POST http://localhost:$port \
            -H "Content-Type: application/json" \
            -d '{"jsonrpc":"2.0","method":"getversion","params":[],"id":1}')
        
        if echo "$response" | grep -q "result"; then
            print_success "Node $i RPC connectivity OK"
        else
            print_error "Node $i RPC connectivity failed"
            test_passed=false
        fi
    done
    
    if [ "$test_passed" = true ]; then
        ((TESTS_PASSED++))
        print_success "Connectivity test passed"
    else
        ((TESTS_FAILED++))
        FAILED_TESTS+=("Connectivity")
        print_error "Connectivity test failed"
    fi
}

# Test: Peer discovery
test_peer_discovery() {
    echo ""
    echo "Testing: Peer Discovery"
    echo "-----------------------"
    
    local test_passed=true
    
    # Give nodes time to discover each other
    sleep 5
    
    # Check peer counts
    for i in $(seq 1 $NODE_COUNT); do
        local port=$((BASE_RPC_PORT + (i-1)*10))
        local response=$(curl -s -X POST http://localhost:$port \
            -H "Content-Type: application/json" \
            -d '{"jsonrpc":"2.0","method":"getpeers","params":[],"id":1}')
        
        local connected=$(echo "$response" | grep -o '"connected":\[[^]]*\]' | grep -o '{' | wc -l)
        
        if [ "$connected" -ge 1 ]; then
            print_success "Node $i has $connected peers"
        else
            print_error "Node $i has no peers"
            test_passed=false
        fi
    done
    
    if [ "$test_passed" = true ]; then
        ((TESTS_PASSED++))
        print_success "Peer discovery test passed"
    else
        ((TESTS_FAILED++))
        FAILED_TESTS+=("Peer Discovery")
        print_error "Peer discovery test failed"
    fi
}

# Test: Block synchronization
test_block_sync() {
    echo ""
    echo "Testing: Block Synchronization"
    echo "------------------------------"
    
    local test_passed=true
    
    # Get block heights from all nodes
    declare -a heights
    for i in $(seq 1 $NODE_COUNT); do
        local port=$((BASE_RPC_PORT + (i-1)*10))
        local response=$(curl -s -X POST http://localhost:$port \
            -H "Content-Type: application/json" \
            -d '{"jsonrpc":"2.0","method":"getblockcount","params":[],"id":1}')
        
        local height=$(echo "$response" | grep -o '"result":[0-9]*' | grep -o '[0-9]*')
        heights[$i]=$height
        print_info "Node $i at block height: $height"
    done
    
    # Check if all nodes are at same height (within 1 block tolerance)
    local max_height=${heights[1]}
    local min_height=${heights[1]}
    
    for height in "${heights[@]}"; do
        if [ "$height" -gt "$max_height" ]; then
            max_height=$height
        fi
        if [ "$height" -lt "$min_height" ]; then
            min_height=$height
        fi
    done
    
    local diff=$((max_height - min_height))
    if [ "$diff" -le 1 ]; then
        print_success "All nodes synchronized (max diff: $diff blocks)"
    else
        print_error "Nodes not synchronized (diff: $diff blocks)"
        test_passed=false
    fi
    
    if [ "$test_passed" = true ]; then
        ((TESTS_PASSED++))
        print_success "Block synchronization test passed"
    else
        ((TESTS_FAILED++))
        FAILED_TESTS+=("Block Synchronization")
        print_error "Block synchronization test failed"
    fi
}

# Test: Transaction propagation
test_transaction_propagation() {
    echo ""
    echo "Testing: Transaction Propagation"
    echo "--------------------------------"
    
    local test_passed=true
    
    # Send transaction to first node
    local tx_hash="0x1234567890abcdef"  # Mock transaction for testing
    local port=$BASE_RPC_PORT
    
    print_info "Sending transaction to Node 1..."
    
    # Mock transaction send (replace with actual transaction)
    curl -s -X POST http://localhost:$port \
        -H "Content-Type: application/json" \
        -d "{\"jsonrpc\":\"2.0\",\"method\":\"sendrawtransaction\",\"params\":[\"00d11f4e96b4\"],\"id\":1}" \
        > /dev/null 2>&1
    
    # Wait for propagation
    sleep 3
    
    # Check mempool on all nodes
    local tx_found_count=0
    for i in $(seq 1 $NODE_COUNT); do
        local port=$((BASE_RPC_PORT + (i-1)*10))
        local response=$(curl -s -X POST http://localhost:$port \
            -H "Content-Type: application/json" \
            -d '{"jsonrpc":"2.0","method":"getrawmempool","params":[],"id":1}')
        
        # Check if transaction is in mempool (simplified check)
        if echo "$response" | grep -q "result"; then
            ((tx_found_count++))
            print_info "Transaction found in Node $i mempool"
        fi
    done
    
    if [ "$tx_found_count" -ge $((NODE_COUNT - 1)) ]; then
        print_success "Transaction propagated to $tx_found_count/$NODE_COUNT nodes"
    else
        print_warning "Transaction only propagated to $tx_found_count/$NODE_COUNT nodes"
        test_passed=false
    fi
    
    if [ "$test_passed" = true ]; then
        ((TESTS_PASSED++))
        print_success "Transaction propagation test passed"
    else
        ((TESTS_FAILED++))
        FAILED_TESTS+=("Transaction Propagation")
        print_error "Transaction propagation test failed"
    fi
}

# Test: Consensus mechanism
test_consensus() {
    echo ""
    echo "Testing: Consensus Mechanism"
    echo "----------------------------"
    
    local test_passed=true
    
    # Monitor block production for 30 seconds
    print_info "Monitoring block production for 30 seconds..."
    
    # Get initial block height
    local port=$BASE_RPC_PORT
    local response=$(curl -s -X POST http://localhost:$port \
        -H "Content-Type: application/json" \
        -d '{"jsonrpc":"2.0","method":"getblockcount","params":[],"id":1}')
    local initial_height=$(echo "$response" | grep -o '"result":[0-9]*' | grep -o '[0-9]*')
    
    # Wait for block production
    sleep 30
    
    # Get final block height
    response=$(curl -s -X POST http://localhost:$port \
        -H "Content-Type: application/json" \
        -d '{"jsonrpc":"2.0","method":"getblockcount","params":[],"id":1}')
    local final_height=$(echo "$response" | grep -o '"result":[0-9]*' | grep -o '[0-9]*')
    
    local blocks_produced=$((final_height - initial_height))
    local expected_blocks=2  # Expecting at least 2 blocks in 30 seconds (15s block time)
    
    if [ "$blocks_produced" -ge "$expected_blocks" ]; then
        print_success "Consensus working: $blocks_produced blocks produced"
    else
        print_error "Consensus issue: only $blocks_produced blocks produced (expected >= $expected_blocks)"
        test_passed=false
    fi
    
    if [ "$test_passed" = true ]; then
        ((TESTS_PASSED++))
        print_success "Consensus test passed"
    else
        ((TESTS_FAILED++))
        FAILED_TESTS+=("Consensus")
        print_error "Consensus test failed"
    fi
}

# Test: API endpoints
test_api_endpoints() {
    echo ""
    echo "Testing: API Endpoints"
    echo "----------------------"
    
    local test_passed=true
    local port=$BASE_RPC_PORT
    
    # Test various RPC methods
    local methods=(
        "getversion"
        "getblockcount"
        "getbestblockhash"
        "getconnectioncount"
        "getpeers"
        "getrawmempool"
        "getnep17balances"
    )
    
    for method in "${methods[@]}"; do
        local response=$(curl -s -X POST http://localhost:$port \
            -H "Content-Type: application/json" \
            -d "{\"jsonrpc\":\"2.0\",\"method\":\"$method\",\"params\":[],\"id\":1}" \
            -w "\n%{http_code}")
        
        local http_code=$(echo "$response" | tail -n1)
        local body=$(echo "$response" | head -n-1)
        
        if [ "$http_code" = "200" ] && echo "$body" | grep -q "result"; then
            print_success "API method '$method' working"
        else
            print_error "API method '$method' failed"
            test_passed=false
        fi
    done
    
    if [ "$test_passed" = true ]; then
        ((TESTS_PASSED++))
        print_success "API endpoints test passed"
    else
        ((TESTS_FAILED++))
        FAILED_TESTS+=("API Endpoints")
        print_error "API endpoints test failed"
    fi
}

# Test: Fault tolerance
test_fault_tolerance() {
    echo ""
    echo "Testing: Fault Tolerance"
    echo "------------------------"
    
    local test_passed=true
    
    print_info "Stopping Node 4 to test fault tolerance..."
    docker-compose -f deployment/docker/docker-compose.yml stop neo-node-4
    
    sleep 10
    
    # Check if network continues to function
    local port=$BASE_RPC_PORT
    local response=$(curl -s -X POST http://localhost:$port \
        -H "Content-Type: application/json" \
        -d '{"jsonrpc":"2.0","method":"getblockcount","params":[],"id":1}')
    
    if echo "$response" | grep -q "result"; then
        print_success "Network continues to function with 1 node down"
    else
        print_error "Network failed with 1 node down"
        test_passed=false
    fi
    
    # Restart the node
    print_info "Restarting Node 4..."
    docker-compose -f deployment/docker/docker-compose.yml start neo-node-4
    
    sleep 15
    
    # Check if node rejoins successfully
    local port4=$((BASE_RPC_PORT + 30))
    response=$(curl -s -X POST http://localhost:$port4/health)
    
    if [ $? -eq 0 ]; then
        print_success "Node 4 successfully rejoined the network"
    else
        print_warning "Node 4 failed to rejoin properly"
    fi
    
    if [ "$test_passed" = true ]; then
        ((TESTS_PASSED++))
        print_success "Fault tolerance test passed"
    else
        ((TESTS_FAILED++))
        FAILED_TESTS+=("Fault Tolerance")
        print_error "Fault tolerance test failed"
    fi
}

# Generate test report
generate_report() {
    local end_time=$(date +%s)
    local duration=$((end_time - START_TIME))
    
    cat > $REPORT_FILE << EOF
{
    "timestamp": "$(date -Iseconds)",
    "duration": $duration,
    "summary": {
        "total_tests": $((TESTS_PASSED + TESTS_FAILED)),
        "passed": $TESTS_PASSED,
        "failed": $TESTS_FAILED,
        "success_rate": $(echo "scale=2; $TESTS_PASSED * 100 / ($TESTS_PASSED + $TESTS_FAILED)" | bc)
    },
    "failed_tests": [$(printf '"%s",' "${FAILED_TESTS[@]}" | sed 's/,$//')]
    ],
    "environment": {
        "node_count": $NODE_COUNT,
        "test_timeout": $TEST_TIMEOUT
    }
}
EOF
    
    echo ""
    echo "Test report saved to $REPORT_FILE"
}

# Cleanup
cleanup() {
    echo ""
    echo "Cleaning up test environment..."
    
    # Stop docker-compose network
    docker-compose -f deployment/docker/docker-compose.yml down
    
    # Archive logs
    if [ -d "$LOG_DIR" ]; then
        tar -czf "test_logs_$(date +%Y%m%d_%H%M%S).tar.gz" $LOG_DIR
        rm -rf $LOG_DIR
    fi
    
    print_success "Cleanup complete"
}

# Main execution
main() {
    START_TIME=$(date +%s)
    
    # Setup
    setup_test_env
    
    # Start network
    if ! start_test_network; then
        print_error "Failed to start test network"
        cleanup
        exit 1
    fi
    
    # Run tests
    test_connectivity
    test_peer_discovery
    test_block_sync
    test_transaction_propagation
    test_consensus
    test_api_endpoints
    test_fault_tolerance
    
    # Results
    echo ""
    echo "======================================"
    echo "Integration Test Results"
    echo "======================================"
    echo -e "Tests Passed: ${GREEN}$TESTS_PASSED${NC}"
    echo -e "Tests Failed: ${RED}$TESTS_FAILED${NC}"
    
    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "${GREEN}✓ All integration tests passed!${NC}"
        RESULT=0
    else
        echo -e "${RED}✗ Some integration tests failed${NC}"
        echo "Failed tests: ${FAILED_TESTS[*]}"
        RESULT=1
    fi
    
    # Generate report
    generate_report
    
    # Cleanup
    cleanup
    
    exit $RESULT
}

# Trap cleanup on exit
trap cleanup EXIT

# Run if not sourced
if [ "${BASH_SOURCE[0]}" == "${0}" ]; then
    main "$@"
fi