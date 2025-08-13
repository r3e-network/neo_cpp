#!/bin/bash

# validate_infrastructure.sh - Comprehensive infrastructure validation
# This script validates all infrastructure components are properly configured

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Counters
TOTAL_CHECKS=0
PASSED_CHECKS=0
FAILED_CHECKS=0
WARNINGS=0

# Function to print colored output
print_status() {
    local status=$1
    local message=$2
    TOTAL_CHECKS=$((TOTAL_CHECKS + 1))
    
    case $status in
        PASS)
            echo -e "${GREEN}✅ PASS:${NC} $message"
            PASSED_CHECKS=$((PASSED_CHECKS + 1))
            ;;
        FAIL)
            echo -e "${RED}❌ FAIL:${NC} $message"
            FAILED_CHECKS=$((FAILED_CHECKS + 1))
            ;;
        WARN)
            echo -e "${YELLOW}⚠️  WARN:${NC} $message"
            WARNINGS=$((WARNINGS + 1))
            ;;
        INFO)
            echo -e "${BLUE}ℹ️  INFO:${NC} $message"
            ;;
    esac
}

print_section() {
    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "$1"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
}

# Start validation
echo "======================================"
echo "   NEO C++ INFRASTRUCTURE VALIDATION"
echo "======================================"
echo "Date: $(date)"
echo "Directory: $(pwd)"
echo ""

# 1. Check Git Repository
print_section "1. GIT REPOSITORY"
if [ -d .git ]; then
    print_status "PASS" "Git repository found"
    
    # Check submodules
    if [ -f .gitmodules ]; then
        print_status "PASS" ".gitmodules file exists"
        
        # Check if googletest is initialized
        if [ -d third_party/googletest/.git ]; then
            print_status "PASS" "GoogleTest submodule initialized"
        else
            print_status "WARN" "GoogleTest submodule not initialized (run: git submodule update --init --recursive)"
        fi
    else
        print_status "WARN" "No .gitmodules file found"
    fi
else
    print_status "FAIL" "Not a git repository"
fi

# 2. Check Build System
print_section "2. BUILD SYSTEM"
if [ -f CMakeLists.txt ]; then
    print_status "PASS" "Root CMakeLists.txt found"
    
    # Check if build directory exists
    if [ -d build ]; then
        print_status "INFO" "Build directory exists"
    else
        print_status "INFO" "Build directory not found (will be created during build)"
    fi
    
    # Check Makefile
    if [ -f Makefile ]; then
        print_status "PASS" "Makefile found"
    else
        print_status "WARN" "Makefile not found"
    fi
else
    print_status "FAIL" "CMakeLists.txt not found"
fi

# 3. Check Scripts
print_section "3. AUTOMATION SCRIPTS"
SCRIPT_DIR="scripts"
if [ -d "$SCRIPT_DIR" ]; then
    print_status "PASS" "Scripts directory found"
    
    # Check critical scripts
    critical_scripts=(
        "setup_project.sh"
        "integration_test.sh"
        "consensus_test.sh"
        "partition_test.sh"
        "performance_test.sh"
        "security_audit.sh"
        "backup_restore.sh"
    )
    
    for script in "${critical_scripts[@]}"; do
        if [ -f "$SCRIPT_DIR/$script" ]; then
            if bash -n "$SCRIPT_DIR/$script" 2>/dev/null; then
                print_status "PASS" "$script - syntax valid"
            else
                print_status "FAIL" "$script - syntax error"
            fi
        else
            print_status "WARN" "$script - not found"
        fi
    done
    
    # Check deployment scripts
    if [ -d "$SCRIPT_DIR/deployment" ]; then
        if [ -f "$SCRIPT_DIR/deployment/deploy.sh" ]; then
            print_status "PASS" "Deployment script found"
        else
            print_status "WARN" "deploy.sh not found in deployment directory"
        fi
    fi
    
    # Check gate scripts
    if [ -d "$SCRIPT_DIR/gates" ]; then
        print_status "PASS" "Quality gates directory found"
        for gate in "$SCRIPT_DIR/gates"/*.sh; do
            if [ -f "$gate" ]; then
                if bash -n "$gate" 2>/dev/null; then
                    print_status "PASS" "$(basename $gate) - syntax valid"
                else
                    print_status "FAIL" "$(basename $gate) - syntax error"
                fi
            fi
        done
    else
        print_status "WARN" "Quality gates directory not found"
    fi
else
    print_status "FAIL" "Scripts directory not found"
fi

# 4. Check Docker Configuration
print_section "4. DOCKER CONFIGURATION"
if [ -f Dockerfile ]; then
    print_status "PASS" "Dockerfile found in root"
elif [ -f deployment/docker/Dockerfile ]; then
    print_status "PASS" "Dockerfile found in deployment/docker"
else
    print_status "WARN" "No Dockerfile found"
fi

if [ -f deployment/docker/docker-compose.yml ]; then
    print_status "PASS" "Docker Compose configuration found"
    
    # Validate docker-compose syntax
    if command -v docker-compose &> /dev/null; then
        if docker-compose -f deployment/docker/docker-compose.yml config > /dev/null 2>&1; then
            print_status "PASS" "Docker Compose syntax valid"
        else
            print_status "FAIL" "Docker Compose syntax invalid"
        fi
    else
        print_status "INFO" "docker-compose not installed - skipping validation"
    fi
else
    print_status "WARN" "Docker Compose configuration not found"
fi

# 5. Check Kubernetes Manifests
print_section "5. KUBERNETES CONFIGURATION"
K8S_DIR="deployment/kubernetes"
if [ -d "$K8S_DIR" ]; then
    print_status "PASS" "Kubernetes directory found"
    
    # Check for essential manifests
    essential_manifests=(
        "namespace.yaml"
        "deployment.yaml"
        "service.yaml"
        "configmap.yaml"
    )
    
    for manifest in "${essential_manifests[@]}"; do
        if [ -f "$K8S_DIR/$manifest" ]; then
            # Basic YAML validation
            if python3 -c "import yaml; yaml.safe_load(open('$K8S_DIR/$manifest'))" 2>/dev/null; then
                print_status "PASS" "$manifest - valid YAML"
            else
                print_status "FAIL" "$manifest - invalid YAML"
            fi
        else
            print_status "INFO" "$manifest - not found (may be optional)"
        fi
    done
else
    print_status "WARN" "Kubernetes directory not found"
fi

# 6. Check Monitoring Stack
print_section "6. MONITORING STACK"
if [ -d "monitoring" ]; then
    print_status "PASS" "Monitoring directory found"
    
    # Check Prometheus
    if [ -f "monitoring/prometheus.yml" ]; then
        if python3 -c "import yaml; yaml.safe_load(open('monitoring/prometheus.yml'))" 2>/dev/null; then
            print_status "PASS" "Prometheus config - valid YAML"
        else
            print_status "FAIL" "Prometheus config - invalid YAML"
        fi
    else
        print_status "WARN" "Prometheus config not found"
    fi
    
    # Check Grafana
    if [ -d "monitoring/grafana" ]; then
        print_status "PASS" "Grafana directory found"
        
        if [ -f "monitoring/grafana/dashboards/neo-dashboard.json" ]; then
            if python3 -c "import json; json.load(open('monitoring/grafana/dashboards/neo-dashboard.json'))" 2>/dev/null; then
                print_status "PASS" "Grafana dashboard - valid JSON"
            else
                print_status "FAIL" "Grafana dashboard - invalid JSON"
            fi
        else
            print_status "WARN" "Grafana dashboard not found"
        fi
    else
        print_status "WARN" "Grafana directory not found"
    fi
    
    # Check AlertManager
    if [ -f "monitoring/alertmanager.yml" ]; then
        print_status "PASS" "AlertManager config found"
    else
        print_status "WARN" "AlertManager config not found"
    fi
    
    # Check Alert Rules
    if [ -f "monitoring/alerts/alerts.yml" ]; then
        ALERT_COUNT=$(grep -c "alert:" monitoring/alerts/alerts.yml 2>/dev/null || echo 0)
        print_status "PASS" "Alert rules found ($ALERT_COUNT alerts defined)"
    else
        print_status "WARN" "Alert rules not found"
    fi
else
    print_status "FAIL" "Monitoring directory not found"
fi

# 7. Check GitHub Actions Workflows
print_section "7. GITHUB ACTIONS"
if [ -d ".github/workflows" ]; then
    print_status "PASS" "GitHub workflows directory found"
    
    workflow_count=$(ls -1 .github/workflows/*.yml 2>/dev/null | wc -l)
    print_status "INFO" "Found $workflow_count workflow files"
    
    # Check essential workflows
    essential_workflows=(
        "quality-gates.yml"
        "quality-gates-lite.yml"
        "build-test-fixed.yml"
        "validate-all.yml"
    )
    
    for workflow in "${essential_workflows[@]}"; do
        if [ -f ".github/workflows/$workflow" ]; then
            if python3 -c "import yaml; yaml.safe_load(open('.github/workflows/$workflow'))" 2>/dev/null; then
                print_status "PASS" "$workflow - valid YAML"
            else
                print_status "FAIL" "$workflow - invalid YAML"
            fi
        else
            print_status "INFO" "$workflow - not found"
        fi
    done
else
    print_status "FAIL" "GitHub workflows directory not found"
fi

# 8. Check Documentation
print_section "8. DOCUMENTATION"
required_docs=(
    "README.md"
    "IMPLEMENTATION_WORKFLOW.md"
    "QUALITY_GATES.md"
    "DETAILED_TASKS.md"
)

for doc in "${required_docs[@]}"; do
    if [ -f "$doc" ]; then
        size=$(wc -l < "$doc")
        print_status "PASS" "$doc found ($size lines)"
    else
        print_status "WARN" "$doc not found"
    fi
done

# Check API documentation
if [ -d "docs/api" ]; then
    print_status "PASS" "API documentation directory found"
else
    print_status "INFO" "API documentation directory not found"
fi

# 9. Check Source Code Structure
print_section "9. SOURCE CODE STRUCTURE"
if [ -d "src" ]; then
    print_status "PASS" "Source directory found"
    
    # Count source files
    cpp_count=$(find src -name "*.cpp" 2>/dev/null | wc -l)
    h_count=$(find include -name "*.h" 2>/dev/null | wc -l)
    print_status "INFO" "Found $cpp_count .cpp files and $h_count .h files"
else
    print_status "FAIL" "Source directory not found"
fi

if [ -d "include" ]; then
    print_status "PASS" "Include directory found"
else
    print_status "WARN" "Include directory not found"
fi

if [ -d "tests" ]; then
    print_status "PASS" "Tests directory found"
    test_count=$(find tests -name "test_*.cpp" 2>/dev/null | wc -l)
    print_status "INFO" "Found $test_count test files"
else
    print_status "WARN" "Tests directory not found"
fi

# 10. Check Dependencies
print_section "10. DEPENDENCIES"
if [ -f "third_party/httplib/httplib.h" ]; then
    print_status "PASS" "httplib dependency found"
else
    print_status "WARN" "httplib not found"
fi

if [ -d "third_party/googletest" ] && [ -f "third_party/googletest/CMakeLists.txt" ]; then
    print_status "PASS" "GoogleTest dependency found"
else
    print_status "WARN" "GoogleTest not properly initialized"
fi

# Final Summary
print_section "VALIDATION SUMMARY"
echo ""
echo "Total Checks:    $TOTAL_CHECKS"
echo -e "${GREEN}Passed:          $PASSED_CHECKS${NC}"
echo -e "${YELLOW}Warnings:        $WARNINGS${NC}"
echo -e "${RED}Failed:          $FAILED_CHECKS${NC}"
echo ""

# Calculate success rate
if [ $TOTAL_CHECKS -gt 0 ]; then
    SUCCESS_RATE=$((PASSED_CHECKS * 100 / TOTAL_CHECKS))
    echo "Success Rate:    $SUCCESS_RATE%"
    echo ""
    
    if [ $FAILED_CHECKS -eq 0 ]; then
        echo -e "${GREEN}✅ Infrastructure validation PASSED - All critical components are configured${NC}"
        exit 0
    elif [ $SUCCESS_RATE -ge 80 ]; then
        echo -e "${YELLOW}⚠️  Infrastructure validation PASSED WITH WARNINGS - Most components configured${NC}"
        echo ""
        echo "Recommended actions:"
        echo "  1. Initialize googletest submodule: git submodule update --init --recursive"
        echo "  2. Run setup script: ./scripts/setup_project.sh"
        echo "  3. Build the project: make"
        exit 0
    else
        echo -e "${RED}❌ Infrastructure validation FAILED - Critical components missing${NC}"
        exit 1
    fi
else
    echo -e "${RED}❌ No checks performed${NC}"
    exit 1
fi