#!/bin/bash

# Neo C++ Network Partition Test Script
# Tests network resilience under partition scenarios

set -e

# Configuration
NODE_COUNT="${NODE_COUNT:-6}"
PARTITION_DURATION="${PARTITION_DURATION:-120}"  # 2 minutes
RECOVERY_TIME="${RECOVERY_TIME:-60}"  # 1 minute
BASE_PORT="${BASE_PORT:-10332}"
NETWORK_INTERFACE="${NETWORK_INTERFACE:-docker0}"
REPORT_FILE="partition_test_report.json"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

echo "======================================"
echo "Neo C++ Network Partition Test"
echo "======================================"
echo "Nodes: $NODE_COUNT"
echo "Partition Duration: $PARTITION_DURATION seconds"
echo "Recovery Time: $RECOVERY_TIME seconds"
echo ""

# Test results
TESTS_PASSED=0
TESTS_FAILED=0
PARTITION_SCENARIOS=()

# Helper functions
print_success() { echo -e "${GREEN}✓${NC} $1"; }
print_error() { echo -e "${RED}✗${NC} $1"; }
print_warning() { echo -e "${YELLOW}⚠${NC} $1"; }
print_info() { echo -e "${BLUE}ℹ${NC} $1"; }
print_partition() { echo -e "${CYAN}🔀${NC} $1"; }

# Check prerequisites
check_prerequisites() {
    echo "Checking prerequisites..."
    
    # Check if running with sufficient privileges for network manipulation
    if [ "$EUID" -ne 0 ]; then
        print_warning "Running without root privileges - some tests may be limited"
    fi
    
    # Check for required tools
    local tools=("iptables" "tc" "curl" "docker")
    for tool in "${tools[@]}"; do
        if command -v $tool &> /dev/null; then
            print_success "$tool found"
        else
            print_error "$tool not found"
            exit 1
        fi
    done
}

# Setup test network
setup_network() {
    echo ""
    echo "Setting up test network..."
    
    # Start clean network
    docker-compose -f deployment/docker/docker-compose.yml down 2>/dev/null || true
    docker-compose -f deployment/docker/docker-compose.yml up -d
    
    # Wait for network to stabilize
    sleep 15
    
    # Verify all nodes are connected
    local connected=true
    for i in $(seq 1 $NODE_COUNT); do
        local port=$((BASE_PORT + (i-1)*10))
        if ! curl -s http://localhost:$port/health > /dev/null 2>&1; then
            print_error "Node $i not healthy"
            connected=false
        fi
    done
    
    if [ "$connected" = true ]; then
        print_success "Test network ready"
        return 0
    else
        print_error "Failed to setup network"
        return 1
    fi
}

# Get network topology
get_network_topology() {
    echo ""
    echo "Current Network Topology:"
    echo "-------------------------"
    
    for i in $(seq 1 $NODE_COUNT); do
        local port=$((BASE_PORT + (i-1)*10))
        local response=$(curl -s -X POST http://localhost:$port \
            -H "Content-Type: application/json" \
            -d '{"jsonrpc":"2.0","method":"getpeers","params":[],"id":1}')
        
        local peer_count=$(echo "$response" | grep -o '"connected":\[[^]]*\]' | grep -o '{' | wc -l)
        echo "Node $i: $peer_count peers connected"
    done
}

# Create network partition using iptables
create_iptables_partition() {
    local group1=("$@")
    
    print_partition "Creating network partition using iptables..."
    
    # Block communication between groups
    for node1 in "${group1[@]}"; do
        for node2 in $(seq 1 $NODE_COUNT); do
            local found=false
            for g1_node in "${group1[@]}"; do
                if [ "$node2" -eq "$g1_node" ]; then
                    found=true
                    break
                fi
            done
            
            if [ "$found" = false ]; then
                # Block traffic between node1 and node2
                local ip1="172.20.0.$((10 + node1))"
                local ip2="172.20.0.$((10 + node2))"
                
                if [ "$EUID" -eq 0 ]; then
                    iptables -I DOCKER-USER -s $ip1 -d $ip2 -j DROP
                    iptables -I DOCKER-USER -s $ip2 -d $ip1 -j DROP
                    print_info "Blocked: Node $node1 <-> Node $node2"
                else
                    print_warning "Simulating block: Node $node1 <-> Node $node2"
                fi
            fi
        done
    done
}

# Remove network partition
remove_partition() {
    print_partition "Removing network partition..."
    
    if [ "$EUID" -eq 0 ]; then
        # Clear all DROP rules from DOCKER-USER chain
        iptables -F DOCKER-USER
        # Re-add default RETURN rule
        iptables -A DOCKER-USER -j RETURN
        print_success "Network partition removed"
    else
        print_warning "Simulating partition removal"
    fi
}

# Add network latency
add_network_latency() {
    local latency=$1  # in milliseconds
    local variance=$2  # variance in ms
    
    print_partition "Adding network latency: ${latency}ms ± ${variance}ms"
    
    if [ "$EUID" -eq 0 ]; then
        tc qdisc add dev $NETWORK_INTERFACE root netem delay ${latency}ms ${variance}ms
        print_success "Network latency added"
    else
        print_warning "Simulating network latency"
    fi
}

# Remove network latency
remove_network_latency() {
    print_info "Removing network latency..."
    
    if [ "$EUID" -eq 0 ]; then
        tc qdisc del dev $NETWORK_INTERFACE root netem 2>/dev/null || true
        print_success "Network latency removed"
    else
        print_warning "Simulating latency removal"
    fi
}

# Add packet loss
add_packet_loss() {
    local loss_rate=$1  # percentage
    
    print_partition "Adding packet loss: ${loss_rate}%"
    
    if [ "$EUID" -eq 0 ]; then
        tc qdisc add dev $NETWORK_INTERFACE root netem loss ${loss_rate}%
        print_success "Packet loss added"
    else
        print_warning "Simulating packet loss"
    fi
}

# Monitor partition behavior
monitor_partition() {
    local duration=$1
    local group1_size=$2
    local group2_size=$3
    
    echo ""
    echo "Monitoring partition for $duration seconds..."
    echo "Group 1: $group1_size nodes | Group 2: $group2_size nodes"
    echo ""
    
    local start_time=$(date +%s)
    local end_time=$((start_time + duration))
    
    # Track block heights for both groups
    local group1_start_height=$(get_block_height 1)
    local group2_start_height=$(get_block_height $((group1_size + 1)))
    
    while [ $(date +%s) -lt $end_time ]; do
        local elapsed=$(($(date +%s) - start_time))
        
        # Get current heights
        local group1_height=$(get_block_height 1)
        local group2_height=$(get_block_height $((group1_size + 1)))
        
        printf "\r[%3ds] Group 1: Height %d | Group 2: Height %d" \
            $elapsed $group1_height $group2_height
        
        sleep 5
    done
    
    echo ""
    
    # Calculate blocks produced
    local group1_blocks=$(($(get_block_height 1) - group1_start_height))
    local group2_blocks=$(($(get_block_height $((group1_size + 1))) - group2_start_height))
    
    echo ""
    print_info "Group 1 produced $group1_blocks blocks"
    print_info "Group 2 produced $group2_blocks blocks"
    
    # Return which group continued (if any)
    if [ $group1_blocks -gt 0 ] && [ $group2_blocks -eq 0 ]; then
        return 1  # Group 1 has consensus
    elif [ $group2_blocks -gt 0 ] && [ $group1_blocks -eq 0 ]; then
        return 2  # Group 2 has consensus
    elif [ $group1_blocks -gt 0 ] && [ $group2_blocks -gt 0 ]; then
        return 3  # Split brain
    else
        return 0  # No consensus
    fi
}

# Test: 50-50 partition
test_equal_partition() {
    echo ""
    echo "Test 1: Equal Partition (50-50)"
    echo "--------------------------------"
    
    local group1_size=$((NODE_COUNT / 2))
    local group2_size=$((NODE_COUNT - group1_size))
    
    print_info "Partitioning network: $group1_size vs $group2_size nodes"
    
    # Create partition
    local group1_nodes=($(seq 1 $group1_size))
    create_iptables_partition "${group1_nodes[@]}"
    
    # Monitor behavior
    monitor_partition $PARTITION_DURATION $group1_size $group2_size
    local result=$?
    
    # Remove partition
    remove_partition
    
    # Evaluate results
    if [ $result -eq 0 ]; then
        print_success "Expected: No consensus in either partition"
        ((TESTS_PASSED++))
        return 0
    else
        print_error "Unexpected: Consensus continued in partition"
        ((TESTS_FAILED++))
        PARTITION_SCENARIOS+=("Equal Partition")
        return 1
    fi
}

# Test: Majority-minority partition
test_majority_partition() {
    echo ""
    echo "Test 2: Majority-Minority Partition"
    echo "------------------------------------"
    
    local majority_size=$((NODE_COUNT * 2 / 3 + 1))
    local minority_size=$((NODE_COUNT - majority_size))
    
    print_info "Partitioning network: $majority_size (majority) vs $minority_size (minority) nodes"
    
    # Create partition - isolate minority
    local minority_nodes=($(seq $((majority_size + 1)) $NODE_COUNT))
    create_iptables_partition "${minority_nodes[@]}"
    
    # Monitor behavior
    monitor_partition $PARTITION_DURATION $majority_size $minority_size
    local result=$?
    
    # Remove partition
    remove_partition
    
    # Evaluate results
    if [ $result -eq 1 ]; then
        print_success "Expected: Majority partition maintains consensus"
        ((TESTS_PASSED++))
        return 0
    else
        print_error "Unexpected: Majority failed to maintain consensus"
        ((TESTS_FAILED++))
        PARTITION_SCENARIOS+=("Majority Partition")
        return 1
    fi
}

# Test: Cascading partitions
test_cascading_partitions() {
    echo ""
    echo "Test 3: Cascading Partitions"
    echo "-----------------------------"
    
    print_info "Creating cascading network partitions..."
    
    local test_passed=true
    
    # First partition: isolate 1 node
    print_partition "Stage 1: Isolating Node 1"
    create_iptables_partition 1
    sleep 30
    
    # Check if network continues
    if [ $(get_block_height 2) -gt $(get_block_height 1) ]; then
        print_success "Network continues with 1 node isolated"
    else
        print_error "Network failed with 1 node isolated"
        test_passed=false
    fi
    
    # Second partition: isolate 2 more nodes
    print_partition "Stage 2: Isolating Nodes 1-3"
    remove_partition
    create_iptables_partition 1 2 3
    sleep 30
    
    # Check consensus status
    local remaining_nodes=$((NODE_COUNT - 3))
    if [ $remaining_nodes -gt $((NODE_COUNT / 2)) ]; then
        if [ $(get_block_height 4) -gt $(get_block_height 1) ]; then
            print_success "Network continues with 3 nodes isolated"
        else
            print_error "Network failed unexpectedly"
            test_passed=false
        fi
    fi
    
    # Remove all partitions
    remove_partition
    
    if [ "$test_passed" = true ]; then
        ((TESTS_PASSED++))
        return 0
    else
        ((TESTS_FAILED++))
        PARTITION_SCENARIOS+=("Cascading Partitions")
        return 1
    fi
}

# Test: Network recovery
test_partition_recovery() {
    echo ""
    echo "Test 4: Partition Recovery"
    echo "---------------------------"
    
    print_info "Testing network recovery after partition..."
    
    # Create partition
    local group1_size=$((NODE_COUNT / 2))
    local group1_nodes=($(seq 1 $group1_size))
    create_iptables_partition "${group1_nodes[@]}"
    
    # Let partition exist for some time
    sleep $PARTITION_DURATION
    
    # Get heights before healing
    local height_before_1=$(get_block_height 1)
    local height_before_2=$(get_block_height $((group1_size + 1)))
    
    # Remove partition
    print_info "Healing network partition..."
    remove_partition
    
    # Monitor recovery
    sleep $RECOVERY_TIME
    
    # Check if all nodes converged
    local converged=true
    local final_height=$(get_block_height 1)
    
    for i in $(seq 2 $NODE_COUNT); do
        local node_height=$(get_block_height $i)
        if [ $((final_height - node_height)) -gt 1 ]; then
            converged=false
            print_error "Node $i not converged (height: $node_height vs $final_height)"
        fi
    done
    
    if [ "$converged" = true ]; then
        print_success "Network recovered and converged at height $final_height"
        ((TESTS_PASSED++))
        return 0
    else
        print_error "Network failed to converge after partition"
        ((TESTS_FAILED++))
        PARTITION_SCENARIOS+=("Partition Recovery")
        return 1
    fi
}

# Test: Network delays
test_network_delays() {
    echo ""
    echo "Test 5: High Network Latency"
    echo "-----------------------------"
    
    print_info "Testing consensus under high latency..."
    
    # Add significant latency
    add_network_latency 500 100  # 500ms ± 100ms
    
    # Monitor consensus
    local start_height=$(get_block_height 1)
    sleep 60
    local end_height=$(get_block_height 1)
    
    local blocks_produced=$((end_height - start_height))
    
    # Remove latency
    remove_network_latency
    
    if [ $blocks_produced -gt 0 ]; then
        print_success "Consensus continues under high latency: $blocks_produced blocks"
        ((TESTS_PASSED++))
        return 0
    else
        print_error "Consensus failed under high latency"
        ((TESTS_FAILED++))
        PARTITION_SCENARIOS+=("High Latency")
        return 1
    fi
}

# Test: Packet loss
test_packet_loss() {
    echo ""
    echo "Test 6: Packet Loss Scenario"
    echo "-----------------------------"
    
    print_info "Testing consensus under packet loss..."
    
    # Add packet loss
    add_packet_loss 10  # 10% packet loss
    
    # Monitor consensus
    local start_height=$(get_block_height 1)
    sleep 60
    local end_height=$(get_block_height 1)
    
    local blocks_produced=$((end_height - start_height))
    
    # Remove packet loss
    remove_network_latency  # Same command removes netem rules
    
    if [ $blocks_produced -gt 1 ]; then
        print_success "Consensus tolerates 10% packet loss: $blocks_produced blocks"
        ((TESTS_PASSED++))
        return 0
    else
        print_error "Consensus degraded under packet loss"
        ((TESTS_FAILED++))
        PARTITION_SCENARIOS+=("Packet Loss")
        return 1
    fi
}

# Helper: Get block height
get_block_height() {
    local node_index=${1:-1}
    local port=$((BASE_PORT + (node_index - 1) * 10))
    
    local response=$(curl -s -X POST http://localhost:$port \
        -H "Content-Type: application/json" \
        -d '{"jsonrpc":"2.0","method":"getblockcount","params":[],"id":1}' 2>/dev/null)
    
    if [ -z "$response" ]; then
        echo "0"
    else
        echo "$response" | grep -o '"result":[0-9]*' | grep -o '[0-9]*' || echo "0"
    fi
}

# Generate report
generate_report() {
    cat > $REPORT_FILE << EOF
{
    "timestamp": "$(date -Iseconds)",
    "configuration": {
        "node_count": $NODE_COUNT,
        "partition_duration": $PARTITION_DURATION,
        "recovery_time": $RECOVERY_TIME
    },
    "results": {
        "total_tests": $((TESTS_PASSED + TESTS_FAILED)),
        "passed": $TESTS_PASSED,
        "failed": $TESTS_FAILED,
        "success_rate": $(echo "scale=2; $TESTS_PASSED * 100 / ($TESTS_PASSED + $TESTS_FAILED)" | bc)
    },
    "failed_scenarios": [$(printf '"%s",' "${PARTITION_SCENARIOS[@]}" | sed 's/,$//')]
}
EOF
    
    print_success "Report saved to $REPORT_FILE"
}

# Cleanup
cleanup() {
    echo ""
    echo "Cleaning up..."
    
    # Remove any remaining network rules
    remove_partition
    remove_network_latency
    
    # Stop test network
    docker-compose -f deployment/docker/docker-compose.yml down
    
    print_success "Cleanup complete"
}

# Main execution
main() {
    echo "Starting network partition tests..."
    
    # Check prerequisites
    check_prerequisites
    
    # Setup network
    if ! setup_network; then
        print_error "Failed to setup network"
        exit 1
    fi
    
    # Show initial topology
    get_network_topology
    
    # Run tests
    test_equal_partition
    test_majority_partition
    test_cascading_partitions
    test_partition_recovery
    test_network_delays
    test_packet_loss
    
    # Results
    echo ""
    echo "======================================"
    echo "Network Partition Test Results"
    echo "======================================"
    echo -e "Tests Passed: ${GREEN}$TESTS_PASSED${NC}"
    echo -e "Tests Failed: ${RED}$TESTS_FAILED${NC}"
    
    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "${GREEN}✓ All partition tests passed!${NC}"
        RESULT=0
    else
        echo -e "${RED}✗ Some partition tests failed${NC}"
        echo "Failed scenarios: ${PARTITION_SCENARIOS[*]}"
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