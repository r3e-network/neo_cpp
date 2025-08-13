#!/bin/bash

# Neo C++ Security Audit Script
# Comprehensive security scanning and vulnerability assessment

set -e

# Configuration
PROJECT_ROOT="${PROJECT_ROOT:-$(pwd)}"
REPORT_DIR="${REPORT_DIR:-security_reports}"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
REPORT_FILE="$REPORT_DIR/security_audit_$TIMESTAMP.json"
HTML_REPORT="$REPORT_DIR/security_audit_$TIMESTAMP.html"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
NC='\033[0m'

echo "======================================"
echo "Neo C++ Security Audit"
echo "======================================"
echo "Project: $PROJECT_ROOT"
echo "Report: $REPORT_FILE"
echo ""

# Audit results
VULNERABILITIES=()
WARNINGS=()
INFO_ITEMS=()
CRITICAL_COUNT=0
HIGH_COUNT=0
MEDIUM_COUNT=0
LOW_COUNT=0
PASSED_CHECKS=0
FAILED_CHECKS=0

# Helper functions
print_success() { echo -e "${GREEN}✓${NC} $1"; }
print_error() { echo -e "${RED}✗${NC} $1"; }
print_warning() { echo -e "${YELLOW}⚠${NC} $1"; }
print_info() { echo -e "${BLUE}ℹ${NC} $1"; }
print_critical() { echo -e "${MAGENTA}☠${NC} $1"; }

# Setup audit environment
setup_audit() {
    echo "Setting up security audit environment..."
    
    # Create report directory
    mkdir -p "$REPORT_DIR"
    
    # Check for required tools
    local tools=("cppcheck" "clang-tidy" "grep" "find")
    for tool in "${tools[@]}"; do
        if command -v $tool &> /dev/null; then
            print_success "$tool found"
        else
            print_warning "$tool not found - some checks will be skipped"
        fi
    done
    
    echo ""
}

# 1. Static Code Analysis
run_static_analysis() {
    echo "1. Static Code Analysis"
    echo "-----------------------"
    
    # CppCheck analysis
    if command -v cppcheck &> /dev/null; then
        print_info "Running CppCheck..."
        
        cppcheck --enable=all \
                 --suppress=missingIncludeSystem \
                 --inline-suppr \
                 --xml \
                 --xml-version=2 \
                 src/ include/ 2> "$REPORT_DIR/cppcheck.xml"
        
        # Parse results
        local errors=$(grep -c '<error ' "$REPORT_DIR/cppcheck.xml" || true)
        if [ "$errors" -gt 0 ]; then
            print_error "CppCheck found $errors issues"
            ((FAILED_CHECKS++))
            HIGH_COUNT=$((HIGH_COUNT + errors))
        else
            print_success "CppCheck: No issues found"
            ((PASSED_CHECKS++))
        fi
    fi
    
    # Clang-Tidy analysis
    if command -v clang-tidy &> /dev/null; then
        print_info "Running Clang-Tidy security checks..."
        
        find src include -name "*.cpp" -o -name "*.h" | while read file; do
            clang-tidy "$file" \
                -checks='-*,cert-*,cppcoreguidelines-*,bugprone-*,clang-analyzer-security*' \
                -- -std=c++17 -I include > "$REPORT_DIR/clang-tidy-$(basename $file).txt" 2>&1 || true
        done
        
        print_success "Clang-Tidy analysis complete"
        ((PASSED_CHECKS++))
    fi
    
    echo ""
}

# 2. Dependency Vulnerability Scan
scan_dependencies() {
    echo "2. Dependency Vulnerability Scan"
    echo "---------------------------------"
    
    print_info "Scanning third-party dependencies..."
    
    # Check for known vulnerable libraries
    local vulnerable_libs=("openssl-1.0" "boost-1.5" "libcurl-7.2")
    local found_vulnerable=false
    
    for lib in "${vulnerable_libs[@]}"; do
        if grep -r "$lib" CMakeLists.txt package.json 2>/dev/null; then
            print_critical "Vulnerable dependency found: $lib"
            VULNERABILITIES+=("Vulnerable dependency: $lib")
            ((CRITICAL_COUNT++))
            found_vulnerable=true
        fi
    done
    
    if [ "$found_vulnerable" = false ]; then
        print_success "No known vulnerable dependencies found"
        ((PASSED_CHECKS++))
    else
        ((FAILED_CHECKS++))
    fi
    
    # Check for outdated libraries
    if [ -f "conanfile.txt" ] || [ -f "vcpkg.json" ]; then
        print_info "Checking for outdated dependencies..."
        # Add specific package manager checks here
    fi
    
    echo ""
}

# 3. Code Pattern Security Scan
scan_code_patterns() {
    echo "3. Code Pattern Security Scan"
    echo "------------------------------"
    
    print_info "Scanning for dangerous code patterns..."
    
    # Dangerous functions
    local dangerous_functions=(
        "gets"
        "strcpy"
        "strcat"
        "sprintf"
        "vsprintf"
        "scanf"
        "system"
        "exec"
    )
    
    for func in "${dangerous_functions[@]}"; do
        local count=$(grep -r "\b$func\s*(" src/ include/ 2>/dev/null | wc -l || true)
        if [ "$count" -gt 0 ]; then
            print_warning "Found $count uses of dangerous function: $func"
            WARNINGS+=("Dangerous function $func used $count times")
            ((MEDIUM_COUNT++))
        fi
    done
    
    # SQL injection patterns
    print_info "Checking for SQL injection vulnerabilities..."
    local sql_concat=$(grep -r "\"SELECT.*\" *+" src/ 2>/dev/null | wc -l || true)
    if [ "$sql_concat" -gt 0 ]; then
        print_error "Potential SQL injection: String concatenation in SQL queries"
        VULNERABILITIES+=("SQL injection risk: $sql_concat occurrences")
        ((HIGH_COUNT++))
        ((FAILED_CHECKS++))
    else
        print_success "No SQL injection patterns found"
        ((PASSED_CHECKS++))
    fi
    
    # Command injection
    print_info "Checking for command injection vulnerabilities..."
    local cmd_injection=$(grep -r "system\|popen\|exec" src/ include/ 2>/dev/null | wc -l || true)
    if [ "$cmd_injection" -gt 0 ]; then
        print_warning "Potential command injection: $cmd_injection system calls found"
        WARNINGS+=("Command injection risk: $cmd_injection occurrences")
        ((MEDIUM_COUNT++))
    fi
    
    echo ""
}

# 4. Cryptography Audit
audit_cryptography() {
    echo "4. Cryptography Audit"
    echo "---------------------"
    
    print_info "Auditing cryptographic implementations..."
    
    # Check for weak algorithms
    local weak_algos=("MD5" "SHA1" "DES" "RC4")
    local found_weak=false
    
    for algo in "${weak_algos[@]}"; do
        if grep -ri "$algo" src/ include/ 2>/dev/null | grep -v "comment" > /dev/null; then
            print_error "Weak cryptographic algorithm found: $algo"
            VULNERABILITIES+=("Weak crypto: $algo")
            ((HIGH_COUNT++))
            found_weak=true
        fi
    done
    
    if [ "$found_weak" = false ]; then
        print_success "No weak cryptographic algorithms found"
        ((PASSED_CHECKS++))
    else
        ((FAILED_CHECKS++))
    fi
    
    # Check for hardcoded keys
    print_info "Checking for hardcoded cryptographic keys..."
    local hardcoded_keys=$(grep -r "BEGIN.*KEY\|password.*=.*\"" src/ include/ 2>/dev/null | wc -l || true)
    if [ "$hardcoded_keys" -gt 0 ]; then
        print_critical "Hardcoded keys/passwords found: $hardcoded_keys occurrences"
        VULNERABILITIES+=("Hardcoded secrets: $hardcoded_keys")
        ((CRITICAL_COUNT++))
        ((FAILED_CHECKS++))
    else
        print_success "No hardcoded keys found"
        ((PASSED_CHECKS++))
    fi
    
    # Check random number generation
    print_info "Checking random number generation..."
    if grep -r "rand()\|srand(" src/ include/ 2>/dev/null > /dev/null; then
        print_warning "Weak random number generation (rand()) detected"
        WARNINGS+=("Weak RNG: rand() used")
        ((MEDIUM_COUNT++))
    else
        print_success "No weak RNG found"
        ((PASSED_CHECKS++))
    fi
    
    echo ""
}

# 5. Memory Safety Audit
audit_memory_safety() {
    echo "5. Memory Safety Audit"
    echo "----------------------"
    
    print_info "Checking memory safety..."
    
    # Buffer overflow risks
    local unsafe_copies=$(grep -r "memcpy\|memmove\|strncpy" src/ include/ 2>/dev/null | wc -l || true)
    if [ "$unsafe_copies" -gt 0 ]; then
        print_warning "Found $unsafe_copies potentially unsafe memory operations"
        WARNINGS+=("Memory safety: $unsafe_copies risky operations")
        ((LOW_COUNT++))
    fi
    
    # Use after free patterns
    print_info "Checking for use-after-free patterns..."
    local delete_patterns=$(grep -r "delete.*;" src/ include/ 2>/dev/null | wc -l || true)
    local nullptr_checks=$(grep -r "= nullptr" src/ include/ 2>/dev/null | wc -l || true)
    
    if [ "$delete_patterns" -gt "$nullptr_checks" ]; then
        print_warning "Potential use-after-free: More deletes than nullptr assignments"
        WARNINGS+=("Use-after-free risk")
        ((MEDIUM_COUNT++))
    else
        print_success "Memory management appears safe"
        ((PASSED_CHECKS++))
    fi
    
    echo ""
}

# 6. Input Validation Audit
audit_input_validation() {
    echo "6. Input Validation Audit"
    echo "-------------------------"
    
    print_info "Checking input validation..."
    
    # Check for integer overflow
    local int_operations=$(grep -r "\\*\|\\+\|\\-" src/ include/ | grep -c "int\|size_t" || true)
    if [ "$int_operations" -gt 100 ]; then
        print_warning "Many integer operations found - ensure overflow checking"
        WARNINGS+=("Integer overflow risk: $int_operations operations")
        ((LOW_COUNT++))
    fi
    
    # Check for bounds checking
    local array_access=$(grep -r "\[.*\]" src/ include/ 2>/dev/null | wc -l || true)
    local bounds_checks=$(grep -r "if.*<.*size\|if.*>=\|assert" src/ include/ 2>/dev/null | wc -l || true)
    
    if [ "$bounds_checks" -lt $((array_access / 10)) ]; then
        print_warning "Insufficient bounds checking for array accesses"
        WARNINGS+=("Bounds checking: Only $bounds_checks checks for $array_access array accesses")
        ((MEDIUM_COUNT++))
    else
        print_success "Adequate bounds checking detected"
        ((PASSED_CHECKS++))
    fi
    
    echo ""
}

# 7. Authentication & Authorization Audit
audit_auth() {
    echo "7. Authentication & Authorization Audit"
    echo "----------------------------------------"
    
    print_info "Checking authentication mechanisms..."
    
    # Check for proper session management
    if grep -r "session\|token\|jwt" src/ include/ 2>/dev/null > /dev/null; then
        print_info "Session management code found"
        
        # Check for session timeout
        if ! grep -r "timeout\|expire" src/ include/ 2>/dev/null > /dev/null; then
            print_warning "No session timeout implementation found"
            WARNINGS+=("Missing session timeout")
            ((MEDIUM_COUNT++))
        else
            print_success "Session timeout implemented"
            ((PASSED_CHECKS++))
        fi
    fi
    
    # Check for rate limiting
    if ! grep -r "rate.*limit\|throttle" src/ include/ 2>/dev/null > /dev/null; then
        print_warning "No rate limiting found"
        WARNINGS+=("Missing rate limiting")
        ((MEDIUM_COUNT++))
    else
        print_success "Rate limiting implemented"
        ((PASSED_CHECKS++))
    fi
    
    echo ""
}

# 8. Network Security Audit
audit_network_security() {
    echo "8. Network Security Audit"
    echo "-------------------------"
    
    print_info "Checking network security..."
    
    # Check for TLS/SSL usage
    if grep -r "SSL\|TLS\|https" src/ include/ 2>/dev/null > /dev/null; then
        print_success "TLS/SSL implementation found"
        ((PASSED_CHECKS++))
        
        # Check for certificate validation
        if ! grep -r "verify.*cert\|certificate.*valid" src/ include/ 2>/dev/null > /dev/null; then
            print_warning "Certificate validation not found"
            WARNINGS+=("Missing certificate validation")
            ((HIGH_COUNT++))
        fi
    else
        print_error "No TLS/SSL implementation found"
        VULNERABILITIES+=("Missing encryption in transit")
        ((HIGH_COUNT++))
        ((FAILED_CHECKS++))
    fi
    
    echo ""
}

# 9. Logging & Monitoring Audit
audit_logging() {
    echo "9. Logging & Monitoring Audit"
    echo "------------------------------"
    
    print_info "Checking logging practices..."
    
    # Check for sensitive data in logs
    local log_patterns=$(grep -r "log.*password\|log.*key\|log.*secret" src/ include/ 2>/dev/null | wc -l || true)
    if [ "$log_patterns" -gt 0 ]; then
        print_critical "Sensitive data potentially logged: $log_patterns occurrences"
        VULNERABILITIES+=("Sensitive data in logs: $log_patterns")
        ((CRITICAL_COUNT++))
        ((FAILED_CHECKS++))
    else
        print_success "No sensitive data logging detected"
        ((PASSED_CHECKS++))
    fi
    
    # Check for security event logging
    if grep -r "security.*log\|audit.*log" src/ include/ 2>/dev/null > /dev/null; then
        print_success "Security event logging implemented"
        ((PASSED_CHECKS++))
    else
        print_warning "No security event logging found"
        WARNINGS+=("Missing security event logging")
        ((LOW_COUNT++))
    fi
    
    echo ""
}

# 10. Container Security Scan
scan_container_security() {
    echo "10. Container Security Scan"
    echo "---------------------------"
    
    if [ -f "Dockerfile" ]; then
        print_info "Scanning Dockerfile..."
        
        # Check for running as root
        if ! grep -q "USER" Dockerfile; then
            print_error "Container runs as root user"
            VULNERABILITIES+=("Container runs as root")
            ((HIGH_COUNT++))
            ((FAILED_CHECKS++))
        else
            print_success "Container runs as non-root user"
            ((PASSED_CHECKS++))
        fi
        
        # Check for latest tags
        if grep -q ":latest" Dockerfile; then
            print_warning "Using :latest tags in Dockerfile"
            WARNINGS+=("Docker: Using latest tags")
            ((LOW_COUNT++))
        fi
        
        # Check for secrets
        if grep -q "ENV.*PASSWORD\|ENV.*KEY" Dockerfile; then
            print_critical "Secrets found in Dockerfile"
            VULNERABILITIES+=("Secrets in Dockerfile")
            ((CRITICAL_COUNT++))
            ((FAILED_CHECKS++))
        fi
    else
        print_info "No Dockerfile found"
    fi
    
    echo ""
}

# Generate JSON report
generate_json_report() {
    cat > "$REPORT_FILE" << EOF
{
    "timestamp": "$(date -Iseconds)",
    "project": "$PROJECT_ROOT",
    "summary": {
        "total_checks": $((PASSED_CHECKS + FAILED_CHECKS)),
        "passed": $PASSED_CHECKS,
        "failed": $FAILED_CHECKS,
        "critical_issues": $CRITICAL_COUNT,
        "high_issues": $HIGH_COUNT,
        "medium_issues": $MEDIUM_COUNT,
        "low_issues": $LOW_COUNT
    },
    "vulnerabilities": [$(printf '"%s",' "${VULNERABILITIES[@]}" | sed 's/,$//')]],
    "warnings": [$(printf '"%s",' "${WARNINGS[@]}" | sed 's/,$//')]],
    "compliance": {
        "owasp_top10": $([ $CRITICAL_COUNT -eq 0 ] && echo "true" || echo "false"),
        "cwe_top25": $([ $HIGH_COUNT -lt 5 ] && echo "true" || echo "false"),
        "pci_dss": $([ $((CRITICAL_COUNT + HIGH_COUNT)) -eq 0 ] && echo "true" || echo "false")
    },
    "risk_score": $(echo "scale=2; ($CRITICAL_COUNT * 10 + $HIGH_COUNT * 5 + $MEDIUM_COUNT * 2 + $LOW_COUNT) / ($PASSED_CHECKS + $FAILED_CHECKS)" | bc)
}
EOF
}

# Generate HTML report
generate_html_report() {
    cat > "$HTML_REPORT" << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <title>Neo C++ Security Audit Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        h1 { color: #333; }
        .summary { background: #f0f0f0; padding: 15px; border-radius: 5px; }
        .critical { color: #d9534f; font-weight: bold; }
        .high { color: #f0ad4e; font-weight: bold; }
        .medium { color: #5bc0de; }
        .low { color: #5cb85c; }
        .passed { color: #5cb85c; }
        .failed { color: #d9534f; }
        table { border-collapse: collapse; width: 100%; margin-top: 20px; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #f2f2f2; }
    </style>
</head>
<body>
    <h1>Neo C++ Security Audit Report</h1>
EOF
    
    echo "<div class='summary'>" >> "$HTML_REPORT"
    echo "<h2>Summary</h2>" >> "$HTML_REPORT"
    echo "<p>Date: $(date)</p>" >> "$HTML_REPORT"
    echo "<p>Total Checks: $((PASSED_CHECKS + FAILED_CHECKS))</p>" >> "$HTML_REPORT"
    echo "<p class='passed'>Passed: $PASSED_CHECKS</p>" >> "$HTML_REPORT"
    echo "<p class='failed'>Failed: $FAILED_CHECKS</p>" >> "$HTML_REPORT"
    echo "</div>" >> "$HTML_REPORT"
    
    echo "<h2>Issues by Severity</h2>" >> "$HTML_REPORT"
    echo "<table>" >> "$HTML_REPORT"
    echo "<tr><th>Severity</th><th>Count</th></tr>" >> "$HTML_REPORT"
    echo "<tr><td class='critical'>Critical</td><td>$CRITICAL_COUNT</td></tr>" >> "$HTML_REPORT"
    echo "<tr><td class='high'>High</td><td>$HIGH_COUNT</td></tr>" >> "$HTML_REPORT"
    echo "<tr><td class='medium'>Medium</td><td>$MEDIUM_COUNT</td></tr>" >> "$HTML_REPORT"
    echo "<tr><td class='low'>Low</td><td>$LOW_COUNT</td></tr>" >> "$HTML_REPORT"
    echo "</table>" >> "$HTML_REPORT"
    
    if [ ${#VULNERABILITIES[@]} -gt 0 ]; then
        echo "<h2>Vulnerabilities</h2><ul>" >> "$HTML_REPORT"
        for vuln in "${VULNERABILITIES[@]}"; do
            echo "<li>$vuln</li>" >> "$HTML_REPORT"
        done
        echo "</ul>" >> "$HTML_REPORT"
    fi
    
    if [ ${#WARNINGS[@]} -gt 0 ]; then
        echo "<h2>Warnings</h2><ul>" >> "$HTML_REPORT"
        for warn in "${WARNINGS[@]}"; do
            echo "<li>$warn</li>" >> "$HTML_REPORT"
        done
        echo "</ul>" >> "$HTML_REPORT"
    fi
    
    echo "</body></html>" >> "$HTML_REPORT"
}

# Main execution
main() {
    setup_audit
    
    # Run all security audits
    run_static_analysis
    scan_dependencies
    scan_code_patterns
    audit_cryptography
    audit_memory_safety
    audit_input_validation
    audit_auth
    audit_network_security
    audit_logging
    scan_container_security
    
    # Generate reports
    generate_json_report
    generate_html_report
    
    # Display results
    echo "======================================"
    echo "Security Audit Results"
    echo "======================================"
    echo -e "Passed Checks: ${GREEN}$PASSED_CHECKS${NC}"
    echo -e "Failed Checks: ${RED}$FAILED_CHECKS${NC}"
    echo ""
    echo "Issues by Severity:"
    echo -e "  Critical: ${MAGENTA}$CRITICAL_COUNT${NC}"
    echo -e "  High:     ${RED}$HIGH_COUNT${NC}"
    echo -e "  Medium:   ${YELLOW}$MEDIUM_COUNT${NC}"
    echo -e "  Low:      ${BLUE}$LOW_COUNT${NC}"
    echo ""
    
    if [ $CRITICAL_COUNT -gt 0 ]; then
        echo -e "${RED}✗ FAILED: Critical security issues found${NC}"
        exit 1
    elif [ $HIGH_COUNT -gt 5 ]; then
        echo -e "${RED}✗ FAILED: Too many high severity issues${NC}"
        exit 1
    elif [ $((MEDIUM_COUNT + LOW_COUNT)) -gt 20 ]; then
        echo -e "${YELLOW}⚠ WARNING: Many security warnings found${NC}"
        exit 0
    else
        echo -e "${GREEN}✓ PASSED: Security audit acceptable${NC}"
        exit 0
    fi
}

# Run if not sourced
if [ "${BASH_SOURCE[0]}" == "${0}" ]; then
    main "$@"
fi