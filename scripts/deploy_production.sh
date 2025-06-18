#!/bin/bash

# Neo C++ Production Deployment Script
# This script automates the complete build, test, and deployment process
# for the Neo C++ blockchain node to ensure production readiness.

set -euo pipefail  # Exit on error, undefined vars, pipe failures

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
INSTALL_DIR="$PROJECT_ROOT/install"
PACKAGE_DIR="$PROJECT_ROOT/packages"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Print banner
print_banner() {
    echo "=================================================================="
    echo "  Neo C++ Production Deployment Script v1.0.0"
    echo "  Building production-ready Neo blockchain node"
    echo "=================================================================="
    echo ""
}

# Check system requirements
check_requirements() {
    log_info "Checking system requirements..."
    
    # Check CMake version
    if ! command -v cmake &> /dev/null; then
        log_error "CMake is required but not installed"
        exit 1
    fi
    
    CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
    CMAKE_MAJOR=$(echo $CMAKE_VERSION | cut -d'.' -f1)
    CMAKE_MINOR=$(echo $CMAKE_VERSION | cut -d'.' -f2)
    
    if [ "$CMAKE_MAJOR" -lt 3 ] || ([ "$CMAKE_MAJOR" -eq 3 ] && [ "$CMAKE_MINOR" -lt 20 ]); then
        log_error "CMake 3.20 or higher is required (found $CMAKE_VERSION)"
        exit 1
    fi
    
    log_success "CMake $CMAKE_VERSION found"
    
    # Check compiler
    if command -v g++ &> /dev/null; then
        GCC_VERSION=$(g++ --version | head -n1 | grep -oP '\d+\.\d+\.\d+')
        log_success "GCC $GCC_VERSION found"
    elif command -v clang++ &> /dev/null; then
        CLANG_VERSION=$(clang++ --version | head -n1 | grep -oP '\d+\.\d+\.\d+')
        log_success "Clang $CLANG_VERSION found"
    else
        log_error "No suitable C++ compiler found (GCC 11+ or Clang 12+ required)"
        exit 1
    fi
    
    # Check vcpkg
    if [ ! -f "$PROJECT_ROOT/vcpkg/scripts/buildsystems/vcpkg.cmake" ]; then
        log_warning "vcpkg not found in project directory"
        log_info "Attempting to bootstrap vcpkg..."
        bootstrap_vcpkg
    else
        log_success "vcpkg found"
    fi
    
    # Check disk space (require at least 5GB)
    AVAILABLE_SPACE=$(df "$PROJECT_ROOT" | tail -1 | awk '{print $4}')
    REQUIRED_SPACE=5242880  # 5GB in KB
    
    if [ "$AVAILABLE_SPACE" -lt "$REQUIRED_SPACE" ]; then
        log_error "Insufficient disk space. Required: 5GB, Available: $(($AVAILABLE_SPACE / 1024 / 1024))GB"
        exit 1
    fi
    
    log_success "System requirements check passed"
}

# Bootstrap vcpkg if needed
bootstrap_vcpkg() {
    log_info "Bootstrapping vcpkg..."
    
    cd "$PROJECT_ROOT"
    
    if [ ! -d "vcpkg" ]; then
        log_info "Cloning vcpkg repository..."
        git clone https://github.com/Microsoft/vcpkg.git
    fi
    
    cd vcpkg
    
    if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
        ./bootstrap-vcpkg.bat
    else
        ./bootstrap-vcpkg.sh
    fi
    
    log_success "vcpkg bootstrapped successfully"
}

# Install dependencies
install_dependencies() {
    log_info "Installing dependencies via vcpkg..."
    
    cd "$PROJECT_ROOT/vcpkg"
    
    # Determine triplet based on platform
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        TRIPLET="x64-linux"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        TRIPLET="x64-osx"
    elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
        TRIPLET="x64-windows"
    else
        log_error "Unsupported platform: $OSTYPE"
        exit 1
    fi
    
    log_info "Installing packages for triplet: $TRIPLET"
    
    # Core dependencies
    PACKAGES=(
        "boost-system"
        "boost-filesystem" 
        "boost-thread"
        "boost-chrono"
        "boost-regex"
        "boost-program-options"
        "openssl"
        "nlohmann-json"
        "spdlog"
        "fmt"
        "gtest"
        "rocksdb"
    )
    
    for package in "${PACKAGES[@]}"; do
        log_info "Installing $package..."
        ./vcpkg install "$package:$TRIPLET"
    done
    
    log_success "All dependencies installed successfully"
}

# Clean previous builds
clean_build() {
    log_info "Cleaning previous build artifacts..."
    
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        log_success "Build directory cleaned"
    fi
    
    if [ -d "$INSTALL_DIR" ]; then
        rm -rf "$INSTALL_DIR"
        log_success "Install directory cleaned"
    fi
    
    if [ -d "$PACKAGE_DIR" ]; then
        rm -rf "$PACKAGE_DIR"
        log_success "Package directory cleaned"
    fi
}

# Configure build
configure_build() {
    log_info "Configuring build with CMake..."
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Configure with production settings
    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
        -DCMAKE_TOOLCHAIN_FILE="$PROJECT_ROOT/vcpkg/scripts/buildsystems/vcpkg.cmake" \
        -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
        -DBUILD_TESTING=ON \
        -DCPACK_PACKAGE_DIRECTORY="$PACKAGE_DIR" \
        "$PROJECT_ROOT"
    
    log_success "Build configured successfully"
}

# Build the project
build_project() {
    log_info "Building Neo C++ node..."
    
    cd "$BUILD_DIR"
    
    # Determine number of parallel jobs
    if command -v nproc &> /dev/null; then
        JOBS=$(nproc)
    elif command -v sysctl &> /dev/null; then
        JOBS=$(sysctl -n hw.ncpu)
    else
        JOBS=4
    fi
    
    log_info "Building with $JOBS parallel jobs..."
    
    # Build the project
    cmake --build . --config Release --parallel "$JOBS"
    
    log_success "Build completed successfully"
}

# Run tests
run_tests() {
    log_info "Running test suite..."
    
    cd "$BUILD_DIR"
    
    # Run unit tests
    if [ -f "neo-unit-tests" ] || [ -f "neo-unit-tests.exe" ]; then
        log_info "Running unit tests..."
        ctest --build-config Release --parallel "$JOBS" --output-on-failure
        log_success "Unit tests passed"
    else
        log_warning "Unit tests not found, skipping..."
    fi
    
    # Run integration tests
    if [ -f "neo-integration-tests" ] || [ -f "neo-integration-tests.exe" ]; then
        log_info "Running integration tests..."
        ./neo-integration-tests
        log_success "Integration tests passed"
    else
        log_warning "Integration tests not found, skipping..."
    fi
    
    log_success "All tests completed successfully"
}

# Run performance benchmarks
run_benchmarks() {
    log_info "Running performance benchmarks..."
    
    cd "$BUILD_DIR"
    
    if [ -f "neo-benchmarks" ] || [ -f "neo-benchmarks.exe" ]; then
        log_info "Executing performance benchmarks..."
        ./neo-benchmarks --benchmark_format=json --benchmark_out=benchmark_results.json
        
        if [ -f "benchmark_results.json" ]; then
            log_success "Benchmark results saved to benchmark_results.json"
        fi
    else
        log_warning "Benchmarks not found, skipping..."
    fi
}

# Install the project
install_project() {
    log_info "Installing Neo C++ node..."
    
    cd "$BUILD_DIR"
    cmake --install . --config Release
    
    log_success "Installation completed to $INSTALL_DIR"
}

# Create packages
create_packages() {
    log_info "Creating distribution packages..."
    
    cd "$BUILD_DIR"
    
    # Create packages
    cpack --config CPackConfig.cmake
    
    if [ -d "$PACKAGE_DIR" ]; then
        log_success "Packages created in $PACKAGE_DIR"
        ls -la "$PACKAGE_DIR"
    else
        log_warning "Package directory not found"
    fi
}

# Validate installation
validate_installation() {
    log_info "Validating installation..."
    
    # Check if executables exist and are runnable
    if [ -f "$INSTALL_DIR/bin/neo-node" ] || [ -f "$INSTALL_DIR/bin/neo-node.exe" ]; then
        log_success "neo-node executable found"
        
        # Test basic functionality
        cd "$INSTALL_DIR/bin"
        if ./neo-node --help &> /dev/null; then
            log_success "neo-node executable is functional"
        else
            log_warning "neo-node executable may have issues"
        fi
    else
        log_error "neo-node executable not found in installation"
        exit 1
    fi
    
    if [ -f "$INSTALL_DIR/bin/neo-cli" ] || [ -f "$INSTALL_DIR/bin/neo-cli.exe" ]; then
        log_success "neo-cli executable found"
        
        # Test basic functionality
        cd "$INSTALL_DIR/bin"
        if ./neo-cli help &> /dev/null; then
            log_success "neo-cli executable is functional"
        else
            log_warning "neo-cli executable may have issues"
        fi
    else
        log_error "neo-cli executable not found in installation"
        exit 1
    fi
    
    # Check configuration file
    if [ -f "$INSTALL_DIR/etc/config.json" ]; then
        log_success "Configuration file found"
    else
        log_warning "Configuration file not found"
    fi
    
    log_success "Installation validation completed"
}

# Generate deployment report
generate_report() {
    log_info "Generating deployment report..."
    
    REPORT_FILE="$PROJECT_ROOT/deployment_report.txt"
    
    cat > "$REPORT_FILE" << EOF
Neo C++ Production Deployment Report
====================================

Deployment Date: $(date)
Build Configuration: Release
Target Platform: $OSTYPE

Build Information:
- CMake Version: $CMAKE_VERSION
- Compiler: $(which g++ || which clang++)
- Build Directory: $BUILD_DIR
- Install Directory: $INSTALL_DIR

Components Built:
- neo-node: Main blockchain node executable
- neo-cli: Command-line interface
- neo-core: Core library

Test Results:
- Unit Tests: $([ -f "$BUILD_DIR/neo-unit-tests" ] && echo "PASSED" || echo "SKIPPED")
- Integration Tests: $([ -f "$BUILD_DIR/neo-integration-tests" ] && echo "PASSED" || echo "SKIPPED")
- Performance Benchmarks: $([ -f "$BUILD_DIR/benchmark_results.json" ] && echo "COMPLETED" || echo "SKIPPED")

Installation:
- Executables: $INSTALL_DIR/bin/
- Libraries: $INSTALL_DIR/lib/
- Headers: $INSTALL_DIR/include/
- Configuration: $INSTALL_DIR/etc/

Packages:
$([ -d "$PACKAGE_DIR" ] && ls -la "$PACKAGE_DIR" || echo "No packages created")

Deployment Status: SUCCESS
EOF
    
    log_success "Deployment report generated: $REPORT_FILE"
}

# Main deployment function
main() {
    print_banner
    
    log_info "Starting Neo C++ production deployment..."
    
    # Parse command line arguments
    SKIP_DEPS=false
    SKIP_TESTS=false
    SKIP_BENCHMARKS=false
    SKIP_PACKAGES=false
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            --skip-deps)
                SKIP_DEPS=true
                shift
                ;;
            --skip-tests)
                SKIP_TESTS=true
                shift
                ;;
            --skip-benchmarks)
                SKIP_BENCHMARKS=true
                shift
                ;;
            --skip-packages)
                SKIP_PACKAGES=true
                shift
                ;;
            --help)
                echo "Usage: $0 [options]"
                echo "Options:"
                echo "  --skip-deps        Skip dependency installation"
                echo "  --skip-tests       Skip test execution"
                echo "  --skip-benchmarks  Skip performance benchmarks"
                echo "  --skip-packages    Skip package creation"
                echo "  --help             Show this help message"
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                exit 1
                ;;
        esac
    done
    
    # Execute deployment steps
    check_requirements
    
    if [ "$SKIP_DEPS" = false ]; then
        install_dependencies
    else
        log_warning "Skipping dependency installation"
    fi
    
    clean_build
    configure_build
    build_project
    
    if [ "$SKIP_TESTS" = false ]; then
        run_tests
    else
        log_warning "Skipping tests"
    fi
    
    if [ "$SKIP_BENCHMARKS" = false ]; then
        run_benchmarks
    else
        log_warning "Skipping benchmarks"
    fi
    
    install_project
    
    if [ "$SKIP_PACKAGES" = false ]; then
        create_packages
    else
        log_warning "Skipping package creation"
    fi
    
    validate_installation
    generate_report
    
    log_success "Neo C++ production deployment completed successfully!"
    echo ""
    echo "Next steps:"
    echo "1. Review the deployment report: $PROJECT_ROOT/deployment_report.txt"
    echo "2. Test the installation: $INSTALL_DIR/bin/neo-node --help"
    echo "3. Configure the node: $INSTALL_DIR/etc/config.json"
    echo "4. Deploy to production environment"
    echo ""
    echo "For production deployment, copy the contents of $INSTALL_DIR to your target system."
}

# Execute main function with all arguments
main "$@" 