#!/bin/bash

# Neo C++ Performance Test Suite
# Comprehensive performance testing and benchmarking

set -e

# Configuration
TEST_DURATION="${TEST_DURATION:-300}"  # 5 minutes
WARMUP_TIME="${WARMUP_TIME:-30}"      # 30 seconds warmup
CONCURRENT_USERS="${CONCURRENT_USERS:-100}"
TARGET_TPS="${TARGET_TPS:-5000}"
TARGET_LATENCY="${TARGET_LATENCY:-100}"  # milliseconds
NODE_URL="${NODE_URL:-http://localhost:10332}"
REPORT_DIR="${REPORT_DIR:-performance_reports}"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
REPORT_FILE="$REPORT_DIR/perf_report_$TIMESTAMP.json"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

echo "======================================"
echo "Neo C++ Performance Test Suite"
echo "======================================"
echo "Duration: $TEST_DURATION seconds"
echo "Concurrent Users: $CONCURRENT_USERS"
echo "Target TPS: $TARGET_TPS"
echo "Target Latency: <$TARGET_LATENCY ms"
echo ""

# Metrics storage
declare -A METRICS
METRICS[tps_max]=0
METRICS[tps_min]=999999
METRICS[tps_avg]=0
METRICS[latency_p50]=0
METRICS[latency_p95]=0
METRICS[latency_p99]=0
METRICS[cpu_avg]=0
METRICS[memory_avg]=0
METRICS[errors]=0
METRICS[total_requests]=0

# Helper functions
print_success() { echo -e "${GREEN}✓${NC} $1"; }
print_error() { echo -e "${RED}✗${NC} $1"; }
print_warning() { echo -e "${YELLOW}⚠${NC} $1"; }
print_info() { echo -e "${BLUE}ℹ${NC} $1"; }
print_metric() { echo -e "${CYAN}📊${NC} $1"; }

# Setup performance test environment
setup_perf_env() {
    echo "Setting up performance test environment..."
    
    # Create report directory
    mkdir -p "$REPORT_DIR"
    mkdir -p "$REPORT_DIR/raw_data"
    
    # Check tools
    local required_tools=("curl" "bc" "awk" "jq")
    for tool in "${required_tools[@]}"; do
        if ! command -v $tool &> /dev/null; then
            print_error "$tool is required but not installed"
            exit 1
        fi
    done
    
    # Check if node is running
    if ! curl -s "$NODE_URL/health" > /dev/null 2>&1; then
        print_error "Node is not responding at $NODE_URL"
        exit 1
    fi
    
    print_success "Performance test environment ready"
    echo ""
}

# Warmup phase
run_warmup() {
    echo "Running warmup phase ($WARMUP_TIME seconds)..."
    
    local end_time=$(($(date +%s) + WARMUP_TIME))
    local count=0
    
    while [ $(date +%s) -lt $end_time ]; do
        # Send warmup requests
        for i in {1..10}; do
            curl -s -X POST "$NODE_URL" \
                -H "Content-Type: application/json" \
                -d '{"jsonrpc":"2.0","method":"getblockcount","params":[],"id":1}' \
                > /dev/null 2>&1 &
        done
        
        count=$((count + 10))
        printf "\rWarmup requests sent: %d" $count
        sleep 1
    done
    
    echo ""
    print_success "Warmup complete"
    echo ""
}

# 1. Throughput Test
test_throughput() {
    echo "1. Throughput Test"
    echo "------------------"
    
    print_info "Testing maximum transaction throughput..."
    
    local test_file="$REPORT_DIR/raw_data/throughput_$TIMESTAMP.csv"
    echo "timestamp,tps,errors" > "$test_file"
    
    local start_time=$(date +%s)
    local end_time=$((start_time + TEST_DURATION))
    local total_tx=0
    local errors=0
    local measurements=()
    
    while [ $(date +%s) -lt $end_time ]; do
        local second_start=$(date +%s.%N)
        local second_tx=0
        local second_errors=0
        
        # Send batch of transactions
        for i in $(seq 1 100); do
            {
                if curl -s -X POST "$NODE_URL" \
                    -H "Content-Type: application/json" \
                    -d '{"jsonrpc":"2.0","method":"sendrawtransaction","params":["00d11f4e96b4"],"id":1}' \
                    --max-time 1 > /dev/null 2>&1; then
                    ((second_tx++))
                else
                    ((second_errors++))
                fi
            } &
        done
        
        wait
        
        # Calculate TPS for this second
        local second_end=$(date +%s.%N)
        local duration=$(echo "$second_end - $second_start" | bc)
        local tps=$(echo "scale=2; $second_tx / $duration" | bc)
        
        # Store metrics
        measurements+=($tps)
        total_tx=$((total_tx + second_tx))
        errors=$((errors + second_errors))
        
        # Log to file
        echo "$(date +%s),$tps,$second_errors" >> "$test_file"
        
        # Update min/max
        if (( $(echo "$tps > ${METRICS[tps_max]}" | bc -l) )); then
            METRICS[tps_max]=$tps
        fi
        if (( $(echo "$tps < ${METRICS[tps_min]}" | bc -l) )); then
            METRICS[tps_min]=$tps
        fi
        
        printf "\rCurrent TPS: %6.2f | Total TX: %d | Errors: %d" $tps $total_tx $errors
    done
    
    echo ""
    
    # Calculate average TPS
    local sum=0
    for tps in "${measurements[@]}"; do
        sum=$(echo "$sum + $tps" | bc)
    done
    METRICS[tps_avg]=$(echo "scale=2; $sum / ${#measurements[@]}" | bc)
    METRICS[total_requests]=$total_tx
    METRICS[errors]=$errors
    
    print_metric "Average TPS: ${METRICS[tps_avg]}"
    print_metric "Max TPS: ${METRICS[tps_max]}"
    print_metric "Min TPS: ${METRICS[tps_min]}"
    
    if (( $(echo "${METRICS[tps_avg]} >= $TARGET_TPS" | bc -l) )); then
        print_success "Throughput test PASSED (${METRICS[tps_avg]} >= $TARGET_TPS)"
    else
        print_error "Throughput test FAILED (${METRICS[tps_avg]} < $TARGET_TPS)"
    fi
    
    echo ""
}

# 2. Latency Test
test_latency() {
    echo "2. Latency Test"
    echo "---------------"
    
    print_info "Testing response latency distribution..."
    
    local test_file="$REPORT_DIR/raw_data/latency_$TIMESTAMP.csv"
    echo "timestamp,latency_ms,status" > "$test_file"
    
    local latencies=()
    local test_duration=60  # 1 minute for latency test
    local end_time=$(($(date +%s) + test_duration))
    
    while [ $(date +%s) -lt $end_time ]; do
        # Measure single request latency
        local start=$(date +%s%N)
        local status="success"
        
        if curl -s -X POST "$NODE_URL" \
            -H "Content-Type: application/json" \
            -d '{"jsonrpc":"2.0","method":"getblockcount","params":[],"id":1}' \
            --max-time 5 > /dev/null 2>&1; then
            local end=$(date +%s%N)
            local latency=$(echo "scale=2; ($end - $start) / 1000000" | bc)
            latencies+=($latency)
            echo "$(date +%s),$latency,$status" >> "$test_file"
        else
            status="timeout"
            echo "$(date +%s),5000,$status" >> "$test_file"
        fi
        
        sleep 0.1  # 10 requests per second
    done
    
    # Sort latencies for percentile calculation
    IFS=$'\n' sorted=($(sort -n <<<"${latencies[*]}"))
    unset IFS
    
    local count=${#sorted[@]}
    if [ $count -gt 0 ]; then
        # Calculate percentiles
        local p50_idx=$(echo "scale=0; $count * 0.50 / 1" | bc)
        local p95_idx=$(echo "scale=0; $count * 0.95 / 1" | bc)
        local p99_idx=$(echo "scale=0; $count * 0.99 / 1" | bc)
        
        METRICS[latency_p50]=${sorted[$p50_idx]}
        METRICS[latency_p95]=${sorted[$p95_idx]}
        METRICS[latency_p99]=${sorted[$p99_idx]}
        
        print_metric "P50 Latency: ${METRICS[latency_p50]} ms"
        print_metric "P95 Latency: ${METRICS[latency_p95]} ms"
        print_metric "P99 Latency: ${METRICS[latency_p99]} ms"
        
        if (( $(echo "${METRICS[latency_p95]} <= $TARGET_LATENCY" | bc -l) )); then
            print_success "Latency test PASSED (P95: ${METRICS[latency_p95]} <= $TARGET_LATENCY ms)"
        else
            print_error "Latency test FAILED (P95: ${METRICS[latency_p95]} > $TARGET_LATENCY ms)"
        fi
    else
        print_error "No latency measurements collected"
    fi
    
    echo ""
}

# 3. Concurrent Users Test
test_concurrent_users() {
    echo "3. Concurrent Users Test"
    echo "------------------------"
    
    print_info "Testing with $CONCURRENT_USERS concurrent users..."
    
    local test_file="$REPORT_DIR/raw_data/concurrent_$TIMESTAMP.csv"
    echo "user_id,requests,errors,avg_latency" > "$test_file"
    
    # Start concurrent user simulations
    for user in $(seq 1 $CONCURRENT_USERS); do
        {
            local user_requests=0
            local user_errors=0
            local user_latencies=()
            local user_end=$(($(date +%s) + 30))  # Each user runs for 30 seconds
            
            while [ $(date +%s) -lt $user_end ]; do
                local start=$(date +%s%N)
                
                if curl -s -X POST "$NODE_URL" \
                    -H "Content-Type: application/json" \
                    -d "{\"jsonrpc\":\"2.0\",\"method\":\"getaccount\",\"params\":[\"user$user\"],\"id\":1}" \
                    --max-time 2 > /dev/null 2>&1; then
                    local end=$(date +%s%N)
                    local latency=$(echo "scale=2; ($end - $start) / 1000000" | bc)
                    user_latencies+=($latency)
                    ((user_requests++))
                else
                    ((user_errors++))
                fi
                
                sleep 0.5  # 2 requests per second per user
            done
            
            # Calculate user average latency
            local sum=0
            for lat in "${user_latencies[@]}"; do
                sum=$(echo "$sum + $lat" | bc)
            done
            local avg_latency=0
            if [ ${#user_latencies[@]} -gt 0 ]; then
                avg_latency=$(echo "scale=2; $sum / ${#user_latencies[@]}" | bc)
            fi
            
            echo "$user,$user_requests,$user_errors,$avg_latency" >> "$test_file"
        } &
        
        # Limit parallel processes
        if [ $((user % 10)) -eq 0 ]; then
            wait
            printf "\rUsers started: %d/%d" $user $CONCURRENT_USERS
        fi
    done
    
    wait
    echo ""
    
    # Analyze results
    local total_requests=$(awk -F',' '{sum+=$2} END {print sum}' "$test_file")
    local total_errors=$(awk -F',' '{sum+=$3} END {print sum}' "$test_file")
    local error_rate=$(echo "scale=2; $total_errors * 100 / $total_requests" | bc)
    
    print_metric "Total Requests: $total_requests"
    print_metric "Total Errors: $total_errors"
    print_metric "Error Rate: $error_rate%"
    
    if (( $(echo "$error_rate <= 1" | bc -l) )); then
        print_success "Concurrent users test PASSED (Error rate: $error_rate% <= 1%)"
    else
        print_error "Concurrent users test FAILED (Error rate: $error_rate% > 1%)"
    fi
    
    echo ""
}

# 4. Stress Test
test_stress() {
    echo "4. Stress Test"
    echo "--------------"
    
    print_info "Running stress test to find breaking point..."
    
    local test_file="$REPORT_DIR/raw_data/stress_$TIMESTAMP.csv"
    echo "load_level,tps,error_rate,latency_p95" > "$test_file"
    
    local load_levels=(100 500 1000 2000 5000 10000)
    local breaking_point=0
    
    for load in "${load_levels[@]}"; do
        print_info "Testing with load level: $load requests/second"
        
        local errors=0
        local successes=0
        local latencies=()
        local test_duration=10
        local end_time=$(($(date +%s) + test_duration))
        
        while [ $(date +%s) -lt $end_time ]; do
            # Send burst of requests
            for i in $(seq 1 $((load/100))); do
                {
                    local start=$(date +%s%N)
                    if curl -s -X POST "$NODE_URL" \
                        -H "Content-Type: application/json" \
                        -d '{"jsonrpc":"2.0","method":"getblockcount","params":[],"id":1}' \
                        --max-time 1 > /dev/null 2>&1; then
                        local end=$(date +%s%N)
                        local latency=$(echo "scale=2; ($end - $start) / 1000000" | bc)
                        latencies+=($latency)
                        ((successes++))
                    else
                        ((errors++))
                    fi
                } &
            done
            
            # Don't overwhelm the system
            sleep 0.01
        done
        
        wait
        
        # Calculate metrics for this load level
        local total=$((successes + errors))
        local error_rate=$(echo "scale=2; $errors * 100 / $total" | bc)
        local tps=$(echo "scale=2; $successes / $test_duration" | bc)
        
        # Calculate P95 latency
        IFS=$'\n' sorted=($(sort -n <<<"${latencies[*]}"))
        unset IFS
        local p95_idx=$(echo "scale=0; ${#sorted[@]} * 0.95 / 1" | bc)
        local p95_latency=0
        if [ ${#sorted[@]} -gt 0 ]; then
            p95_latency=${sorted[$p95_idx]:-0}
        fi
        
        echo "$load,$tps,$error_rate,$p95_latency" >> "$test_file"
        
        print_metric "Load: $load | TPS: $tps | Error Rate: $error_rate% | P95: $p95_latency ms"
        
        # Check if system is breaking
        if (( $(echo "$error_rate > 10" | bc -l) )) || (( $(echo "$p95_latency > 1000" | bc -l) )); then
            breaking_point=$load
            print_warning "System breaking point detected at load level: $load"
            break
        fi
    done
    
    if [ $breaking_point -gt 0 ]; then
        print_metric "Breaking point: $breaking_point requests/second"
        if [ $breaking_point -ge 5000 ]; then
            print_success "Stress test PASSED (Breaking point >= 5000 req/s)"
        else
            print_error "Stress test FAILED (Breaking point < 5000 req/s)"
        fi
    else
        print_success "System handled maximum load level without breaking"
    fi
    
    echo ""
}

# 5. Resource Usage Test
test_resource_usage() {
    echo "5. Resource Usage Test"
    echo "----------------------"
    
    print_info "Monitoring resource usage during load..."
    
    local test_file="$REPORT_DIR/raw_data/resources_$TIMESTAMP.csv"
    echo "timestamp,cpu_percent,memory_mb,disk_io_kb" > "$test_file"
    
    # Start load generation in background
    {
        while true; do
            for i in {1..50}; do
                curl -s -X POST "$NODE_URL" \
                    -H "Content-Type: application/json" \
                    -d '{"jsonrpc":"2.0","method":"getblockcount","params":[],"id":1}' \
                    > /dev/null 2>&1 &
            done
            sleep 1
        done
    } &
    local load_pid=$!
    
    # Monitor resources for 60 seconds
    local end_time=$(($(date +%s) + 60))
    local cpu_measurements=()
    local mem_measurements=()
    
    while [ $(date +%s) -lt $end_time ]; do
        # Get container stats (assuming Docker)
        if command -v docker &> /dev/null; then
            local stats=$(docker stats --no-stream --format "{{.CPUPerc}},{{.MemUsage}}" neo-node-1 2>/dev/null | head -1)
            local cpu=$(echo "$stats" | cut -d',' -f1 | tr -d '%')
            local mem=$(echo "$stats" | cut -d',' -f2 | cut -d'/' -f1 | tr -d 'MiB' | tr -d ' ')
            
            if [ -n "$cpu" ] && [ -n "$mem" ]; then
                cpu_measurements+=($cpu)
                mem_measurements+=($mem)
                echo "$(date +%s),$cpu,$mem,0" >> "$test_file"
            fi
        fi
        
        printf "\rMonitoring resources... %d seconds remaining" $((end_time - $(date +%s)))
        sleep 2
    done
    
    # Stop load generation
    kill $load_pid 2>/dev/null || true
    wait $load_pid 2>/dev/null || true
    
    echo ""
    
    # Calculate averages
    if [ ${#cpu_measurements[@]} -gt 0 ]; then
        local cpu_sum=0
        for cpu in "${cpu_measurements[@]}"; do
            cpu_sum=$(echo "$cpu_sum + $cpu" | bc)
        done
        METRICS[cpu_avg]=$(echo "scale=2; $cpu_sum / ${#cpu_measurements[@]}" | bc)
    fi
    
    if [ ${#mem_measurements[@]} -gt 0 ]; then
        local mem_sum=0
        for mem in "${mem_measurements[@]}"; do
            mem_sum=$(echo "$mem_sum + $mem" | bc)
        done
        METRICS[memory_avg]=$(echo "scale=2; $mem_sum / ${#mem_measurements[@]}" | bc)
    fi
    
    print_metric "Average CPU Usage: ${METRICS[cpu_avg]}%"
    print_metric "Average Memory Usage: ${METRICS[memory_avg]} MB"
    
    if (( $(echo "${METRICS[cpu_avg]} <= 70" | bc -l) )) && (( $(echo "${METRICS[memory_avg]} <= 2048" | bc -l) )); then
        print_success "Resource usage test PASSED"
    else
        print_error "Resource usage test FAILED (CPU or Memory too high)"
    fi
    
    echo ""
}

# 6. Endurance Test
test_endurance() {
    echo "6. Endurance Test"
    echo "-----------------"
    
    print_info "Running endurance test for extended period..."
    
    local test_file="$REPORT_DIR/raw_data/endurance_$TIMESTAMP.csv"
    echo "minute,tps,errors,memory_mb" > "$test_file"
    
    local duration_minutes=5
    local baseline_memory=0
    
    for minute in $(seq 1 $duration_minutes); do
        print_info "Minute $minute/$duration_minutes"
        
        local minute_tx=0
        local minute_errors=0
        local end_time=$(($(date +%s) + 60))
        
        while [ $(date +%s) -lt $end_time ]; do
            # Send steady load
            for i in {1..10}; do
                if curl -s -X POST "$NODE_URL" \
                    -H "Content-Type: application/json" \
                    -d '{"jsonrpc":"2.0","method":"getblockcount","params":[],"id":1}' \
                    --max-time 1 > /dev/null 2>&1; then
                    ((minute_tx++))
                else
                    ((minute_errors++))
                fi
            done
            sleep 0.1
        done
        
        # Get memory usage
        local memory=0
        if command -v docker &> /dev/null; then
            memory=$(docker stats --no-stream --format "{{.MemUsage}}" neo-node-1 2>/dev/null | head -1 | cut -d'/' -f1 | tr -d 'MiB' | tr -d ' ')
        fi
        
        if [ $minute -eq 1 ]; then
            baseline_memory=$memory
        fi
        
        local tps=$(echo "scale=2; $minute_tx / 60" | bc)
        echo "$minute,$tps,$minute_errors,$memory" >> "$test_file"
        
        print_metric "Minute $minute: TPS=$tps, Errors=$minute_errors, Memory=${memory}MB"
    done
    
    # Check for memory leaks
    local final_memory=$(tail -1 "$test_file" | cut -d',' -f4)
    local memory_increase=$(echo "scale=2; ($final_memory - $baseline_memory) / $baseline_memory * 100" | bc)
    
    print_metric "Memory increase: $memory_increase%"
    
    if (( $(echo "$memory_increase <= 10" | bc -l) )); then
        print_success "Endurance test PASSED (No significant memory leak)"
    else
        print_warning "Endurance test WARNING (Memory increased by $memory_increase%)"
    fi
    
    echo ""
}

# Generate performance report
generate_performance_report() {
    cat > "$REPORT_FILE" << EOF
{
    "timestamp": "$(date -Iseconds)",
    "configuration": {
        "test_duration": $TEST_DURATION,
        "concurrent_users": $CONCURRENT_USERS,
        "target_tps": $TARGET_TPS,
        "target_latency": $TARGET_LATENCY,
        "node_url": "$NODE_URL"
    },
    "results": {
        "throughput": {
            "average_tps": ${METRICS[tps_avg]},
            "max_tps": ${METRICS[tps_max]},
            "min_tps": ${METRICS[tps_min]},
            "total_requests": ${METRICS[total_requests]},
            "errors": ${METRICS[errors]}
        },
        "latency": {
            "p50": ${METRICS[latency_p50]},
            "p95": ${METRICS[latency_p95]},
            "p99": ${METRICS[latency_p99]}
        },
        "resources": {
            "cpu_average": ${METRICS[cpu_avg]},
            "memory_average": ${METRICS[memory_avg]}
        }
    },
    "tests_passed": {
        "throughput": $([ $(echo "${METRICS[tps_avg]} >= $TARGET_TPS" | bc) -eq 1 ] && echo "true" || echo "false"),
        "latency": $([ $(echo "${METRICS[latency_p95]} <= $TARGET_LATENCY" | bc) -eq 1 ] && echo "true" || echo "false"),
        "resources": $([ $(echo "${METRICS[cpu_avg]} <= 70" | bc) -eq 1 ] && echo "true" || echo "false")
    },
    "performance_score": $(echo "scale=2; (${METRICS[tps_avg]} / $TARGET_TPS * 40 + (1 - ${METRICS[latency_p95]} / $TARGET_LATENCY) * 40 + (1 - ${METRICS[cpu_avg]} / 100) * 20)" | bc)
}
EOF
    
    print_success "Performance report saved to $REPORT_FILE"
}

# Main execution
main() {
    echo "Starting Neo C++ Performance Test Suite..."
    echo ""
    
    # Setup
    setup_perf_env
    
    # Warmup
    run_warmup
    
    # Run performance tests
    test_throughput
    test_latency
    test_concurrent_users
    test_stress
    test_resource_usage
    test_endurance
    
    # Generate report
    generate_performance_report
    
    # Summary
    echo "======================================"
    echo "Performance Test Summary"
    echo "======================================"
    echo -e "Average TPS: ${CYAN}${METRICS[tps_avg]}${NC} (Target: $TARGET_TPS)"
    echo -e "P95 Latency: ${CYAN}${METRICS[latency_p95]} ms${NC} (Target: <$TARGET_LATENCY ms)"
    echo -e "CPU Usage: ${CYAN}${METRICS[cpu_avg]}%${NC}"
    echo -e "Memory Usage: ${CYAN}${METRICS[memory_avg]} MB${NC}"
    echo ""
    
    # Overall pass/fail
    if [ $(echo "${METRICS[tps_avg]} >= $TARGET_TPS" | bc) -eq 1 ] && \
       [ $(echo "${METRICS[latency_p95]} <= $TARGET_LATENCY" | bc) -eq 1 ]; then
        echo -e "${GREEN}✓ PERFORMANCE TESTS PASSED${NC}"
        exit 0
    else
        echo -e "${RED}✗ PERFORMANCE TESTS FAILED${NC}"
        exit 1
    fi
}

# Run if not sourced
if [ "${BASH_SOURCE[0]}" == "${0}" ]; then
    main "$@"
fi