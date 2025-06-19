#!/bin/bash
# Professional build script for Neo C++

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default values
BUILD_TYPE="Release"
BUILD_TESTS="ON"
BUILD_EXAMPLES="ON"
CLEAN_BUILD="false"
INSTALL_DEPS="false"
VERBOSE="false"

print_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -t, --type TYPE        Build type (Debug/Release/RelWithDebInfo) [default: Release]"
    echo "  --no-tests            Disable building tests"
    echo "  --no-examples         Disable building examples"
    echo "  -c, --clean           Clean build directory before building"
    echo "  -d, --deps            Install dependencies via vcpkg"
    echo "  -v, --verbose         Verbose build output"
    echo "  -h, --help            Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                    # Release build with tests"
    echo "  $0 -t Debug -c        # Clean debug build"
    echo "  $0 --no-tests -v      # Release build without tests, verbose"
}

print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        --no-tests)
            BUILD_TESTS="OFF"
            shift
            ;;
        --no-examples)
            BUILD_EXAMPLES="OFF"
            shift
            ;;
        -c|--clean)
            CLEAN_BUILD="true"
            shift
            ;;
        -d|--deps)
            INSTALL_DEPS="true"
            shift
            ;;
        -v|--verbose)
            VERBOSE="true"
            shift
            ;;
        -h|--help)
            print_usage
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            print_usage
            exit 1
            ;;
    esac
done

# Validate build type
case $BUILD_TYPE in
    Debug|Release|RelWithDebInfo)
        ;;
    *)
        print_error "Invalid build type: $BUILD_TYPE"
        print_usage
        exit 1
        ;;
esac

print_info "Starting Neo C++ build process"
print_info "Build type: $BUILD_TYPE"
print_info "Tests: $BUILD_TESTS"
print_info "Examples: $BUILD_EXAMPLES"

# Check if we're in the right directory
if [[ ! -f "CMakeLists.txt" ]]; then
    print_error "CMakeLists.txt not found. Please run this script from the project root."
    exit 1
fi

# Check for required tools
command -v cmake >/dev/null 2>&1 || { print_error "cmake is required but not installed."; exit 1; }
command -v git >/dev/null 2>&1 || { print_error "git is required but not installed."; exit 1; }

# Create build directory
BUILD_DIR="build"
if [[ "$CLEAN_BUILD" == "true" ]]; then
    print_info "Cleaning build directory"
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"

# Install dependencies if requested
if [[ "$INSTALL_DEPS" == "true" ]]; then
    print_info "Setting up dependencies via vcpkg"
    if [[ ! -d "vcpkg" ]]; then
        print_info "Cloning vcpkg"
        git clone https://github.com/Microsoft/vcpkg.git
    fi
    
    if [[ ! -f "vcpkg/vcpkg" ]]; then
        print_info "Bootstrapping vcpkg"
        ./vcpkg/bootstrap-vcpkg.sh
    fi
fi

# Configure CMake
print_info "Configuring CMake"
CMAKE_ARGS=(
    "-B" "$BUILD_DIR"
    "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
    "-DNEO_BUILD_TESTS=$BUILD_TESTS"
    "-DNEO_BUILD_EXAMPLES=$BUILD_EXAMPLES"
)

if [[ -d "vcpkg" ]]; then
    CMAKE_ARGS+=("-DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake")
fi

if command -v ninja >/dev/null 2>&1; then
    CMAKE_ARGS+=("-G" "Ninja")
    BUILDER="ninja"
else
    BUILDER="make"
fi

if [[ "$VERBOSE" == "true" ]]; then
    CMAKE_ARGS+=("-DCMAKE_VERBOSE_MAKEFILE=ON")
fi

cmake "${CMAKE_ARGS[@]}"

# Build
print_info "Building project"
BUILD_ARGS=(
    "--build" "$BUILD_DIR"
    "--config" "$BUILD_TYPE"
)

if [[ "$VERBOSE" == "true" ]]; then
    BUILD_ARGS+=("--verbose")
fi

# Use parallel jobs
if [[ "$BUILDER" == "ninja" ]]; then
    BUILD_ARGS+=("-j" "$(nproc)")
elif [[ "$BUILDER" == "make" ]]; then
    BUILD_ARGS+=("--" "-j$(nproc)")
fi

cmake "${BUILD_ARGS[@]}"

print_info "Build completed successfully"

# Run tests if enabled
if [[ "$BUILD_TESTS" == "ON" ]]; then
    print_info "Running tests"
    cd "$BUILD_DIR"
    ctest --output-on-failure --timeout 300
    cd ..
    print_info "All tests passed"
fi

print_info "Neo C++ build process completed successfully!"
print_info "Executables are in: $BUILD_DIR/"