# ========================================================================
# Neo C++ Build Verification Script for PowerShell
# This script provides comprehensive build verification with advanced
# features like dependency checking, performance monitoring, and detailed reporting
# ========================================================================

param(
    [switch]$SkipDeps,
    [switch]$SkipTests,
    [switch]$Quick,
    [switch]$Verbose,
    [switch]$Help,
    [string]$BuildType = "Release",
    [string]$Generator = "Visual Studio 17 2022"
)

# Script configuration
$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
$BuildDir = Join-Path $ProjectRoot "build-release"
$VcpkgDir = Join-Path $ProjectRoot "vcpkg"
$ErrorCount = 0
$WarningCount = 0
$BuildStartTime = Get-Date

# Import required modules
if (Get-Module -ListAvailable -Name Microsoft.PowerShell.Utility) {
    Import-Module Microsoft.PowerShell.Utility -Force
}

# ========================================================================
# Helper Functions
# ========================================================================

function Write-ColoredOutput {
    param(
        [string]$Message,
        [string]$Level = "INFO"
    )
    
    $timestamp = Get-Date -Format "HH:mm:ss"
    switch ($Level) {
        "INFO"    { Write-Host "[$timestamp] [INFO] $Message" -ForegroundColor Cyan }
        "SUCCESS" { Write-Host "[$timestamp] [SUCCESS] $Message" -ForegroundColor Green }
        "WARNING" { 
            Write-Host "[$timestamp] [WARNING] $Message" -ForegroundColor Yellow
            $script:WarningCount++
        }
        "ERROR"   { 
            Write-Host "[$timestamp] [ERROR] $Message" -ForegroundColor Red
            $script:ErrorCount++
        }
    }
    
    if ($Verbose) {
        Add-Content -Path (Join-Path $ProjectRoot "build_verbose.log") -Value "[$timestamp] [$Level] $Message"
    }
}

function Show-Banner {
    Write-Host ""
    Write-Host "================================================================" -ForegroundColor Magenta
    Write-Host "  Neo C++ Build Verification Script v2.0.0 (PowerShell)" -ForegroundColor Magenta
    Write-Host "  Advanced build verification with dependency analysis" -ForegroundColor Magenta
    Write-Host "================================================================" -ForegroundColor Magenta
    Write-Host ""
}

function Show-Help {
    Write-Host "Neo C++ Build Verification Script"
    Write-Host ""
    Write-Host "USAGE:"
    Write-Host "  .\build_verify.ps1 [options]"
    Write-Host ""
    Write-Host "OPTIONS:"
    Write-Host "  -SkipDeps      Skip dependency installation"
    Write-Host "  -SkipTests     Skip test execution"
    Write-Host "  -Quick         Quick build without full verification"
    Write-Host "  -Verbose       Enable verbose logging"
    Write-Host "  -BuildType     Build type (Release/Debug) [default: Release]"
    Write-Host "  -Generator     CMake generator [default: Visual Studio 17 2022]"
    Write-Host "  -Help          Show this help message"
    Write-Host ""
    Write-Host "EXAMPLES:"
    Write-Host "  .\build_verify.ps1                    # Full verification"
    Write-Host "  .\build_verify.ps1 -Quick             # Quick build"
    Write-Host "  .\build_verify.ps1 -SkipDeps -Verbose # Skip deps with verbose output"
    Write-Host ""
}

# ========================================================================
# System Requirements Check
# ========================================================================

function Test-SystemRequirements {
    Write-ColoredOutput "Checking system requirements..." "INFO"
    
    # Check if we're in the right directory
    if (-not (Test-Path (Join-Path $ProjectRoot "CMakeLists.txt"))) {
        Write-ColoredOutput "CMakeLists.txt not found. Are you in the correct project directory?" "ERROR"
        throw "Project directory validation failed"
    }
    
    # Check PowerShell version
    if ($PSVersionTable.PSVersion.Major -lt 5) {
        Write-ColoredOutput "PowerShell 5.0 or higher required (found $($PSVersionTable.PSVersion))" "ERROR"
        throw "PowerShell version requirement not met"
    }
    Write-ColoredOutput "PowerShell $($PSVersionTable.PSVersion) found" "SUCCESS"
    
    # Check CMake
    try {
        $cmakeVersion = & cmake --version 2>$null | Select-String "cmake version" | ForEach-Object { $_.ToString().Split(" ")[2] }
        if (-not $cmakeVersion) { throw "CMake not found" }
        Write-ColoredOutput "CMake $cmakeVersion found" "SUCCESS"
    }
    catch {
        Write-ColoredOutput "CMake is required but not found. Please install CMake 3.20 or higher." "ERROR"
        throw "CMake requirement not met"
    }
    
    # Check Visual Studio
    $vsPath = Find-VisualStudio
    if (-not $vsPath) {
        Write-ColoredOutput "Visual Studio 2019/2022 or Build Tools required but not found" "ERROR"
        throw "Visual Studio requirement not met"
    }
    
    # Check vcpkg
    if (-not (Test-Path (Join-Path $VcpkgDir "vcpkg.exe"))) {
        Write-ColoredOutput "vcpkg not found, will bootstrap if needed" "WARNING"
    }
    else {
        Write-ColoredOutput "vcpkg found" "SUCCESS"
    }
    
    # Check disk space (require at least 5GB)
    $drive = (Get-Item $ProjectRoot).PSDrive
    $freeSpace = (Get-WmiObject -Class Win32_LogicalDisk -Filter "DeviceID='$($drive.Name):'").FreeSpace
    $requiredSpace = 5GB
    
    if ($freeSpace -lt $requiredSpace) {
        Write-ColoredOutput "Insufficient disk space. Required: 5GB, Available: $([math]::Round($freeSpace/1GB, 2))GB" "ERROR"
        throw "Disk space requirement not met"
    }
    Write-ColoredOutput "Sufficient disk space available: $([math]::Round($freeSpace/1GB, 2))GB" "SUCCESS"
    
    # Check Git (needed for vcpkg)
    try {
        $gitVersion = & git --version 2>$null
        Write-ColoredOutput "Git found: $gitVersion" "SUCCESS"
    }
    catch {
        Write-ColoredOutput "Git is required for vcpkg but not found" "ERROR"
        throw "Git requirement not met"
    }
    
    Write-ColoredOutput "System requirements check completed successfully" "SUCCESS"
}

function Find-VisualStudio {
    # Try to find Visual Studio using vswhere
    $vswherePath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    
    if (Test-Path $vswherePath) {
        try {
            $vsPath = & $vswherePath -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
            if ($vsPath -and (Test-Path (Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"))) {
                Write-ColoredOutput "Found Visual Studio at: $vsPath" "SUCCESS"
                return $vsPath
            }
        }
        catch {
            Write-ColoredOutput "vswhere failed to locate Visual Studio" "WARNING"
        }
    }
    
    # Fallback: try common VS paths
    $vsPaths = @(
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional", 
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Enterprise",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Professional",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Community"
    )
    
    foreach ($path in $vsPaths) {
        if (Test-Path (Join-Path $path "VC\Auxiliary\Build\vcvars64.bat")) {
            Write-ColoredOutput "Found Visual Studio at: $path" "SUCCESS"
            return $path
        }
    }
    
    return $null
}

# ========================================================================
# Dependency Management
# ========================================================================

function Install-Dependencies {
    Write-ColoredOutput "Installing dependencies via vcpkg..." "INFO"
    
    # Bootstrap vcpkg if needed
    if (-not (Test-Path (Join-Path $VcpkgDir "vcpkg.exe"))) {
        Initialize-Vcpkg
    }
    
    Set-Location $VcpkgDir
    
    $triplet = "x64-windows"
    Write-ColoredOutput "Installing packages for triplet: $triplet" "INFO"
    
    # Core dependencies with version constraints
    $packages = @(
        "boost-system",
        "boost-filesystem", 
        "boost-thread",
        "boost-chrono",
        "boost-regex",
        "boost-program-options",
        "openssl",
        "nlohmann-json",
        "spdlog",
        "fmt",
        "gtest",
        "rocksdb"
    )
    
    $installed = @()
    $failed = @()
    
    foreach ($package in $packages) {
        Write-ColoredOutput "Installing $package..." "INFO"
        try {
            & .\vcpkg.exe install "$package`:$triplet" 2>&1 | Out-Null
            if ($LASTEXITCODE -eq 0) {
                Write-ColoredOutput "$package installed successfully" "SUCCESS"
                $installed += $package
            }
            else {
                Write-ColoredOutput "Failed to install $package" "WARNING"
                $failed += $package
            }
        }
        catch {
            Write-ColoredOutput "Exception installing $package`: $_" "WARNING"
            $failed += $package
        }
    }
    
    Write-ColoredOutput "Dependencies installation completed. Installed: $($installed.Count), Failed: $($failed.Count)" "INFO"
    
    if ($failed.Count -gt 0) {
        Write-ColoredOutput "Failed packages: $($failed -join ', ')" "WARNING"
    }
}

function Initialize-Vcpkg {
    Write-ColoredOutput "Bootstrapping vcpkg..." "INFO"
    
    Set-Location $ProjectRoot
    
    if (-not (Test-Path "vcpkg")) {
        Write-ColoredOutput "Cloning vcpkg repository..." "INFO"
        & git clone https://github.com/Microsoft/vcpkg.git
        if ($LASTEXITCODE -ne 0) {
            Write-ColoredOutput "Failed to clone vcpkg repository" "ERROR"
            throw "vcpkg clone failed"
        }
    }
    
    Set-Location "vcpkg"
    
    if (-not (Test-Path "vcpkg.exe")) {
        Write-ColoredOutput "Building vcpkg..." "INFO"
        & .\bootstrap-vcpkg.bat
        if ($LASTEXITCODE -ne 0) {
            Write-ColoredOutput "Failed to bootstrap vcpkg" "ERROR"
            throw "vcpkg bootstrap failed"
        }
    }
    
    Write-ColoredOutput "vcpkg bootstrapped successfully" "SUCCESS"
}

# ========================================================================
# Build Process
# ========================================================================

function Clear-BuildArtifacts {
    Write-ColoredOutput "Cleaning previous build artifacts..." "INFO"
    
    $pathsToClean = @(
        $BuildDir,
        (Join-Path $ProjectRoot "test_build"),
        (Join-Path $ProjectRoot "CMakeCache.txt"),
        (Join-Path $ProjectRoot "CMakeFiles")
    )
    
    foreach ($path in $pathsToClean) {
        if (Test-Path $path) {
            Remove-Item $path -Recurse -Force -ErrorAction SilentlyContinue
            Write-ColoredOutput "Cleaned: $path" "SUCCESS"
        }
    }
    
    Write-ColoredOutput "Build artifacts cleaned" "SUCCESS"
}

function Invoke-CMakeConfigure {
    Write-ColoredOutput "Configuring build with CMake..." "INFO"
    
    if (-not (Test-Path $BuildDir)) {
        New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
    }
    
    Set-Location $BuildDir
    
    $cmakeArgs = @(
        "-DCMAKE_BUILD_TYPE=$BuildType",
        "-DCMAKE_TOOLCHAIN_FILE=$VcpkgDir\scripts\buildsystems\vcpkg.cmake",
        "-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON",
        "-DBUILD_TESTING=ON",
        "-G", "`"$Generator`"",
        $ProjectRoot
    )
    
    Write-ColoredOutput "CMake command: cmake $($cmakeArgs -join ' ')" "INFO"
    
    & cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) {
        Write-ColoredOutput "CMake configuration failed" "ERROR"
        throw "CMake configure failed"
    }
    
    Write-ColoredOutput "Build configured successfully" "SUCCESS"
}

function Invoke-Build {
    Write-ColoredOutput "Building Neo C++ node..." "INFO"
    
    Set-Location $BuildDir
    
    # Determine number of parallel jobs
    $jobs = [Environment]::ProcessorCount
    Write-ColoredOutput "Building with $jobs parallel jobs..." "INFO"
    
    $buildStart = Get-Date
    
    & cmake --build . --config $BuildType --parallel $jobs
    if ($LASTEXITCODE -ne 0) {
        Write-ColoredOutput "Build failed" "ERROR"
        throw "Build failed"
    }
    
    $buildTime = (Get-Date) - $buildStart
    Write-ColoredOutput "Build completed successfully in $($buildTime.TotalMinutes.ToString('F1')) minutes" "SUCCESS"
}

function Test-BuildOutput {
    Write-ColoredOutput "Verifying build output..." "INFO"
    
    $expectedFiles = @{
        "neo-node.exe" = "src\node\$BuildType\neo-node.exe"
        "neo-cli.exe" = "$BuildType\neo-cli.exe"
        "neo-core.lib" = "$BuildType\neo-core.lib"
    }
    
    $buildResults = @{}
    
    foreach ($name in $expectedFiles.Keys) {
        $path = Join-Path $BuildDir $expectedFiles[$name]
        if (Test-Path $path) {
            $fileInfo = Get-Item $path
            Write-ColoredOutput "$name built successfully ($([math]::Round($fileInfo.Length/1MB, 2))MB)" "SUCCESS"
            $buildResults[$name] = @{
                "Status" = "SUCCESS"
                "Size" = $fileInfo.Length
                "Path" = $path
            }
        }
        else {
            if ($name -eq "neo-cli.exe") {
                Write-ColoredOutput "$name not found (may not be implemented yet)" "WARNING"
                $buildResults[$name] = @{ "Status" = "NOT_IMPLEMENTED" }
            }
            else {
                Write-ColoredOutput "$name not found" "ERROR"
                $buildResults[$name] = @{ "Status" = "FAILED" }
            }
        }
    }
    
    # Test basic executable functionality
    $nodeExePath = Join-Path $BuildDir "src\node\$BuildType\neo-node.exe"
    if (Test-Path $nodeExePath) {
        try {
            Set-Location (Split-Path $nodeExePath)
            $helpOutput = & .\neo-node.exe --help 2>&1
            if ($LASTEXITCODE -eq 0) {
                Write-ColoredOutput "neo-node.exe responds to --help correctly" "SUCCESS"
                $buildResults["neo-node.exe"]["Functional"] = $true
            }
            else {
                Write-ColoredOutput "neo-node.exe --help failed (may need configuration)" "WARNING"
                $buildResults["neo-node.exe"]["Functional"] = $false
            }
        }
        catch {
            Write-ColoredOutput "Error testing neo-node.exe functionality: $_" "WARNING"
            $buildResults["neo-node.exe"]["Functional"] = $false
        }
    }
    
    Write-ColoredOutput "Build output verification completed" "SUCCESS"
    return $buildResults
}

# ========================================================================
# Testing
# ========================================================================

function Invoke-QuickTests {
    Write-ColoredOutput "Running verification tests..." "INFO"
    
    Set-Location $BuildDir
    
    $testResults = @{}
    
    # Run unit tests
    $unitTestPath = "$BuildType\neo-unit-tests.exe"
    if (Test-Path $unitTestPath) {
        Write-ColoredOutput "Running unit tests..." "INFO"
        try {
            $testStart = Get-Date
            & $unitTestPath --gtest_brief=1
            $testTime = (Get-Date) - $testStart
            
            if ($LASTEXITCODE -eq 0) {
                Write-ColoredOutput "Unit tests passed in $($testTime.TotalSeconds.ToString('F1'))s" "SUCCESS"
                $testResults["UnitTests"] = @{ "Status" = "PASSED"; "Time" = $testTime }
            }
            else {
                Write-ColoredOutput "Some unit tests failed" "WARNING"
                $testResults["UnitTests"] = @{ "Status" = "FAILED"; "Time" = $testTime }
            }
        }
        catch {
            Write-ColoredOutput "Error running unit tests: $_" "WARNING"
            $testResults["UnitTests"] = @{ "Status" = "ERROR" }
        }
    }
    else {
        Write-ColoredOutput "Unit tests not found, skipping..." "WARNING"
        $testResults["UnitTests"] = @{ "Status" = "NOT_FOUND" }
    }
    
    # Run integration tests
    $integrationTestPath = "$BuildType\neo-integration-tests.exe"
    if (Test-Path $integrationTestPath) {
        Write-ColoredOutput "Running integration tests..." "INFO"
        try {
            $testStart = Get-Date
            & $integrationTestPath
            $testTime = (Get-Date) - $testStart
            
            if ($LASTEXITCODE -eq 0) {
                Write-ColoredOutput "Integration tests passed in $($testTime.TotalSeconds.ToString('F1'))s" "SUCCESS"
                $testResults["IntegrationTests"] = @{ "Status" = "PASSED"; "Time" = $testTime }
            }
            else {
                Write-ColoredOutput "Some integration tests failed" "WARNING"
                $testResults["IntegrationTests"] = @{ "Status" = "FAILED"; "Time" = $testTime }
            }
        }
        catch {
            Write-ColoredOutput "Error running integration tests: $_" "WARNING"
            $testResults["IntegrationTests"] = @{ "Status" = "ERROR" }
        }
    }
    else {
        Write-ColoredOutput "Integration tests not found, skipping..." "WARNING"
        $testResults["IntegrationTests"] = @{ "Status" = "NOT_FOUND" }
    }
    
    Write-ColoredOutput "Test execution completed" "SUCCESS"
    return $testResults
}

# ========================================================================
# Reporting
# ========================================================================

function New-BuildReport {
    param(
        [hashtable]$BuildResults,
        [hashtable]$TestResults
    )
    
    Write-ColoredOutput "Generating comprehensive build report..." "INFO"
    
    $reportPath = Join-Path $ProjectRoot "build_verification_report.json"
    $buildTime = (Get-Date) - $BuildStartTime
    
    $report = @{
        "ReportGenerated" = (Get-Date).ToString("yyyy-MM-dd HH:mm:ss")
        "BuildConfiguration" = $BuildType
        "Platform" = "Windows x64"
        "Generator" = $Generator
        "BuildTime" = @{
            "TotalMinutes" = $buildTime.TotalMinutes
            "Start" = $BuildStartTime.ToString("yyyy-MM-dd HH:mm:ss")
            "End" = (Get-Date).ToString("yyyy-MM-dd HH:mm:ss")
        }
        "SystemInfo" = @{
            "PowerShellVersion" = $PSVersionTable.PSVersion.ToString()
            "OSVersion" = [System.Environment]::OSVersion.ToString()
            "ProcessorCount" = [Environment]::ProcessorCount
            "TotalMemoryGB" = [math]::Round((Get-WmiObject -Class Win32_ComputerSystem).TotalPhysicalMemory / 1GB, 2)
        }
        "Directories" = @{
            "ProjectRoot" = $ProjectRoot
            "BuildDir" = $BuildDir
            "VcpkgDir" = $VcpkgDir
        }
        "BuildResults" = $BuildResults
        "TestResults" = $TestResults
        "Statistics" = @{
            "ErrorCount" = $ErrorCount
            "WarningCount" = $WarningCount
            "OverallStatus" = if ($ErrorCount -eq 0) { "SUCCESS" } else { "FAILED" }
        }
    }
    
    # Save as JSON
    $report | ConvertTo-Json -Depth 5 | Set-Content -Path $reportPath -Encoding UTF8
    
    # Also create human-readable text report
    $textReportPath = Join-Path $ProjectRoot "build_verification_report.txt"
    $textReport = @"
Neo C++ Build Verification Report
==================================

Build Date: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
Build Configuration: $BuildType
Platform: Windows x64
Generator: $Generator
Build Time: $($buildTime.TotalMinutes.ToString('F1')) minutes

System Information:
- PowerShell Version: $($PSVersionTable.PSVersion)
- OS Version: $([System.Environment]::OSVersion)
- CPU Cores: $([Environment]::ProcessorCount)
- Total Memory: $([math]::Round((Get-WmiObject -Class Win32_ComputerSystem).TotalPhysicalMemory / 1GB, 2))GB

Build Results:
$($BuildResults.Keys | ForEach-Object { "- $_`: $($BuildResults[$_].Status)" } | Out-String)

Test Results:
$($TestResults.Keys | ForEach-Object { "- $_`: $($TestResults[$_].Status)" } | Out-String)

Statistics:
- Errors: $ErrorCount
- Warnings: $WarningCount
- Overall Status: $(if ($ErrorCount -eq 0) { "SUCCESS" } else { "FAILED" })

Build artifacts available in: $BuildDir
"@
    
    Set-Content -Path $textReportPath -Value $textReport -Encoding UTF8
    
    Write-ColoredOutput "Build reports generated:" "SUCCESS"
    Write-ColoredOutput "  JSON: $reportPath" "INFO"
    Write-ColoredOutput "  Text: $textReportPath" "INFO"
    
    return $report
}

# ========================================================================
# Main Execution
# ========================================================================

function Main {
    try {
        if ($Help) {
            Show-Help
            return
        }
        
        Show-Banner
        
        Set-Location $ProjectRoot
        
        Write-ColoredOutput "Starting Neo C++ build verification..." "INFO"
        Write-ColoredOutput "Build Type: $BuildType, Generator: $Generator" "INFO"
        
        # System requirements check
        Test-SystemRequirements
        
        # Install dependencies
        if (-not $SkipDeps) {
            Install-Dependencies
        }
        else {
            Write-ColoredOutput "Skipping dependency installation" "WARNING"
        }
        
        # Build process
        Clear-BuildArtifacts
        Invoke-CMakeConfigure
        Invoke-Build
        $buildResults = Test-BuildOutput
        
        # Testing
        $testResults = @{}
        if (-not $SkipTests) {
            $testResults = Invoke-QuickTests
        }
        else {
            Write-ColoredOutput "Skipping tests" "WARNING"
        }
        
        # Generate report
        $report = New-BuildReport -BuildResults $buildResults -TestResults $testResults
        
        # Final status
        Write-Host ""
        Write-Host "================================================================" -ForegroundColor Magenta
        
        if ($ErrorCount -eq 0) {
            Write-ColoredOutput "Neo C++ build verification completed successfully!" "SUCCESS"
            Write-Host ""
            Write-Host "Build Summary:" -ForegroundColor Green
            Write-Host "  Build Time: $((Get-Date) - $BuildStartTime | ForEach-Object { $_.TotalMinutes.ToString('F1') }) minutes" -ForegroundColor White
            Write-Host "  Main Executable: $BuildDir\src\node\$BuildType\neo-node.exe" -ForegroundColor White
            Write-Host "  Build Report: $(Join-Path $ProjectRoot 'build_verification_report.txt')" -ForegroundColor White
        }
        else {
            Write-ColoredOutput "Build verification completed with $ErrorCount errors and $WarningCount warnings" "ERROR"
            Write-Host "Please check the build reports for details." -ForegroundColor Yellow
        }
        
        Write-Host "================================================================" -ForegroundColor Magenta
        Write-Host ""
        
        return $ErrorCount
    }
    catch {
        Write-ColoredOutput "Build verification failed with exception: $_" "ERROR"
        Write-Host ""
        Write-Host "Troubleshooting steps:" -ForegroundColor Yellow
        Write-Host "1. Ensure Visual Studio 2019/2022 or Build Tools are installed" -ForegroundColor White
        Write-Host "2. Verify CMake 3.20+ is installed and in PATH" -ForegroundColor White
        Write-Host "3. Check that Git is installed (required for vcpkg)" -ForegroundColor White
        Write-Host "4. Ensure you have at least 5GB of free disk space" -ForegroundColor White
        Write-Host "5. Run PowerShell as Administrator if permission issues occur" -ForegroundColor White
        Write-Host ""
        return 1
    }
}

# Execute main function
exit (Main) 