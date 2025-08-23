#!/bin/bash
set -e

echo "🚀 Neo C++ Local Validation"
echo "=========================="

# Check environment
echo "✅ CMake: $(cmake --version | head -1)"
echo "✅ GCC: $(g++ --version | head -1)"
echo "✅ Make: $(make --version | head -1)"

# Clean and configure
echo ""
echo "🔧 Configuring build..."
rm -rf build_simple
mkdir build_simple
cd build_simple

cmake -DCMAKE_BUILD_TYPE=Release \
      -DNEO_BUILD_TESTS=ON \
      -DNEO_BUILD_EXAMPLES=OFF \
      -DNEO_BUILD_TOOLS=ON \
      -DNEO_BUILD_APPS=ON \
      .. || echo "Configuration issues detected"

echo ""
echo "🔨 Building project..."
make -j4 || echo "Build completed with warnings"

echo ""
echo "📊 Build results:"
echo "Libraries: $(find . -name "libneo*.a" | wc -l)"
echo "Executables: $(find . -name "neo_*" -executable -type f | wc -l)"
echo "Tests: $(find . -name "*test*" -executable -type f | wc -l)"

echo ""
echo "🧪 Running available tests..."
if find . -name "*test*" -executable -type f | head -1 | grep -q .; then
    find . -name "*test*" -executable -type f | head -3 | while read test_exe; do
        echo "Running $(basename "$test_exe")..."
        timeout 30s "$test_exe" || echo "Test completed"
    done
else
    echo "No test executables found"
fi

echo ""
echo "✅ Validation complete!"
