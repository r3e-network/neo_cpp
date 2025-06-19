#!/bin/bash

# Neo C++ Test Runner Script

echo "╔═══════════════════════════════════════════════════════════════════════════════╗"
echo "║                        NEO C++ TEST RUNNER                                   ║"
echo "╚═══════════════════════════════════════════════════════════════════════════════╝"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Function to check if Docker is installed
check_docker() {
    if ! command -v docker &> /dev/null; then
        echo -e "${RED}❌ Docker is not installed${NC}"
        echo "Please install Docker first: https://docs.docker.com/get-docker/"
        return 1
    fi
    
    if ! docker info &> /dev/null; then
        echo -e "${RED}❌ Docker daemon is not running${NC}"
        echo "Please start Docker daemon"
        return 1
    fi
    
    echo -e "${GREEN}✅ Docker is ready${NC}"
    return 0
}

# Function to run tests in Docker
run_docker_tests() {
    echo -e "\n${BLUE}🐳 Running tests in Docker container...${NC}"
    echo "This will build a complete test environment with all dependencies"
    
    # Build and run tests
    docker-compose -f docker-compose.test.yml up --build neo-cpp-tests
    
    if [ $? -eq 0 ]; then
        echo -e "\n${GREEN}✅ Tests completed successfully!${NC}"
        echo "Test results are available in ./test-results/"
    else
        echo -e "\n${RED}❌ Tests failed${NC}"
        return 1
    fi
}

# Function to run tests locally
run_local_tests() {
    echo -e "\n${BLUE}🔧 Running tests locally...${NC}"
    
    # Check if build directory exists
    if [ ! -d "build" ]; then
        echo "Creating build directory..."
        mkdir -p build
    fi
    
    cd build
    
    # Configure if needed
    if [ ! -f "CMakeCache.txt" ]; then
        echo "Configuring project..."
        cmake .. -DCMAKE_BUILD_TYPE=Debug
        
        if [ $? -ne 0 ]; then
            echo -e "${RED}❌ Configuration failed${NC}"
            echo "Missing dependencies. Try using Docker instead:"
            echo "  ./run_tests.sh --docker"
            return 1
        fi
    fi
    
    # Build
    echo "Building tests..."
    make -j$(nproc)
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}❌ Build failed${NC}"
        return 1
    fi
    
    # Run tests
    echo "Running tests..."
    ctest --verbose --output-on-failure
    
    if [ $? -eq 0 ]; then
        echo -e "\n${GREEN}✅ All tests passed!${NC}"
    else
        echo -e "\n${RED}❌ Some tests failed${NC}"
        return 1
    fi
    
    cd ..
}

# Main menu
show_menu() {
    echo -e "\n${BLUE}Select test execution method:${NC}"
    echo "1) Run tests in Docker (recommended - no setup required)"
    echo "2) Run tests locally (requires dependencies installed)"
    echo "3) Open interactive Docker development environment"
    echo "4) Show build instructions"
    echo "5) Exit"
    
    read -p "Enter your choice [1-5]: " choice
    
    case $choice in
        1)
            if check_docker; then
                run_docker_tests
            fi
            ;;
        2)
            run_local_tests
            ;;
        3)
            echo -e "\n${BLUE}🐳 Starting interactive Docker environment...${NC}"
            docker-compose -f docker-compose.test.yml run --rm neo-cpp-dev
            ;;
        4)
            cat << EOF

${BLUE}📋 BUILD INSTRUCTIONS${NC}
═══════════════════════════════════════════════════════════════════════════════

${YELLOW}Option 1: Using Docker (Recommended)${NC}
1. Install Docker: https://docs.docker.com/get-docker/
2. Run: ./run_tests.sh --docker

${YELLOW}Option 2: Local Installation${NC}
1. Install dependencies:
   # Ubuntu/Debian:
   sudo apt-get update
   sudo apt-get install -y build-essential cmake git
   sudo apt-get install -y libboost-all-dev libssl-dev
   sudo apt-get install -y nlohmann-json3-dev libspdlog-dev
   
   # Install Google Test:
   git clone https://github.com/google/googletest.git
   cd googletest && mkdir build && cd build
   cmake .. && make && sudo make install

2. Build the project:
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Debug
   make -j\$(nproc)

3. Run tests:
   ctest --verbose

${YELLOW}Option 3: Using vcpkg${NC}
1. Bootstrap vcpkg:
   ./vcpkg/bootstrap-vcpkg.sh

2. Install dependencies:
   ./vcpkg/vcpkg install gtest boost openssl nlohmann-json spdlog

3. Build with vcpkg toolchain:
   cmake .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
   make -j\$(nproc)

EOF
            ;;
        5)
            echo "Exiting..."
            exit 0
            ;;
        *)
            echo -e "${RED}Invalid choice${NC}"
            ;;
    esac
}

# Parse command line arguments
if [ "$1" == "--docker" ]; then
    if check_docker; then
        run_docker_tests
    fi
elif [ "$1" == "--local" ]; then
    run_local_tests
elif [ "$1" == "--help" ] || [ "$1" == "-h" ]; then
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  --docker    Run tests in Docker container"
    echo "  --local     Run tests locally"
    echo "  --help      Show this help message"
    echo ""
    echo "If no option is provided, an interactive menu will be shown."
else
    # Show interactive menu
    while true; do
        show_menu
        echo -e "\n${BLUE}Press Enter to continue...${NC}"
        read
    done
fi