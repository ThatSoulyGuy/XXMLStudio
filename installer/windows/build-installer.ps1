# Windows Installer Build Script for XXMLStudio
# Creates a deployable package and Inno Setup installer
#
# Prerequisites:
# - Qt 6.x with MinGW or MSVC
# - CMake
# - Ninja (optional)
# - Inno Setup 6 (for installer creation)

param(
    [string]$QtDir = "C:\Qt\6.10.1\mingw_64",
    [string]$Version = "0.1.0",
    [switch]$SkipBuild,
    [switch]$SkipInstaller,
    [switch]$Help
)

$ErrorActionPreference = "Stop"

# Configuration
$AppName = "XXMLStudio"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = (Get-Item "$ScriptDir\..\..").FullName
$BuildDir = "$ProjectRoot\build"
$InstallerOut = "$ScriptDir\out"
$DeployDir = "$InstallerOut\deploy"

function Write-Info {
    param([string]$Message)
    Write-Host "[INFO] $Message" -ForegroundColor Green
}

function Write-Warn {
    param([string]$Message)
    Write-Host "[WARN] $Message" -ForegroundColor Yellow
}

function Write-Error {
    param([string]$Message)
    Write-Host "[ERROR] $Message" -ForegroundColor Red
}

function Show-Help {
    Write-Host ""
    Write-Host "XXMLStudio Windows Installer Builder"
    Write-Host ""
    Write-Host "Usage: .\build-installer.ps1 [options]"
    Write-Host ""
    Write-Host "Options:"
    Write-Host "  -QtDir <path>      Path to Qt installation (default: C:\Qt\6.10.1\mingw_64)"
    Write-Host "  -Version <ver>     Version string for the installer (default: 0.1.0)"
    Write-Host "  -SkipBuild         Skip building the application"
    Write-Host "  -SkipInstaller     Skip creating the installer (just deploy)"
    Write-Host "  -Help              Show this help message"
    Write-Host ""
    exit 0
}

function Test-Prerequisites {
    Write-Info "Checking prerequisites..."

    # Check Qt directory
    if (-not (Test-Path $QtDir)) {
        Write-Error "Qt directory not found: $QtDir"
        Write-Error "Set -QtDir parameter to your Qt installation"
        exit 1
    }

    # Check windeployqt
    $script:WinDeployQt = "$QtDir\bin\windeployqt.exe"
    if (-not (Test-Path $WinDeployQt)) {
        # Try windeployqt6
        $script:WinDeployQt = "$QtDir\bin\windeployqt6.exe"
        if (-not (Test-Path $WinDeployQt)) {
            Write-Error "windeployqt not found in: $QtDir\bin"
            exit 1
        }
    }

    # Check CMake
    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        Write-Error "cmake not found in PATH"
        exit 1
    }

    # Check Inno Setup (optional)
    $script:InnoSetup = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
    if (-not (Test-Path $InnoSetup)) {
        $script:InnoSetup = "C:\Program Files\Inno Setup 6\ISCC.exe"
        if (-not (Test-Path $InnoSetup)) {
            Write-Warn "Inno Setup not found. Installer creation will be skipped."
            Write-Warn "Download from: https://jrsoftware.org/isinfo.php"
            $script:InnoSetup = $null
        }
    }

    Write-Info "All prerequisites satisfied"
}

function Build-App {
    Write-Info "Building $AppName..."

    Set-Location $ProjectRoot

    # Determine generator (prefer Ninja if available)
    $Generator = "Ninja"
    if (-not (Get-Command ninja -ErrorAction SilentlyContinue)) {
        $Generator = "MinGW Makefiles"
        Write-Warn "Ninja not found, using MinGW Makefiles"
    }

    # Configure
    & cmake -B build -G $Generator `
        -DCMAKE_BUILD_TYPE=Release `
        -DCMAKE_PREFIX_PATH="$QtDir"

    if ($LASTEXITCODE -ne 0) {
        Write-Error "CMake configuration failed"
        exit 1
    }

    # Build
    & cmake --build build --config Release

    if ($LASTEXITCODE -ne 0) {
        Write-Error "Build failed"
        exit 1
    }

    $ExePath = "$BuildDir\$AppName.exe"
    if (-not (Test-Path $ExePath)) {
        Write-Error "Build failed: $AppName.exe not found"
        exit 1
    }

    Write-Info "Build completed successfully"
}

function Deploy-Qt {
    Write-Info "Deploying Qt libraries..."

    # Create deployment directory
    if (Test-Path $DeployDir) {
        Remove-Item -Recurse -Force $DeployDir
    }
    New-Item -ItemType Directory -Path $DeployDir -Force | Out-Null

    # Copy executable
    $ExePath = "$BuildDir\$AppName.exe"
    Copy-Item $ExePath $DeployDir

    # Run windeployqt
    Write-Info "Running windeployqt..."
    & $WinDeployQt `
        --release `
        --no-translations `
        --no-system-d3d-compiler `
        --no-opengl-sw `
        --dir $DeployDir `
        "$DeployDir\$AppName.exe"

    if ($LASTEXITCODE -ne 0) {
        Write-Error "windeployqt failed"
        exit 1
    }

    # Copy additional files
    Write-Info "Copying additional files..."

    # Copy toolchain if it exists
    $ToolchainSrc = "$ProjectRoot\toolchain"
    if (Test-Path $ToolchainSrc) {
        Write-Info "Copying bundled toolchain..."
        Copy-Item -Recurse $ToolchainSrc "$DeployDir\toolchain"
    }

    # Copy any resource files
    $ResourcesSrc = "$ProjectRoot\resources"
    if (Test-Path "$ResourcesSrc\themes") {
        # Themes are embedded in resources.qrc, no need to copy
    }

    Write-Info "Qt deployment completed"
}

function Create-Installer {
    if (-not $InnoSetup) {
        Write-Warn "Skipping installer creation (Inno Setup not found)"
        return
    }

    Write-Info "Creating installer with Inno Setup..."

    # Generate Inno Setup script with current paths
    $IssContent = Get-Content "$ScriptDir\XXMLStudio.iss" -Raw
    $IssContent = $IssContent -replace '\$\{VERSION\}', $Version
    $IssContent = $IssContent -replace '\$\{DEPLOY_DIR\}', $DeployDir
    $IssContent = $IssContent -replace '\$\{OUTPUT_DIR\}', $InstallerOut

    $TempIss = "$InstallerOut\XXMLStudio-temp.iss"
    $IssContent | Out-File -FilePath $TempIss -Encoding UTF8

    # Run Inno Setup compiler
    & $InnoSetup $TempIss

    if ($LASTEXITCODE -ne 0) {
        Write-Error "Inno Setup compilation failed"
        exit 1
    }

    # Cleanup temp file
    Remove-Item $TempIss

    Write-Info "Installer created: $InstallerOut\$AppName-$Version-Setup.exe"
}

function Main {
    Write-Host ""
    Write-Host "=========================================="
    Write-Host "  $AppName Windows Installer Builder"
    Write-Host "  Version: $Version"
    Write-Host "=========================================="
    Write-Host ""

    if ($Help) {
        Show-Help
    }

    # Create output directory
    if (-not (Test-Path $InstallerOut)) {
        New-Item -ItemType Directory -Path $InstallerOut -Force | Out-Null
    }

    Test-Prerequisites

    if (-not $SkipBuild) {
        Build-App
    }

    Deploy-Qt

    if (-not $SkipInstaller) {
        Create-Installer
    }

    Write-Host ""
    Write-Info "Build completed successfully!"
    Write-Info "Deployed files: $DeployDir"
    if (-not $SkipInstaller -and $InnoSetup) {
        Write-Info "Installer: $InstallerOut\$AppName-$Version-Setup.exe"
    }
    Write-Host ""
}

Main
