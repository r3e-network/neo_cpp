#!/bin/bash

# Neo C++ Project Setup Script
# Initializes development environment and project tracking

set -e

echo "======================================"
echo "Neo C++ Project Setup"
echo "======================================"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
PROJECT_NAME="neo-cpp-production"
WORKFLOW_VERSION="1.0.0"

# Function to print colored output
print_status() {
    echo -e "${GREEN}✓${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

# Check prerequisites
check_prerequisites() {
    echo "Checking prerequisites..."
    
    # Check Git
    if ! command -v git &> /dev/null; then
        print_error "Git is not installed"
        exit 1
    fi
    print_status "Git found"
    
    # Check CMake
    if ! command -v cmake &> /dev/null; then
        print_error "CMake is not installed"
        exit 1
    fi
    print_status "CMake found"
    
    # Check Docker
    if ! command -v docker &> /dev/null; then
        print_warning "Docker is not installed (optional for containerization)"
    else
        print_status "Docker found"
    fi
    
    # Check GitHub CLI (optional)
    if ! command -v gh &> /dev/null; then
        print_warning "GitHub CLI not installed (optional for project setup)"
    else
        print_status "GitHub CLI found"
    fi
}

# Setup project structure
setup_project_structure() {
    echo ""
    echo "Setting up project structure..."
    
    # Create necessary directories
    directories=(
        "scripts/gates"
        "scripts/tests"
        "scripts/deployment"
        "docs/runbooks"
        "docs/api"
        "monitoring/dashboards"
        "monitoring/alerts"
        ".github/ISSUE_TEMPLATE"
        "deployment/kubernetes"
        "deployment/docker"
    )
    
    for dir in "${directories[@]}"; do
        if [ ! -d "$dir" ]; then
            mkdir -p "$dir"
            print_status "Created $dir"
        else
            print_status "$dir already exists"
        fi
    done
}

# Setup Git hooks
setup_git_hooks() {
    echo ""
    echo "Setting up Git hooks..."
    
    # Pre-commit hook
    cat > .git/hooks/pre-commit << 'EOF'
#!/bin/bash
# Pre-commit hook for Neo C++

echo "Running pre-commit checks..."

# Run format check
if ! make format-check; then
    echo "Code formatting check failed. Run 'make format' to fix."
    exit 1
fi

# Run quick tests
if ! make test-quick; then
    echo "Quick tests failed."
    exit 1
fi

echo "Pre-commit checks passed!"
EOF
    chmod +x .git/hooks/pre-commit
    print_status "Pre-commit hook installed"
    
    # Pre-push hook
    cat > .git/hooks/pre-push << 'EOF'
#!/bin/bash
# Pre-push hook for Neo C++

echo "Running pre-push checks..."

# Run security scan
if ! make security-scan; then
    echo "Security scan failed."
    exit 1
fi

# Run full tests
if ! make test; then
    echo "Tests failed."
    exit 1
fi

echo "Pre-push checks passed!"
EOF
    chmod +x .git/hooks/pre-push
    print_status "Pre-push hook installed"
}

# Create GitHub issue templates
create_issue_templates() {
    echo ""
    echo "Creating issue templates..."
    
    # Bug report template
    cat > .github/ISSUE_TEMPLATE/bug_report.md << 'EOF'
---
name: Bug Report
about: Report a bug in Neo C++
title: '[BUG] '
labels: bug, triage
assignees: ''
---

## Description
A clear description of the bug.

## Steps to Reproduce
1. Step 1
2. Step 2
3. Step 3

## Expected Behavior
What should happen.

## Actual Behavior
What actually happens.

## Environment
- OS: [e.g., Ubuntu 22.04]
- Neo C++ version: [e.g., 1.0.0]
- Commit hash: [e.g., abc123]

## Logs
```
Paste relevant logs here
```

## Additional Context
Any other relevant information.
EOF
    print_status "Bug report template created"
    
    # Task template
    cat > .github/ISSUE_TEMPLATE/task.md << 'EOF'
---
name: Task
about: Create a task from the workflow
title: '[TASK] '
labels: task
assignees: ''
---

## Task Details
**Sprint**: Sprint X
**Story Points**: X
**Priority**: P0/P1/P2

## Description
Task description from DETAILED_TASKS.md

## Acceptance Criteria
- [ ] Criterion 1
- [ ] Criterion 2
- [ ] Criterion 3

## Dependencies
- List any dependencies

## Definition of Done
- [ ] Code complete and reviewed
- [ ] Tests written and passing
- [ ] Documentation updated
- [ ] Quality gates passed
EOF
    print_status "Task template created"
}

# Setup GitHub Project
setup_github_project() {
    echo ""
    echo "Setting up GitHub Project..."
    
    if command -v gh &> /dev/null; then
        echo "Would you like to create a GitHub Project for tracking? (y/n)"
        read -r response
        
        if [[ "$response" == "y" ]]; then
            # Create project
            gh project create --title "Neo C++ Production Deployment" \
                --body "8-week implementation workflow for Neo C++ production deployment" \
                --visibility public
            
            print_status "GitHub Project created"
            
            # Create columns
            gh api graphql -f query='
            mutation {
              addProjectColumn(input: {projectId: "", name: "Backlog"}) {
                columnEdge { node { id } }
              }
            }'
            
            print_status "Project columns created"
        fi
    else
        print_warning "GitHub CLI not available. Please create project manually."
    fi
}

# Create Makefile targets
create_makefile_targets() {
    echo ""
    echo "Adding Makefile targets..."
    
    cat >> Makefile << 'EOF'

# Quality Gate Targets
.PHONY: gate-security gate-performance gate-quality

gate-security:
	@echo "Running security gate checks..."
	@./scripts/gates/security_gate.sh

gate-performance:
	@echo "Running performance gate checks..."
	@./scripts/gates/performance_gate.sh

gate-quality:
	@echo "Running quality gate checks..."
	@./scripts/gates/quality_gate.sh

# Testing Targets
.PHONY: test-quick test-integration test-performance

test-quick:
	@echo "Running quick tests..."
	@ctest --output-on-failure -R "Quick"

test-integration:
	@echo "Running integration tests..."
	@./scripts/tests/integration_test.sh

test-performance:
	@echo "Running performance tests..."
	@./scripts/tests/performance_test.sh

# Security Targets
.PHONY: security-scan security-audit

security-scan:
	@echo "Running security scan..."
	@cppcheck --enable=all src/ include/

security-audit:
	@echo "Running security audit..."
	@./scripts/security_audit.sh

# Deployment Targets
.PHONY: docker-build k8s-deploy

docker-build:
	@echo "Building Docker image..."
	@docker build -t neo-cpp:latest .

k8s-deploy:
	@echo "Deploying to Kubernetes..."
	@kubectl apply -f deployment/kubernetes/

# Format Targets
.PHONY: format format-check

format:
	@echo "Formatting code..."
	@find src include -name "*.cpp" -o -name "*.h" | xargs clang-format -i

format-check:
	@echo "Checking code format..."
	@find src include -name "*.cpp" -o -name "*.h" | xargs clang-format --dry-run --Werror
EOF
    
    print_status "Makefile targets added"
}

# Create monitoring setup
setup_monitoring() {
    echo ""
    echo "Setting up monitoring configuration..."
    
    # Prometheus configuration
    cat > monitoring/prometheus.yml << 'EOF'
global:
  scrape_interval: 15s
  evaluation_interval: 15s

alerting:
  alertmanagers:
    - static_configs:
        - targets:
          - alertmanager:9093

rule_files:
  - "alerts/*.yml"

scrape_configs:
  - job_name: 'neo-cpp'
    static_configs:
      - targets: ['localhost:9090']
    metrics_path: /metrics
EOF
    print_status "Prometheus configuration created"
    
    # Alert rules
    cat > monitoring/alerts/alerts.yml << 'EOF'
groups:
  - name: neo-cpp
    interval: 30s
    rules:
      - alert: HighMemoryUsage
        expr: process_resident_memory_bytes > 2e9
        for: 5m
        annotations:
          summary: "High memory usage detected"
          
      - alert: LowTPS
        expr: neo_tps < 1000
        for: 10m
        annotations:
          summary: "TPS below threshold"
          
      - alert: HighErrorRate
        expr: rate(neo_errors_total[5m]) > 0.01
        for: 5m
        annotations:
          summary: "High error rate detected"
EOF
    print_status "Alert rules created"
}

# Create workflow scripts
create_workflow_scripts() {
    echo ""
    echo "Creating workflow automation scripts..."
    
    # Sprint planning script
    cat > scripts/sprint_planning.sh << 'EOF'
#!/bin/bash
# Sprint Planning Automation

SPRINT_NUMBER=$1
if [ -z "$SPRINT_NUMBER" ]; then
    echo "Usage: $0 <sprint_number>"
    exit 1
fi

echo "Planning Sprint $SPRINT_NUMBER..."

# Extract tasks for sprint from DETAILED_TASKS.md
grep -A 50 "Sprint $SPRINT_NUMBER" DETAILED_TASKS.md > current_sprint.md

# Create GitHub issues for each task
while IFS= read -r line; do
    if [[ $line == *"Story"* ]]; then
        gh issue create --title "$line" --body "See DETAILED_TASKS.md" --label "sprint-$SPRINT_NUMBER"
    fi
done < current_sprint.md

echo "Sprint $SPRINT_NUMBER planned!"
EOF
    chmod +x scripts/sprint_planning.sh
    print_status "Sprint planning script created"
    
    # Gate check script
    cat > scripts/gates/check_gate.sh << 'EOF'
#!/bin/bash
# Quality Gate Check

GATE_NAME=$1
if [ -z "$GATE_NAME" ]; then
    echo "Usage: $0 <gate_name>"
    exit 1
fi

echo "Checking gate: $GATE_NAME"

# Run gate-specific checks
case $GATE_NAME in
    "security")
        make gate-security
        ;;
    "performance")
        make gate-performance
        ;;
    "quality")
        make gate-quality
        ;;
    *)
        echo "Unknown gate: $GATE_NAME"
        exit 1
        ;;
esac

if [ $? -eq 0 ]; then
    echo "✅ Gate $GATE_NAME passed"
    # Update tracking
    echo "$(date): Gate $GATE_NAME passed" >> gate_history.log
else
    echo "❌ Gate $GATE_NAME failed"
    exit 1
fi
EOF
    chmod +x scripts/gates/check_gate.sh
    print_status "Gate check script created"
}

# Create initial project dashboard
create_dashboard() {
    echo ""
    echo "Creating project dashboard..."
    
    cat > PROJECT_DASHBOARD.md << 'EOF'
# Neo C++ Production Deployment Dashboard

## Current Sprint
**Sprint**: 1  
**Week**: 1-2  
**Focus**: Security & Performance  

## Gate Status
| Gate | Status | Date | Owner |
|------|--------|------|-------|
| Security Assessment | 🔴 Not Started | - | Security |
| Performance Baseline | 🔴 Not Started | - | Performance |
| Code Quality | 🔴 Not Started | - | Development |

## Metrics
| Metric | Current | Target | Status |
|--------|---------|--------|--------|
| TPS | - | 5000 | ⏳ |
| Memory | - | <2GB | ⏳ |
| Coverage | - | >85% | ⏳ |
| Vulnerabilities | - | 0 | ⏳ |

## Team Allocation
| Role | Person | Allocation |
|------|--------|------------|
| Security Architect | TBD | 100% |
| Performance Engineer | TBD | 100% |
| DevOps Engineer | TBD | 20% |
| Backend Developer | TBD | 50% |

## Risk Register
| Risk | Probability | Impact | Status |
|------|-------------|--------|--------|
| Performance degradation | Medium | High | Monitoring |
| Security vulnerability | Low | Critical | Monitoring |

## Links
- [Implementation Workflow](IMPLEMENTATION_WORKFLOW.md)
- [Detailed Tasks](DETAILED_TASKS.md)
- [Quality Gates](QUALITY_GATES.md)
- [GitHub Project](#)
EOF
    print_status "Project dashboard created"
}

# Main execution
main() {
    echo ""
    echo "Starting Neo C++ project setup..."
    echo "Version: $WORKFLOW_VERSION"
    echo ""
    
    check_prerequisites
    setup_project_structure
    setup_git_hooks
    create_issue_templates
    setup_github_project
    create_makefile_targets
    setup_monitoring
    create_workflow_scripts
    create_dashboard
    
    echo ""
    echo "======================================"
    echo -e "${GREEN}✓ Project setup complete!${NC}"
    echo "======================================"
    echo ""
    echo "Next steps:"
    echo "1. Review PROJECT_DASHBOARD.md"
    echo "2. Assign team members to roles"
    echo "3. Run './scripts/sprint_planning.sh 1' to plan Sprint 1"
    echo "4. Begin security audit (Week 1)"
    echo ""
    echo "Useful commands:"
    echo "  make gate-security    - Run security gate checks"
    echo "  make test-quick       - Run quick tests"
    echo "  make docker-build     - Build Docker image"
    echo ""
}

# Run main function
main "$@"