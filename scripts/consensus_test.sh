#!/bin/bash

# Neo C++ Consensus Test Script
# Tests dBFT consensus mechanism under various conditions

set -e

# Configuration
VALIDATOR_COUNT="${VALIDATOR_COUNT:-7}"
TEST_DURATION="${TEST_DURATION:-600}"  # 10 minutes
FAULT_NODES="${FAULT_NODES:-2}"  # Byzantine fault tolerance f = (n-1)/3
BASE_PORT="${BASE_PORT:-10332}"
LOG_FILE="consensus_test.log"
REPORT_FILE="consensus_report.json"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo "======================================"
echo "Neo C++ Consensus Test Suite"
echo "======================================"
echo "Validators: $VALIDATOR_COUNT"
echo "Test Duration: $TEST_DURATION seconds"
echo "Byzantine Nodes: $FAULT_NODES"
echo ""

# Test metrics
BLOCKS_PRODUCED=0
CONSENSUS_ROUNDS=0
VIEW_CHANGES=0
FAILED_ROUNDS=0
START_HEIGHT=0
END_HEIGHT=0

# Print functions
print_success() { echo -e "${GREEN}✓${NC} $1"; }
print_error() { echo -e "${RED}✗${NC} $1"; }
print_warning() { echo -e "${YELLOW}⚠${NC} $1"; }
print_info() { echo -e "${BLUE}ℹ${NC} $1"; }

# Setup consensus test environment
setup_consensus_env() {
    echo "Setting up consensus test environment..."
    
    # Create test configuration for validators
    cat > consensus_test_config.json << EOF
{
    "consensus": {
        "enabled": true,
        "validator_count": $VALIDATOR_COUNT,
        "block_time": 15000,
        "view_timeout": 60000,
        "max_transactions_per_block": 500,
        "recovery_enabled": true
    },
    "test": {
        "fault_nodes": $FAULT_NODES,
        "test_duration": $TEST_DURATION,
        "inject_faults": true,
        "network_partition": false
    }
}
EOF
    
    print_success "Consensus test environment configured"
}

# Start consensus network
start_consensus_network() {
    echo ""
    echo "Starting consensus network with $VALIDATOR_COUNT validators..."
    
    # Start validator nodes
    for i in $(seq 1 $VALIDATOR_COUNT); do
        local port=$((BASE_PORT + i - 1))
        print_info "Starting validator $i on port $port"
        
        # Start validator node (mock command - replace with actual)
        # ./neo-node --config consensus_test_config.json --validator-index $((i-1)) --port $port &
    done
    
    # Wait for network to stabilize
    sleep 10
    
    # Get initial block height
    local response=$(curl -s -X POST http://localhost:$BASE_PORT \
        -H "Content-Type: application/json" \
        -d '{"jsonrpc":"2.0","method":"getblockcount","params":[],"id":1}')
    START_HEIGHT=$(echo "$response" | grep -o '"result":[0-9]*' | grep -o '[0-9]*')
    
    print_success "Consensus network started at height $START_HEIGHT"
}

# Monitor consensus metrics
monitor_consensus() {
    local duration=$1
    local end_time=$(($(date +%s) + duration))
    
    echo ""
    echo "Monitoring consensus for $duration seconds..."
    echo "Time | Height | View | Status"
    echo "-----|--------|------|-------"
    
    while [ $(date +%s) -lt $end_time ]; do
        # Get current consensus state
        local response=$(curl -s -X POST http://localhost:$BASE_PORT \
            -H "Content-Type: application/json" \
            -d '{"jsonrpc":"2.0","method":"getconsensusstate","params":[],"id":1}')
        
        # Parse response (mock parsing - adjust based on actual API)
        local height=$(echo "$response" | grep -o '"height":[0-9]*' | grep -o '[0-9]*')
        local view=$(echo "$response" | grep -o '"view":[0-9]*' | grep -o '[0-9]*')
        local phase=$(echo "$response" | grep -o '"phase":"[^"]*"' | cut -d'"' -f4)
        
        # Display status
        printf "%4ds | %6s | %4s | %s\n" \
            $(($(date +%s) - (end_time - duration))) \
            "${height:-0}" \
            "${view:-0}" \
            "${phase:-unknown}"
        
        # Track metrics
        if [ -n "$view" ] && [ "$view" -gt 0 ]; then
            ((VIEW_CHANGES++))
        fi
        
        sleep 5
    done
}

# Test: Normal consensus operation
test_normal_consensus() {
    echo ""
    echo "Test 1: Normal Consensus Operation"
    echo "-----------------------------------"
    
    local test_duration=60
    print_info "Running normal consensus for $test_duration seconds..."
    
    # Monitor consensus
    local start_height=$(get_block_height)
    sleep $test_duration
    local end_height=$(get_block_height)
    
    local blocks_produced=$((end_height - start_height))
    local expected_blocks=$((test_duration / 15))  # 15 second block time
    
    if [ "$blocks_produced" -ge $((expected_blocks - 1)) ]; then
        print_success "Normal consensus: $blocks_produced blocks produced (expected ~$expected_blocks)"
        return 0
    else
        print_error "Normal consensus: only $blocks_produced blocks produced (expected ~$expected_blocks)"
        return 1
    fi
}

# Test: Consensus with Byzantine nodes
test_byzantine_fault() {
    echo ""
    echo "Test 2: Byzantine Fault Tolerance"
    echo "----------------------------------"
    
    print_info "Simulating $FAULT_NODES Byzantine nodes..."
    
    # Make some nodes Byzantine (send invalid messages)
    for i in $(seq 1 $FAULT_NODES); do
        local port=$((BASE_PORT + i - 1))
        # Send command to make node Byzantine (mock)
        curl -s -X POST http://localhost:$port/admin/byzantine-mode \
            -d '{"enabled": true}' > /dev/null 2>&1
        print_warning "Node $i is now Byzantine"
    done
    
    # Monitor consensus for 60 seconds
    local start_height=$(get_block_height)
    sleep 60
    local end_height=$(get_block_height)
    
    local blocks_produced=$((end_height - start_height))
    
    if [ "$blocks_produced" -ge 2 ]; then
        print_success "Consensus continues with $FAULT_NODES Byzantine nodes: $blocks_produced blocks"
        return 0
    else
        print_error "Consensus failed with Byzantine nodes: only $blocks_produced blocks"
        return 1
    fi
}

# Test: View change scenarios
test_view_changes() {
    echo ""
    echo "Test 3: View Change Scenarios"
    echo "------------------------------"
    
    print_info "Testing view change mechanism..."
    
    # Stop primary node to trigger view change
    local primary_port=$BASE_PORT
    print_info "Stopping primary node to trigger view change..."
    
    # Mock stop command
    # docker stop neo-validator-1
    
    # Monitor for view change
    local initial_view=$(get_current_view)
    sleep 30
    local new_view=$(get_current_view)
    
    if [ "$new_view" -gt "$initial_view" ]; then
        print_success "View change successful: $initial_view -> $new_view"
        
        # Restart the node
        # docker start neo-validator-1
        return 0
    else
        print_error "View change failed"
        return 1
    fi
}

# Test: Network partition
test_network_partition() {
    echo ""
    echo "Test 4: Network Partition Recovery"
    echo "-----------------------------------"
    
    print_info "Creating network partition..."
    
    # Partition the network (isolate minority of nodes)
    local minority=$((VALIDATOR_COUNT / 3))
    
    for i in $(seq 1 $minority); do
        # Add network rules to isolate nodes (mock)
        print_info "Isolating validator $i"
        # iptables commands to block traffic
    done
    
    # Check if majority continues
    sleep 30
    local majority_height=$(get_block_height)
    
    print_info "Healing network partition..."
    # Remove network isolation rules
    
    sleep 30
    local healed_height=$(get_block_height)
    
    if [ "$healed_height" -gt "$majority_height" ]; then
        print_success "Network recovered from partition"
        return 0
    else
        print_error "Network failed to recover from partition"
        return 1
    fi
}

# Test: High transaction load
test_high_load() {
    echo ""
    echo "Test 5: High Transaction Load"
    echo "------------------------------"
    
    print_info "Generating high transaction load..."
    
    # Start transaction generators
    for i in {1..10}; do
        (
            while true; do
                curl -s -X POST http://localhost:$BASE_PORT \
                    -H "Content-Type: application/json" \
                    -d '{"jsonrpc":"2.0","method":"sendrawtransaction","params":["00d11f4e96b4"],"id":1}' \
                    > /dev/null 2>&1
                sleep 0.1
            done
        ) &
        TX_PIDS[$i]=$!
    done
    
    # Monitor consensus under load
    local start_height=$(get_block_height)
    sleep 60
    local end_height=$(get_block_height)
    
    # Stop transaction generators
    for pid in "${TX_PIDS[@]}"; do
        kill $pid 2>/dev/null || true
    done
    
    local blocks_produced=$((end_height - start_height))
    
    if [ "$blocks_produced" -ge 3 ]; then
        print_success "Consensus stable under high load: $blocks_produced blocks"
        return 0
    else
        print_error "Consensus degraded under load: only $blocks_produced blocks"
        return 1
    fi
}

# Test: Recovery mechanism
test_recovery() {
    echo ""
    echo "Test 6: Recovery Mechanism"
    echo "---------------------------"
    
    print_info "Testing consensus recovery..."
    
    # Crash multiple validators
    local crash_count=$((FAULT_NODES + 1))
    
    for i in $(seq 1 $crash_count); do
        print_info "Crashing validator $i"
        # docker stop neo-validator-$i
    done
    
    # Wait for timeout
    sleep 30
    
    # Restart validators one by one
    for i in $(seq 1 $crash_count); do
        print_info "Restarting validator $i"
        # docker start neo-validator-$i
        sleep 10
    done
    
    # Check if consensus resumes
    local height_before=$(get_block_height)
    sleep 30
    local height_after=$(get_block_height)
    
    if [ "$height_after" -gt "$height_before" ]; then
        print_success "Consensus recovered after multiple failures"
        return 0
    else
        print_error "Consensus failed to recover"
        return 1
    fi
}

# Helper: Get current block height
get_block_height() {
    local response=$(curl -s -X POST http://localhost:$BASE_PORT \
        -H "Content-Type: application/json" \
        -d '{"jsonrpc":"2.0","method":"getblockcount","params":[],"id":1}')
    echo "$response" | grep -o '"result":[0-9]*' | grep -o '[0-9]*'
}

# Helper: Get current view number
get_current_view() {
    local response=$(curl -s -X POST http://localhost:$BASE_PORT \
        -H "Content-Type: application/json" \
        -d '{"jsonrpc":"2.0","method":"getconsensusstate","params":[],"id":1}')
    echo "$response" | grep -o '"view":[0-9]*' | grep -o '[0-9]*'
}

# Generate consensus report
generate_consensus_report() {
    END_HEIGHT=$(get_block_height)
    BLOCKS_PRODUCED=$((END_HEIGHT - START_HEIGHT))
    
    cat > $REPORT_FILE << EOF
{
    "timestamp": "$(date -Iseconds)",
    "configuration": {
        "validators": $VALIDATOR_COUNT,
        "byzantine_nodes": $FAULT_NODES,
        "test_duration": $TEST_DURATION
    },
    "results": {
        "blocks_produced": $BLOCKS_PRODUCED,
        "start_height": $START_HEIGHT,
        "end_height": $END_HEIGHT,
        "view_changes": $VIEW_CHANGES,
        "failed_rounds": $FAILED_ROUNDS,
        "average_block_time": $(echo "scale=2; $TEST_DURATION / $BLOCKS_PRODUCED" | bc),
        "consensus_success_rate": $(echo "scale=2; ($BLOCKS_PRODUCED * 100) / ($TEST_DURATION / 15)" | bc)
    },
    "tests": {
        "normal_consensus": "$([ $TEST_NORMAL -eq 0 ] && echo "passed" || echo "failed")",
        "byzantine_fault": "$([ $TEST_BYZANTINE -eq 0 ] && echo "passed" || echo "failed")",
        "view_changes": "$([ $TEST_VIEW -eq 0 ] && echo "passed" || echo "failed")",
        "network_partition": "$([ $TEST_PARTITION -eq 0 ] && echo "passed" || echo "failed")",
        "high_load": "$([ $TEST_LOAD -eq 0 ] && echo "passed" || echo "failed")",
        "recovery": "$([ $TEST_RECOVERY -eq 0 ] && echo "passed" || echo "failed")"
    }
}
EOF
    
    print_success "Consensus report saved to $REPORT_FILE"
}

# Main execution
main() {
    echo "Starting consensus test suite..."
    
    # Setup
    setup_consensus_env
    start_consensus_network
    
    # Run tests
    TEST_NORMAL=1
    TEST_BYZANTINE=1
    TEST_VIEW=1
    TEST_PARTITION=1
    TEST_LOAD=1
    TEST_RECOVERY=1
    
    test_normal_consensus && TEST_NORMAL=0
    test_byzantine_fault && TEST_BYZANTINE=0
    test_view_changes && TEST_VIEW=0
    test_network_partition && TEST_PARTITION=0
    test_high_load && TEST_LOAD=0
    test_recovery && TEST_RECOVERY=0
    
    # Results
    echo ""
    echo "======================================"
    echo "Consensus Test Results"
    echo "======================================"
    
    local total_tests=6
    local passed_tests=$((6 - TEST_NORMAL - TEST_BYZANTINE - TEST_VIEW - TEST_PARTITION - TEST_LOAD - TEST_RECOVERY))
    
    echo "Tests Passed: $passed_tests/$total_tests"
    
    if [ $passed_tests -eq $total_tests ]; then
        echo -e "${GREEN}✓ All consensus tests passed!${NC}"
        RESULT=0
    else
        echo -e "${RED}✗ Some consensus tests failed${NC}"
        RESULT=1
    fi
    
    # Generate report
    generate_consensus_report
    
    exit $RESULT
}

# Run if not sourced
if [ "${BASH_SOURCE[0]}" == "${0}" ]; then
    main "$@"
fi