#!/bin/bash

# Security Quality Gate
# Enforces security standards before deployment

set -e

# Configuration
GATE_NAME="Security Gate"
THRESHOLD_CRITICAL=0
THRESHOLD_HIGH=3
THRESHOLD_MEDIUM=10
REPORT_FILE="security_gate_report.json"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "======================================"
echo "Security Quality Gate"
echo "======================================"

# Gate results
GATE_PASSED=true
FAILURES=()

# Run security audit
echo "Running security audit..."
if ! ./scripts/security_audit.sh > security_audit.log 2>&1; then
    echo -e "${YELLOW}⚠${NC} Security audit completed with findings"
fi

# Parse audit results
if [ -f "security_reports/security_audit_*.json" ]; then
    LATEST_REPORT=$(ls -t security_reports/security_audit_*.json | head -1)
    
    CRITICAL=$(jq '.summary.critical_issues' "$LATEST_REPORT")
    HIGH=$(jq '.summary.high_issues' "$LATEST_REPORT")
    MEDIUM=$(jq '.summary.medium_issues' "$LATEST_REPORT")
    
    echo ""
    echo "Security Issues Found:"
    echo "  Critical: $CRITICAL (Threshold: $THRESHOLD_CRITICAL)"
    echo "  High: $HIGH (Threshold: $THRESHOLD_HIGH)"
    echo "  Medium: $MEDIUM (Threshold: $THRESHOLD_MEDIUM)"
    echo ""
    
    # Check thresholds
    if [ "$CRITICAL" -gt "$THRESHOLD_CRITICAL" ]; then
        GATE_PASSED=false
        FAILURES+=("Critical security issues: $CRITICAL")
    fi
    
    if [ "$HIGH" -gt "$THRESHOLD_HIGH" ]; then
        GATE_PASSED=false
        FAILURES+=("High security issues exceed threshold: $HIGH > $THRESHOLD_HIGH")
    fi
    
    if [ "$MEDIUM" -gt "$THRESHOLD_MEDIUM" ]; then
        echo -e "${YELLOW}⚠${NC} Warning: Medium issues exceed threshold"
    fi
fi

# Check for secrets in code
echo "Checking for secrets in code..."
SECRET_PATTERNS=(
    "password.*=.*['\"]"
    "api[_-]?key.*=.*['\"]"
    "secret.*=.*['\"]"
    "token.*=.*['\"]"
    "BEGIN.*PRIVATE KEY"
)

for pattern in "${SECRET_PATTERNS[@]}"; do
    if grep -r "$pattern" src/ include/ 2>/dev/null | grep -v "test" > /dev/null; then
        GATE_PASSED=false
        FAILURES+=("Hardcoded secrets detected: $pattern")
    fi
done

# Check SAST results
if [ -f "sast_report.json" ]; then
    echo "Checking SAST results..."
    SAST_HIGH=$(jq '.high_severity_count' sast_report.json)
    if [ "$SAST_HIGH" -gt 0 ]; then
        GATE_PASSED=false
        FAILURES+=("SAST high severity issues: $SAST_HIGH")
    fi
fi

# Generate gate report
cat > "$REPORT_FILE" << EOF
{
    "gate": "$GATE_NAME",
    "timestamp": "$(date -Iseconds)",
    "passed": $([[ "$GATE_PASSED" == true ]] && echo "true" || echo "false"),
    "issues": {
        "critical": ${CRITICAL:-0},
        "high": ${HIGH:-0},
        "medium": ${MEDIUM:-0}
    },
    "failures": [$(printf '"%s",' "${FAILURES[@]}" | sed 's/,$//')]
}
EOF

# Results
echo ""
echo "======================================"
if [ "$GATE_PASSED" = true ]; then
    echo -e "${GREEN}✓ SECURITY GATE PASSED${NC}"
    exit 0
else
    echo -e "${RED}✗ SECURITY GATE FAILED${NC}"
    echo ""
    echo "Failures:"
    for failure in "${FAILURES[@]}"; do
        echo "  - $failure"
    done
    exit 1
fi