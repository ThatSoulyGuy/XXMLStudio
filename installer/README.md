# XXMLStudio Installer Build System

This directory contains scripts and configuration files for building installers on different platforms.

## Directory Structure

```
installer/
├── README.md           # This file
├── macos/
│   ├── build-installer.sh    # macOS DMG builder script
│   ├── .env.example          # Example configuration (copy to .env.local)
│   ├── .env.local            # Local config with credentials (gitignored)
│   ├── entitlements.plist    # Code signing entitlements
│   └── Info.plist.in         # App bundle Info.plist template
├── windows/
│   ├── build-installer.ps1   # PowerShell build script
│   ├── build-installer.bat   # Batch wrapper for PowerShell
│   └── XXMLStudio.iss        # Inno Setup script
└── linux/
    ├── build-installer.sh    # Linux AppImage/DEB/RPM builder
    └── xxmlstudio.desktop    # Desktop entry file
```

## Prerequisites

### All Platforms
- CMake 3.16+
- Qt 6.10.1 or later
- C++17 compatible compiler

### macOS
- Xcode Command Line Tools
- (Optional) Developer ID certificate for code signing
- (Optional) Apple Developer account for notarization

### Windows
- Visual Studio 2019/2022 or MinGW
- (Optional) Inno Setup 6 for creating installer EXE
- (Optional) Code signing certificate

### Linux
- GCC 9+ or Clang 10+
- (Optional) linuxdeployqt or linuxdeploy for AppImage creation
- (Optional) rpmbuild for RPM packages
- (Optional) dpkg-deb for DEB packages

## Building Installers

### macOS

First, configure your settings:
```bash
cd installer/macos
cp .env.example .env.local
# Edit .env.local with your Qt path and signing credentials
```

Then build:
```bash
./build-installer.sh
```

Command-line options:
- `--skip-build` - Skip building, use existing build
- `--skip-sign` - Skip code signing
- `--qt-dir DIR` - Override Qt installation path
- `--version VER` - Override version string

Configuration via `.env.local`:
```bash
QT_DIR="${HOME}/Qt/6.10.1/macos"     # Qt installation path
VERSION="0.1.0"                      # Version string

# Code signing (optional)
DEVELOPER_ID="Developer ID Application: Your Name (TEAMID)"

# Notarization (optional, requires signing)
APPLE_ID="your@email.com"
APPLE_PASSWORD="xxxx-xxxx-xxxx-xxxx" # App-specific password
TEAM_ID="XXXXXXXXXX"                 # 10-character Team ID

# Skip flags
SKIP_SIGNING=0                       # Set to 1 to skip signing
SKIP_NOTARIZATION=0                  # Set to 1 to skip notarization
```

Output: `installer/macos/out/XXMLStudio-X.Y.Z-macOS.dmg`

### Windows

```powershell
cd installer\windows
.\build-installer.ps1
```

Or using the batch file:
```cmd
cd installer\windows
build-installer.bat
```

Options:
- `-QtDir DIR` - Path to Qt installation (default: C:\Qt\6.10.1\mingw_64)
- `-Version VER` - Version string (default: 0.1.0)
- `-SkipBuild` - Skip building, use existing build
- `-SkipInstaller` - Only deploy, don't create installer

Output: `installer/windows/out/XXMLStudio-X.Y.Z-Setup.exe`

### Linux

```bash
cd installer/linux
./build-installer.sh
```

Options:
- `--skip-build` - Skip building, use existing build
- `--qt-dir DIR` - Path to Qt installation (default: /opt/Qt/6.10.1/gcc_64)
- `--version VER` - Version string (default: 0.1.0)

Output:
- `installer/linux/out/XXMLStudio-X.Y.Z-x86_64.AppImage`
- `installer/linux/out/XXMLStudio-X.Y.Z.deb`
- `installer/linux/out/XXMLStudio-X.Y.Z.rpm`

## Using CMake/CPack

You can also use CPack directly:

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/path/to/Qt

# Build
cmake --build build --config Release

# Package (creates platform-appropriate installer)
cd build
cpack
```

## Qt Libraries Included

The installers bundle the following Qt modules:
- Qt6Core
- Qt6Gui
- Qt6Widgets
- Qt6Network
- Qt6Concurrent
- Qt6Svg

Plus required plugins:
- platforms (cocoa/windows/xcb)
- imageformats (png, jpeg, svg, etc.)
- iconengines (svg icon support)
- styles (platform styles)

## Code Signing

### macOS

Configure signing in `.env.local`:
```bash
# Find your certificate name:
security find-identity -v -p codesigning

# Add to .env.local:
DEVELOPER_ID="Developer ID Application: Your Name (XXXXXXXXXX)"
```

For notarization, also add:
```bash
APPLE_ID="your@email.com"
APPLE_PASSWORD="xxxx-xxxx-xxxx-xxxx"  # App-specific password from appleid.apple.com
TEAM_ID="XXXXXXXXXX"                   # From developer.apple.com/account
```

### Windows

Sign after building with signtool:
```cmd
signtool sign /f certificate.pfx /p password /t http://timestamp.digicert.com installer\windows\out\XXMLStudio-Setup.exe
```

## Troubleshooting

### macOS: "App is damaged and can't be opened"
This happens when the app is not signed or notarized. Either:
1. Sign and notarize the app using the build script with proper credentials
2. Remove the quarantine attribute: `xattr -cr XXMLStudio.app`

### Windows: Missing DLLs
If the app fails to start with missing DLL errors:
1. Ensure windeployqt ran successfully
2. Check that all Qt plugins are copied
3. Install Visual C++ Redistributable

### Linux: App won't start
Check library dependencies:
```bash
ldd XXMLStudio
```

Ensure Qt platform plugin is found:
```bash
export QT_DEBUG_PLUGINS=1
./XXMLStudio
```

## Version History

- 0.1.0 - Initial release
