@echo off
setlocal EnableDelayedExpansion

REM ========================================================================
REM Neo C++ Build Verification Script for Windows
REM This script verifies the build environment, dependencies, and builds
REM the complete Neo C++ node with verification
REM ========================================================================

echo.
echo ================================================================
echo   Neo C++ Build Verification Script v1.0.0
echo   Verifying build environment and building production node
echo ================================================================
echo.

REM Set script variables
set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%\.."
set "BUILD_DIR=%PROJECT_ROOT%\build-release"
set "TEST_BUILD_DIR=%PROJECT_ROOT%\test_build"
set "VCPKG_DIR=%PROJECT_ROOT%\vcpkg"
set "ERROR_COUNT=0"
set "WARNING_COUNT=0"

REM Color codes for output (if supported)
set "RED=[91m"
set "GREEN=[92m" 
set "YELLOW=[93m"
set "BLUE=[94m"
set "NC=[0m"

REM ========================================================================
REM Helper Functions
REM ========================================================================

:log_info
echo %BLUE%[INFO]%NC% %~1
goto :eof

:log_success
echo %GREEN%[SUCCESS]%NC% %~1
goto :eof

:log_warning
echo %YELLOW%[WARNING]%NC% %~1
set /a WARNING_COUNT+=1
goto :eof

:log_error
echo %RED%[ERROR]%NC% %~1
set /a ERROR_COUNT+=1
goto :eof

REM ========================================================================
REM System Requirements Check
REM ========================================================================

:check_requirements
call :log_info "Checking system requirements..."

REM Check if we're in the right directory
if not exist "%PROJECT_ROOT%\CMakeLists.txt" (
    call :log_error "CMakeLists.txt not found. Are you in the correct project directory?"
    goto :error_exit
)

REM Check CMake
cmake --version > nul 2>&1
if errorlevel 1 (
    call :log_error "CMake is required but not found. Please install CMake 3.20 or higher."
    goto :error_exit
)

for /f "tokens=3" %%i in ('cmake --version ^| findstr /C:"cmake version"') do set CMAKE_VERSION=%%i
call :log_success "CMake !CMAKE_VERSION! found"

REM Check Visual Studio Build Tools / MSVC
where cl.exe > nul 2>&1
if errorlevel 1 (
    call :log_warning "MSVC compiler not found in PATH. Attempting to locate Visual Studio..."
    call :find_visual_studio
    if errorlevel 1 (
        call :log_error "Visual Studio 2019/2022 or Build Tools required but not found"
        goto :error_exit
    )
) else (
    call :log_success "MSVC compiler found in PATH"
)

REM Check vcpkg
if not exist "%VCPKG_DIR%\vcpkg.exe" (
    call :log_warning "vcpkg not found in project directory"
    call :log_info "Attempting to bootstrap vcpkg..."
    call :bootstrap_vcpkg
    if errorlevel 1 goto :error_exit
) else (
    call :log_success "vcpkg found"
)

REM Check disk space (require at least 5GB)
call :check_disk_space
if errorlevel 1 goto :error_exit

call :log_success "System requirements check completed"
goto :eof

REM ========================================================================
REM Find Visual Studio Installation
REM ========================================================================

:find_visual_studio
REM Try to find Visual Studio using vswhere
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
    for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VS_PATH=%%i"
        if exist "!VS_PATH!\VC\Auxiliary\Build\vcvars64.bat" (
            call :log_info "Found Visual Studio at: !VS_PATH!"
            call "!VS_PATH!\VC\Auxiliary\Build\vcvars64.bat" > nul
            call :log_success "Visual Studio environment configured"
            goto :eof
        )
    )
)

REM Fallback: try common VS paths
set "VS_PATHS=%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise;%ProgramFiles%\Microsoft Visual Studio\2022\Professional;%ProgramFiles%\Microsoft Visual Studio\2022\Community;%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Enterprise;%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional;%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community"

for %%i in ("%VS_PATHS:;=" "%") do (
    if exist "%%~i\VC\Auxiliary\Build\vcvars64.bat" (
        call :log_info "Found Visual Studio at: %%~i"
        call "%%~i\VC\Auxiliary\Build\vcvars64.bat" > nul
        call :log_success "Visual Studio environment configured"
        goto :eof
    )
)

exit /b 1

REM ========================================================================
REM Bootstrap vcpkg if needed
REM ========================================================================

:bootstrap_vcpkg
call :log_info "Bootstrapping vcpkg..."

cd /d "%PROJECT_ROOT%"

if not exist "vcpkg" (
    call :log_info "Cloning vcpkg repository..."
    git clone https://github.com/Microsoft/vcpkg.git
    if errorlevel 1 (
        call :log_error "Failed to clone vcpkg repository"
        exit /b 1
    )
)

cd vcpkg

if not exist "vcpkg.exe" (
    call :log_info "Building vcpkg..."
    call bootstrap-vcpkg.bat
    if errorlevel 1 (
        call :log_error "Failed to bootstrap vcpkg"
        exit /b 1
    )
)

call :log_success "vcpkg bootstrapped successfully"
goto :eof

REM ========================================================================
REM Check available disk space
REM ========================================================================

:check_disk_space
REM Get available space on current drive
for /f "tokens=3" %%i in ('dir /-c "%PROJECT_ROOT%" ^| findstr /C:"bytes free"') do set AVAILABLE_SPACE=%%i

REM Remove commas from number
set "AVAILABLE_SPACE=!AVAILABLE_SPACE:,=!"

REM Check if we have at least 5GB (5368709120 bytes)
if !AVAILABLE_SPACE! LSS 5368709120 (
    call :log_error "Insufficient disk space. Required: 5GB, Available: approximately !AVAILABLE_SPACE! bytes"
    exit /b 1
)

call :log_success "Sufficient disk space available"
goto :eof

REM ========================================================================
REM Install Dependencies via vcpkg
REM ========================================================================

:install_dependencies
call :log_info "Installing dependencies via vcpkg..."

cd /d "%VCPKG_DIR%"

set "TRIPLET=x64-windows"
call :log_info "Installing packages for triplet: !TRIPLET!"

REM Core dependencies - install them one by one with error checking
set "PACKAGES=boost-system boost-filesystem boost-thread boost-chrono boost-regex boost-program-options openssl nlohmann-json spdlog fmt gtest rocksdb"

for %%p in (%PACKAGES%) do (
    call :log_info "Installing %%p..."
    vcpkg install %%p:!TRIPLET!
    if errorlevel 1 (
        call :log_warning "Failed to install %%p, continuing with other packages..."
    ) else (
        call :log_success "%%p installed successfully"
    )
)

call :log_success "Dependencies installation completed"
goto :eof

REM ========================================================================
REM Clean Previous Builds
REM ========================================================================

:clean_build
call :log_info "Cleaning previous build artifacts..."

if exist "%BUILD_DIR%" (
    rd /s /q "%BUILD_DIR%" 2>nul
    call :log_success "Build directory cleaned"
)

if exist "%TEST_BUILD_DIR%" (
    rd /s /q "%TEST_BUILD_DIR%" 2>nul
    call :log_success "Test build directory cleaned"
)

REM Clean any build artifacts in root
if exist "%PROJECT_ROOT%\CMakeCache.txt" del /f /q "%PROJECT_ROOT%\CMakeCache.txt" 2>nul
if exist "%PROJECT_ROOT%\CMakeFiles" rd /s /q "%PROJECT_ROOT%\CMakeFiles" 2>nul

call :log_success "Previous build artifacts cleaned"
goto :eof

REM ========================================================================
REM Configure Build with CMake
REM ========================================================================

:configure_build
call :log_info "Configuring build with CMake..."

mkdir "%BUILD_DIR%" 2>nul
cd /d "%BUILD_DIR%"

REM Configure with production settings
cmake ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_DIR%\scripts\buildsystems\vcpkg.cmake" ^
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON ^
    -DBUILD_TESTING=ON ^
    -G "Visual Studio 17 2022" ^
    "%PROJECT_ROOT%"

if errorlevel 1 (
    call :log_error "CMake configuration failed"
    goto :error_exit
)

call :log_success "Build configured successfully"
goto :eof

REM ========================================================================
REM Build the Project
REM ========================================================================

:build_project
call :log_info "Building Neo C++ node..."

cd /d "%BUILD_DIR%"

REM Build the project with maximum CPU cores
cmake --build . --config Release --parallel

if errorlevel 1 (
    call :log_error "Build failed"
    goto :error_exit
)

call :log_success "Build completed successfully"
goto :eof

REM ========================================================================
REM Verify Build Output
REM ========================================================================

:verify_build_output
call :log_info "Verifying build output..."

REM Check if main executables exist
if exist "%BUILD_DIR%\src\node\Release\neo-node.exe" (
    call :log_success "neo-node.exe built successfully"
) else (
    call :log_error "neo-node.exe not found"
)

if exist "%BUILD_DIR%\Release\neo-cli.exe" (
    call :log_success "neo-cli.exe built successfully"
) else (
    call :log_warning "neo-cli.exe not found (may not be implemented yet)"
)

REM Check core library
if exist "%BUILD_DIR%\Release\neo-core.lib" (
    call :log_success "neo-core.lib built successfully"
) else (
    call :log_warning "neo-core.lib not found"
)

REM Test basic executable functionality
cd /d "%BUILD_DIR%\src\node\Release"
neo-node.exe --help > nul 2>&1
if errorlevel 1 (
    call :log_warning "neo-node.exe --help failed (may need configuration)"
) else (
    call :log_success "neo-node.exe responds to --help"
)

call :log_success "Build output verification completed"
goto :eof

REM ========================================================================
REM Run Quick Tests
REM ========================================================================

:run_quick_tests
call :log_info "Running quick verification tests..."

cd /d "%BUILD_DIR%"

REM Run unit tests if they exist
if exist "Release\neo-unit-tests.exe" (
    call :log_info "Running unit tests..."
    Release\neo-unit-tests.exe --gtest_brief=1
    if errorlevel 1 (
        call :log_warning "Some unit tests failed"
    ) else (
        call :log_success "Unit tests passed"
    )
) else (
    call :log_warning "Unit tests not found, skipping..."
)

REM Run integration tests if they exist
if exist "Release\neo-integration-tests.exe" (
    call :log_info "Running integration tests..."
    Release\neo-integration-tests.exe
    if errorlevel 1 (
        call :log_warning "Some integration tests failed"
    ) else (
        call :log_success "Integration tests passed"
    )
) else (
    call :log_warning "Integration tests not found, skipping..."
)

call :log_success "Quick tests completed"
goto :eof

REM ========================================================================
REM Generate Build Report
REM ========================================================================

:generate_report
call :log_info "Generating build verification report..."

set "REPORT_FILE=%PROJECT_ROOT%\build_verification_report.txt"

echo Neo C++ Build Verification Report > "%REPORT_FILE%"
echo ================================== >> "%REPORT_FILE%"
echo. >> "%REPORT_FILE%"
echo Build Date: %DATE% %TIME% >> "%REPORT_FILE%"
echo Build Configuration: Release >> "%REPORT_FILE%"
echo Platform: Windows x64 >> "%REPORT_FILE%"
echo. >> "%REPORT_FILE%"
echo System Information: >> "%REPORT_FILE%"
echo - CMake Version: !CMAKE_VERSION! >> "%REPORT_FILE%"
echo - Compiler: MSVC >> "%REPORT_FILE%"
echo - vcpkg: %VCPKG_DIR% >> "%REPORT_FILE%"
echo - Build Directory: %BUILD_DIR% >> "%REPORT_FILE%"
echo. >> "%REPORT_FILE%"
echo Build Results: >> "%REPORT_FILE%"
if exist "%BUILD_DIR%\src\node\Release\neo-node.exe" (
    echo - neo-node.exe: BUILT >> "%REPORT_FILE%"
) else (
    echo - neo-node.exe: FAILED >> "%REPORT_FILE%"
)
if exist "%BUILD_DIR%\Release\neo-cli.exe" (
    echo - neo-cli.exe: BUILT >> "%REPORT_FILE%"
) else (
    echo - neo-cli.exe: NOT BUILT >> "%REPORT_FILE%"
)
if exist "%BUILD_DIR%\Release\neo-core.lib" (
    echo - neo-core.lib: BUILT >> "%REPORT_FILE%"
) else (
    echo - neo-core.lib: FAILED >> "%REPORT_FILE%"
)
echo. >> "%REPORT_FILE%"
echo Verification Status: >> "%REPORT_FILE%"
echo - Errors: !ERROR_COUNT! >> "%REPORT_FILE%"
echo - Warnings: !WARNING_COUNT! >> "%REPORT_FILE%"
if !ERROR_COUNT! EQU 0 (
    echo - Overall Status: SUCCESS >> "%REPORT_FILE%"
) else (
    echo - Overall Status: FAILED >> "%REPORT_FILE%"
)

call :log_success "Build verification report generated: %REPORT_FILE%"
goto :eof

REM ========================================================================
REM Main Execution Flow
REM ========================================================================

:main
cd /d "%PROJECT_ROOT%"

REM Parse command line arguments
set "SKIP_DEPS=false"
set "SKIP_TESTS=false"
set "QUICK_BUILD=false"

:parse_args
if "%~1"=="" goto :start_build
if "%~1"=="--skip-deps" (
    set "SKIP_DEPS=true"
    shift
    goto :parse_args
)
if "%~1"=="--skip-tests" (
    set "SKIP_TESTS=true"
    shift
    goto :parse_args
)
if "%~1"=="--quick" (
    set "QUICK_BUILD=true"
    shift
    goto :parse_args
)
if "%~1"=="--help" (
    echo Usage: %0 [options]
    echo Options:
    echo   --skip-deps    Skip dependency installation
    echo   --skip-tests   Skip test execution  
    echo   --quick        Quick build without full verification
    echo   --help         Show this help message
    goto :eof
)
call :log_warning "Unknown option: %~1"
shift
goto :parse_args

:start_build
call :log_info "Starting Neo C++ build verification..."

REM Execute verification steps
call :check_requirements
if errorlevel 1 goto :error_exit

if "%SKIP_DEPS%"=="false" (
    call :install_dependencies
    if errorlevel 1 goto :error_exit
) else (
    call :log_warning "Skipping dependency installation"
)

call :clean_build
call :configure_build
if errorlevel 1 goto :error_exit

call :build_project
if errorlevel 1 goto :error_exit

call :verify_build_output

if "%SKIP_TESTS%"=="false" (
    call :run_quick_tests
) else (
    call :log_warning "Skipping tests"
)

call :generate_report

REM Final status
echo.
echo ================================================================
if !ERROR_COUNT! EQU 0 (
    call :log_success "Neo C++ build verification completed successfully!"
    echo.
    echo Build artifacts available in: %BUILD_DIR%
    echo Main executable: %BUILD_DIR%\src\node\Release\neo-node.exe
    echo Build report: %PROJECT_ROOT%\build_verification_report.txt
) else (
    call :log_error "Build verification completed with !ERROR_COUNT! errors and !WARNING_COUNT! warnings"
    echo Please check the build report for details: %PROJECT_ROOT%\build_verification_report.txt
)
echo ================================================================
echo.

goto :eof

:error_exit
call :log_error "Build verification failed!"
echo.
echo Please check the error messages above and ensure:
echo 1. Visual Studio 2019/2022 or Build Tools are installed
echo 2. CMake 3.20+ is installed and in PATH
echo 3. Git is installed (for vcpkg)
echo 4. You have at least 5GB of free disk space
echo 5. You're running this script from the project root
echo.
exit /b 1

REM Execute main function
call :main %* 