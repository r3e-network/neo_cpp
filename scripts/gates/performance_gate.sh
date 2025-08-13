#!/bin/bash

# Performance Quality Gate
# Enforces performance standards before deployment

set -e

# Configuration
GATE_NAME="Performance Gate"
MIN_TPS=5000
MAX_LATENCY_P95=100  # milliseconds
MAX_CPU_USAGE=70     # percentage
MAX_MEMORY_USAGE=2048 # MB
REPORT_FILE="performance_gate_report.json"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "======================================"
echo "Performance Quality Gate"
echo "======================================"

# Gate results
GATE_PASSED=true
FAILURES=()
WARNINGS=()

# Run performance tests
echo "Running performance tests..."
if ! ./scripts/performance_test.sh > performance_test.log 2>&1; then
    echo -e "${YELLOW}⚠${NC} Performance tests completed with issues"
fi

# Parse performance results
if [ -f "performance_reports/perf_report_*.json" ]; then
    LATEST_REPORT=$(ls -t performance_reports/perf_report_*.json | head -1)
    
    # Extract metrics
    AVG_TPS=$(jq '.results.throughput.average_tps' "$LATEST_REPORT")
    P95_LATENCY=$(jq '.results.latency.p95' "$LATEST_REPORT")
    CPU_AVG=$(jq '.results.resources.cpu_average' "$LATEST_REPORT")
    MEM_AVG=$(jq '.results.resources.memory_average' "$LATEST_REPORT")
    
    echo ""
    echo "Performance Metrics:"
    echo "  Average TPS: $AVG_TPS (Required: >=$MIN_TPS)"
    echo "  P95 Latency: $P95_LATENCY ms (Required: <=$MAX_LATENCY_P95 ms)"
    echo "  CPU Usage: $CPU_AVG% (Required: <=$MAX_CPU_USAGE%)"
    echo "  Memory Usage: $MEM_AVG MB (Required: <=$MAX_MEMORY_USAGE MB)"
    echo ""
    
    # Check TPS threshold
    if (( $(echo "$AVG_TPS < $MIN_TPS" | bc -l) )); then
        GATE_PASSED=false
        FAILURES+=("TPS below minimum: $AVG_TPS < $MIN_TPS")
    fi
    
    # Check latency threshold
    if (( $(echo "$P95_LATENCY > $MAX_LATENCY_P95" | bc -l) )); then
        GATE_PASSED=false
        FAILURES+=("P95 latency too high: $P95_LATENCY ms > $MAX_LATENCY_P95 ms")
    fi
    
    # Check CPU usage
    if (( $(echo "$CPU_AVG > $MAX_CPU_USAGE" | bc -l) )); then
        GATE_PASSED=false
        FAILURES+=("CPU usage too high: $CPU_AVG% > $MAX_CPU_USAGE%")
    fi
    
    # Check memory usage
    if (( $(echo "$MEM_AVG > $MAX_MEMORY_USAGE" | bc -l) )); then
        WARNINGS+=("Memory usage high: $MEM_AVG MB > $MAX_MEMORY_USAGE MB")
    fi
else
    GATE_PASSED=false
    FAILURES+=("No performance test results found")
fi

# Run TPS test
echo "Running TPS validation test..."
if ./scripts/tps_test.sh > tps_test.log 2>&1; then
    echo -e "${GREEN}✓${NC} TPS test passed"
else
    GATE_PASSED=false
    FAILURES+=("TPS validation test failed")
fi

# Check for performance regression
if [ -f "performance_baseline.json" ]; then
    echo "Checking for performance regression..."
    
    BASELINE_TPS=$(jq '.tps' performance_baseline.json)
    BASELINE_LATENCY=$(jq '.latency_p95' performance_baseline.json)
    
    # Allow 10% regression
    MIN_ACCEPTABLE_TPS=$(echo "scale=2; $BASELINE_TPS * 0.9" | bc)
    MAX_ACCEPTABLE_LATENCY=$(echo "scale=2; $BASELINE_LATENCY * 1.1" | bc)
    
    if (( $(echo "$AVG_TPS < $MIN_ACCEPTABLE_TPS" | bc -l) )); then
        WARNINGS+=("Performance regression detected: TPS dropped >10%")
    fi
    
    if (( $(echo "$P95_LATENCY > $MAX_ACCEPTABLE_LATENCY" | bc -l) )); then
        WARNINGS+=("Performance regression detected: Latency increased >10%")
    fi
fi

# Generate gate report
cat > "$REPORT_FILE" << EOF
{
    "gate": "$GATE_NAME",
    "timestamp": "$(date -Iseconds)",
    "passed": $([[ "$GATE_PASSED" == true ]] && echo "true" || echo "false"),
    "metrics": {
        "tps": ${AVG_TPS:-0},
        "latency_p95": ${P95_LATENCY:-0},
        "cpu_usage": ${CPU_AVG:-0},
        "memory_usage": ${MEM_AVG:-0}
    },
    "thresholds": {
        "min_tps": $MIN_TPS,
        "max_latency_p95": $MAX_LATENCY_P95,
        "max_cpu_usage": $MAX_CPU_USAGE,
        "max_memory_usage": $MAX_MEMORY_USAGE
    },
    "failures": [$(printf '"%s",' "${FAILURES[@]}" | sed 's/,$//')]],
    "warnings": [$(printf '"%s",' "${WARNINGS[@]}" | sed 's/,$//')]]
}
EOF

# Results
echo ""
echo "======================================"
if [ "$GATE_PASSED" = true ]; then
    echo -e "${GREEN}✓ PERFORMANCE GATE PASSED${NC}"
    
    if [ ${#WARNINGS[@]} -gt 0 ]; then
        echo ""
        echo "Warnings:"
        for warning in "${WARNINGS[@]}"; do
            echo -e "  ${YELLOW}⚠${NC} $warning"
        done
    fi
    
    exit 0
else
    echo -e "${RED}✗ PERFORMANCE GATE FAILED${NC}"
    echo ""
    echo "Failures:"
    for failure in "${FAILURES[@]}"; do
        echo "  - $failure"
    done
    
    if [ ${#WARNINGS[@]} -gt 0 ]; then
        echo ""
        echo "Warnings:"
        for warning in "${WARNINGS[@]}"; do
            echo -e "  ${YELLOW}⚠${NC} $warning"
        done
    fi
    
    exit 1
fi