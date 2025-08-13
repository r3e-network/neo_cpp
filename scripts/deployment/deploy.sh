#!/bin/bash

# Neo C++ Deployment Script
# Automated deployment to various environments

set -e

# Configuration
ENVIRONMENT="${1:-staging}"
VERSION="${2:-latest}"
DEPLOYMENT_METHOD="${3:-docker}"  # docker, kubernetes, binary
DRY_RUN="${DRY_RUN:-false}"
ROLLBACK_ON_FAILURE="${ROLLBACK_ON_FAILURE:-true}"
HEALTH_CHECK_RETRIES=30
HEALTH_CHECK_INTERVAL=10

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
NC='\033[0m'

echo "======================================"
echo "Neo C++ Deployment"
echo "======================================"
echo "Environment: $ENVIRONMENT"
echo "Version: $VERSION"
echo "Method: $DEPLOYMENT_METHOD"
echo "Dry Run: $DRY_RUN"
echo ""

# Deployment state
DEPLOYMENT_ID="deploy-$(date +%Y%m%d-%H%M%S)"
DEPLOYMENT_LOG="deployment-$DEPLOYMENT_ID.log"
PREVIOUS_VERSION=""
DEPLOYMENT_SUCCESS=false

# Helper functions
print_success() { echo -e "${GREEN}✓${NC} $1" | tee -a "$DEPLOYMENT_LOG"; }
print_error() { echo -e "${RED}✗${NC} $1" | tee -a "$DEPLOYMENT_LOG"; }
print_warning() { echo -e "${YELLOW}⚠${NC} $1" | tee -a "$DEPLOYMENT_LOG"; }
print_info() { echo -e "${BLUE}ℹ${NC} $1" | tee -a "$DEPLOYMENT_LOG"; }
print_stage() { echo -e "${MAGENTA}▶${NC} $1" | tee -a "$DEPLOYMENT_LOG"; }

# Execute command with dry-run support
execute() {
    local cmd="$1"
    if [ "$DRY_RUN" = "true" ]; then
        print_info "[DRY-RUN] Would execute: $cmd"
    else
        print_info "Executing: $cmd"
        eval "$cmd"
    fi
}

# Pre-deployment checks
pre_deployment_checks() {
    print_stage "Running pre-deployment checks..."
    
    # Check environment configuration
    local config_file="config/$ENVIRONMENT.yaml"
    if [ ! -f "$config_file" ]; then
        print_error "Configuration file not found: $config_file"
        return 1
    fi
    print_success "Configuration found for $ENVIRONMENT"
    
    # Run quality gates
    print_info "Running quality gates..."
    
    if [ "$DRY_RUN" = "false" ]; then
        # Security gate
        if ./scripts/gates/security_gate.sh > /dev/null 2>&1; then
            print_success "Security gate passed"
        else
            print_error "Security gate failed"
            return 1
        fi
        
        # Performance gate
        if ./scripts/gates/performance_gate.sh > /dev/null 2>&1; then
            print_success "Performance gate passed"
        else
            print_error "Performance gate failed"
            return 1
        fi
        
        # Quality gate
        if ./scripts/gates/quality_gate.sh > /dev/null 2>&1; then
            print_success "Quality gate passed"
        else
            print_warning "Quality gate has warnings"
        fi
    else
        print_info "[DRY-RUN] Would run quality gates"
    fi
    
    # Check deployment permissions
    case $ENVIRONMENT in
        production)
            print_info "Production deployment - checking additional requirements..."
            # Check for approval
            if [ ! -f ".approvals/production-$VERSION.txt" ] && [ "$DRY_RUN" = "false" ]; then
                print_error "Production deployment requires approval"
                return 1
            fi
            ;;
        staging)
            print_info "Staging deployment - standard checks"
            ;;
        development)
            print_info "Development deployment - minimal checks"
            ;;
        *)
            print_error "Unknown environment: $ENVIRONMENT"
            return 1
            ;;
    esac
    
    return 0
}

# Build artifacts
build_artifacts() {
    print_stage "Building deployment artifacts..."
    
    case $DEPLOYMENT_METHOD in
        docker)
            print_info "Building Docker image..."
            execute "docker build -t neo-cpp:$VERSION -f Dockerfile ."
            execute "docker tag neo-cpp:$VERSION neo-cpp:$DEPLOYMENT_ID"
            
            if [ "$ENVIRONMENT" = "production" ]; then
                execute "docker tag neo-cpp:$VERSION ghcr.io/neo-project/neo-cpp:$VERSION"
            fi
            ;;
            
        kubernetes)
            print_info "Building Kubernetes manifests..."
            execute "helm template neo-cpp ./charts/neo-cpp \
                --set image.tag=$VERSION \
                --set environment=$ENVIRONMENT \
                --values ./charts/neo-cpp/values-$ENVIRONMENT.yaml \
                > deployment-$DEPLOYMENT_ID.yaml"
            ;;
            
        binary)
            print_info "Building binary release..."
            execute "make clean"
            execute "make release VERSION=$VERSION"
            execute "tar -czf neo-cpp-$VERSION.tar.gz build/release/*"
            ;;
            
        *)
            print_error "Unknown deployment method: $DEPLOYMENT_METHOD"
            return 1
            ;;
    esac
    
    print_success "Artifacts built successfully"
    return 0
}

# Backup current deployment
backup_current() {
    print_stage "Backing up current deployment..."
    
    case $DEPLOYMENT_METHOD in
        docker)
            # Get current running version
            PREVIOUS_VERSION=$(docker ps --filter "name=neo-cpp" --format "{{.Image}}" | cut -d: -f2 || echo "none")
            
            if [ "$PREVIOUS_VERSION" != "none" ] && [ "$PREVIOUS_VERSION" != "" ]; then
                print_info "Current version: $PREVIOUS_VERSION"
                execute "docker tag neo-cpp:$PREVIOUS_VERSION neo-cpp:backup-$DEPLOYMENT_ID"
                
                # Backup data volumes
                execute "docker run --rm -v neo-data:/data -v $(pwd)/backups:/backup \
                    alpine tar czf /backup/data-backup-$DEPLOYMENT_ID.tar.gz /data"
            else
                print_info "No existing deployment found"
            fi
            ;;
            
        kubernetes)
            # Backup current deployment
            execute "kubectl get deployment neo-cpp -n neo-cpp -o yaml > backup-deployment-$DEPLOYMENT_ID.yaml || true"
            execute "kubectl get configmap -n neo-cpp -o yaml > backup-configmap-$DEPLOYMENT_ID.yaml || true"
            
            # Get current version
            PREVIOUS_VERSION=$(kubectl get deployment neo-cpp -n neo-cpp \
                -o jsonpath='{.spec.template.spec.containers[0].image}' | cut -d: -f2 || echo "none")
            ;;
            
        binary)
            # Backup current binary
            if [ -f "/opt/neo-cpp/bin/neo-node" ]; then
                execute "cp -r /opt/neo-cpp /opt/neo-cpp-backup-$DEPLOYMENT_ID"
                PREVIOUS_VERSION=$(./neo-node --version | cut -d' ' -f2 || echo "none")
            fi
            ;;
    esac
    
    print_success "Backup completed"
    return 0
}

# Deploy new version
deploy_new_version() {
    print_stage "Deploying version $VERSION..."
    
    case $DEPLOYMENT_METHOD in
        docker)
            # Stop current containers
            print_info "Stopping current containers..."
            execute "docker-compose -f deployment/docker/docker-compose.yml down || true"
            
            # Update compose file with new version
            execute "sed -i.bak 's/neo-cpp:latest/neo-cpp:$VERSION/g' deployment/docker/docker-compose.yml"
            
            # Start new containers
            print_info "Starting new containers..."
            execute "docker-compose -f deployment/docker/docker-compose.yml up -d"
            ;;
            
        kubernetes)
            print_info "Applying Kubernetes manifests..."
            
            # Apply new deployment
            execute "kubectl apply -f deployment-$DEPLOYMENT_ID.yaml"
            
            # Wait for rollout
            if [ "$DRY_RUN" = "false" ]; then
                print_info "Waiting for rollout to complete..."
                kubectl rollout status deployment/neo-cpp -n neo-cpp --timeout=600s
            fi
            ;;
            
        binary)
            print_info "Installing new binary..."
            
            # Stop service
            execute "systemctl stop neo-cpp || true"
            
            # Extract and install
            execute "tar -xzf neo-cpp-$VERSION.tar.gz -C /opt/neo-cpp"
            execute "chmod +x /opt/neo-cpp/bin/neo-node"
            
            # Start service
            execute "systemctl start neo-cpp"
            ;;
    esac
    
    print_success "Deployment executed"
    return 0
}

# Health checks
run_health_checks() {
    print_stage "Running health checks..."
    
    local retries=0
    local healthy=false
    
    while [ $retries -lt $HEALTH_CHECK_RETRIES ]; do
        case $DEPLOYMENT_METHOD in
            docker)
                if docker exec neo-node-1 curl -f http://localhost:10332/health > /dev/null 2>&1; then
                    healthy=true
                    break
                fi
                ;;
                
            kubernetes)
                local ready_pods=$(kubectl get pods -n neo-cpp -l app=neo-cpp \
                    -o jsonpath='{.items[*].status.conditions[?(@.type=="Ready")].status}' | grep -c "True" || echo "0")
                local total_pods=$(kubectl get pods -n neo-cpp -l app=neo-cpp --no-headers | wc -l)
                
                if [ "$ready_pods" -eq "$total_pods" ] && [ "$total_pods" -gt 0 ]; then
                    healthy=true
                    break
                fi
                ;;
                
            binary)
                if curl -f http://localhost:10332/health > /dev/null 2>&1; then
                    healthy=true
                    break
                fi
                ;;
        esac
        
        retries=$((retries + 1))
        print_info "Health check attempt $retries/$HEALTH_CHECK_RETRIES"
        sleep $HEALTH_CHECK_INTERVAL
    done
    
    if [ "$healthy" = true ]; then
        print_success "Health checks passed"
        return 0
    else
        print_error "Health checks failed after $retries attempts"
        return 1
    fi
}

# Smoke tests
run_smoke_tests() {
    print_stage "Running smoke tests..."
    
    # Basic API test
    print_info "Testing RPC endpoint..."
    if curl -s -X POST http://localhost:10332 \
        -H "Content-Type: application/json" \
        -d '{"jsonrpc":"2.0","method":"getversion","params":[],"id":1}' | grep -q "result"; then
        print_success "RPC endpoint responsive"
    else
        print_error "RPC endpoint not responding"
        return 1
    fi
    
    # Check block production
    print_info "Checking block production..."
    local initial_height=$(curl -s -X POST http://localhost:10332 \
        -H "Content-Type: application/json" \
        -d '{"jsonrpc":"2.0","method":"getblockcount","params":[],"id":1}' | jq -r '.result')
    
    sleep 30
    
    local new_height=$(curl -s -X POST http://localhost:10332 \
        -H "Content-Type: application/json" \
        -d '{"jsonrpc":"2.0","method":"getblockcount","params":[],"id":1}' | jq -r '.result')
    
    if [ "$new_height" -gt "$initial_height" ]; then
        print_success "Block production working (height: $initial_height -> $new_height)"
    else
        print_error "Block production not working"
        return 1
    fi
    
    # Check peer connections
    print_info "Checking peer connections..."
    local peer_count=$(curl -s -X POST http://localhost:10332 \
        -H "Content-Type: application/json" \
        -d '{"jsonrpc":"2.0","method":"getconnectioncount","params":[],"id":1}' | jq -r '.result')
    
    if [ "$peer_count" -gt 0 ]; then
        print_success "Connected to $peer_count peers"
    else
        print_warning "No peer connections"
    fi
    
    return 0
}

# Rollback deployment
rollback_deployment() {
    print_stage "Rolling back deployment..."
    
    if [ "$PREVIOUS_VERSION" = "none" ] || [ "$PREVIOUS_VERSION" = "" ]; then
        print_error "No previous version to rollback to"
        return 1
    fi
    
    print_info "Rolling back to version: $PREVIOUS_VERSION"
    
    case $DEPLOYMENT_METHOD in
        docker)
            # Stop current containers
            execute "docker-compose -f deployment/docker/docker-compose.yml down"
            
            # Restore compose file
            execute "mv deployment/docker/docker-compose.yml.bak deployment/docker/docker-compose.yml"
            
            # Start previous version
            execute "docker-compose -f deployment/docker/docker-compose.yml up -d"
            ;;
            
        kubernetes)
            # Rollback deployment
            execute "kubectl rollout undo deployment/neo-cpp -n neo-cpp"
            execute "kubectl rollout status deployment/neo-cpp -n neo-cpp --timeout=600s"
            ;;
            
        binary)
            # Stop service
            execute "systemctl stop neo-cpp"
            
            # Restore backup
            execute "rm -rf /opt/neo-cpp"
            execute "mv /opt/neo-cpp-backup-$DEPLOYMENT_ID /opt/neo-cpp"
            
            # Start service
            execute "systemctl start neo-cpp"
            ;;
    esac
    
    # Verify rollback
    sleep 10
    if run_health_checks; then
        print_success "Rollback completed successfully"
        return 0
    else
        print_error "Rollback failed - manual intervention required"
        return 1
    fi
}

# Post-deployment tasks
post_deployment_tasks() {
    print_stage "Running post-deployment tasks..."
    
    # Update monitoring
    print_info "Updating monitoring configuration..."
    execute "curl -X POST http://prometheus:9090/-/reload || true"
    
    # Send notifications
    if [ "$ENVIRONMENT" = "production" ]; then
        print_info "Sending deployment notifications..."
        
        # Slack notification
        if [ -n "$SLACK_WEBHOOK" ]; then
            execute "curl -X POST $SLACK_WEBHOOK \
                -H 'Content-Type: application/json' \
                -d '{\"text\":\"Neo C++ $VERSION deployed to $ENVIRONMENT\"}'"
        fi
        
        # Email notification
        if command -v mail &> /dev/null && [ -n "$DEPLOYMENT_EMAIL" ]; then
            echo "Neo C++ version $VERSION has been deployed to $ENVIRONMENT" | \
                mail -s "Deployment Success: Neo C++ $VERSION" "$DEPLOYMENT_EMAIL"
        fi
    fi
    
    # Update deployment registry
    cat > "deployments/$DEPLOYMENT_ID.json" << EOF
{
    "id": "$DEPLOYMENT_ID",
    "version": "$VERSION",
    "environment": "$ENVIRONMENT",
    "method": "$DEPLOYMENT_METHOD",
    "timestamp": "$(date -Iseconds)",
    "previous_version": "$PREVIOUS_VERSION",
    "status": "success"
}
EOF
    
    # Clean up old backups (keep last 5)
    print_info "Cleaning up old backups..."
    ls -t backups/data-backup-*.tar.gz 2>/dev/null | tail -n +6 | xargs rm -f 2>/dev/null || true
    
    print_success "Post-deployment tasks completed"
    return 0
}

# Main deployment flow
main() {
    echo "Starting deployment: $DEPLOYMENT_ID" | tee "$DEPLOYMENT_LOG"
    echo "" | tee -a "$DEPLOYMENT_LOG"
    
    # Create necessary directories
    mkdir -p deployments backups .approvals
    
    # Pre-deployment checks
    if ! pre_deployment_checks; then
        print_error "Pre-deployment checks failed"
        exit 1
    fi
    
    # Build artifacts
    if ! build_artifacts; then
        print_error "Failed to build artifacts"
        exit 1
    fi
    
    # Backup current deployment
    if ! backup_current; then
        print_warning "Backup failed - continuing with caution"
    fi
    
    # Deploy new version
    if ! deploy_new_version; then
        print_error "Deployment failed"
        
        if [ "$ROLLBACK_ON_FAILURE" = "true" ] && [ "$DRY_RUN" = "false" ]; then
            rollback_deployment
        fi
        exit 1
    fi
    
    # Skip health checks in dry-run mode
    if [ "$DRY_RUN" = "false" ]; then
        # Health checks
        if ! run_health_checks; then
            print_error "Health checks failed"
            
            if [ "$ROLLBACK_ON_FAILURE" = "true" ]; then
                rollback_deployment
            fi
            exit 1
        fi
        
        # Smoke tests
        if ! run_smoke_tests; then
            print_error "Smoke tests failed"
            
            if [ "$ROLLBACK_ON_FAILURE" = "true" ]; then
                rollback_deployment
            fi
            exit 1
        fi
    else
        print_info "[DRY-RUN] Would run health checks and smoke tests"
    fi
    
    # Post-deployment tasks
    post_deployment_tasks
    
    # Success
    DEPLOYMENT_SUCCESS=true
    
    echo "" | tee -a "$DEPLOYMENT_LOG"
    echo "======================================"
    echo -e "${GREEN}✓ DEPLOYMENT SUCCESSFUL${NC}"
    echo "======================================"
    echo "Deployment ID: $DEPLOYMENT_ID"
    echo "Version: $VERSION"
    echo "Environment: $ENVIRONMENT"
    echo "Log: $DEPLOYMENT_LOG"
    
    exit 0
}

# Handle interrupts
trap 'echo "Deployment interrupted"; [ "$DEPLOYMENT_SUCCESS" = false ] && rollback_deployment; exit 130' INT TERM

# Run main deployment
main "$@"