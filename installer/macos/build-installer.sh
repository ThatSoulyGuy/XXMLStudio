#!/bin/bash
#
# macOS Installer Build Script for XXMLStudio
# Creates a deployable .app bundle and .dmg installer
#

set -e

# Script directory
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# =============================================================================
# Load environment configuration
# =============================================================================

# Load .env.local if it exists (contains sensitive credentials)
if [ -f "$SCRIPT_DIR/.env.local" ]; then
    echo "Loading configuration from .env.local..."
    set -a  # automatically export all variables
    source "$SCRIPT_DIR/.env.local"
    set +a
fi

# =============================================================================
# Configuration (can be overridden by .env.local or command line)
# =============================================================================

APP_NAME="XXMLStudio"
VERSION="${VERSION:-0.1.0}"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
INSTALLER_OUT="$SCRIPT_DIR/out"
QT_DIR="${QT_DIR:-$HOME/Qt/6.10.1/macos}"

# Signing and notarization flags
SKIP_SIGNING="${SKIP_SIGNING:-0}"
SKIP_NOTARIZATION="${SKIP_NOTARIZATION:-0}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check prerequisites
check_prerequisites() {
    log_info "Checking prerequisites..."

    if [ ! -d "$QT_DIR" ]; then
        log_error "Qt directory not found: $QT_DIR"
        log_error "Set QT_DIR environment variable to your Qt installation"
        exit 1
    fi

    MACDEPLOYQT="$QT_DIR/bin/macdeployqt"
    if [ ! -f "$MACDEPLOYQT" ]; then
        log_error "macdeployqt not found: $MACDEPLOYQT"
        exit 1
    fi

    if ! command -v cmake &> /dev/null; then
        log_error "cmake not found in PATH"
        exit 1
    fi

    if ! command -v hdiutil &> /dev/null; then
        log_error "hdiutil not found (required for DMG creation)"
        exit 1
    fi

    log_info "All prerequisites satisfied"
}

# Build the application
build_app() {
    log_info "Building $APP_NAME..."

    cd "$PROJECT_ROOT"

    # Configure with Release build
    cmake -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="$QT_DIR"

    # Build
    cmake --build build --config Release

    if [ ! -d "$BUILD_DIR/$APP_NAME.app" ]; then
        log_error "Build failed: $APP_NAME.app not found"
        exit 1
    fi

    log_info "Build completed successfully"
}

# Deploy Qt frameworks
deploy_qt() {
    log_info "Deploying Qt frameworks..."

    APP_BUNDLE="$BUILD_DIR/$APP_NAME.app"

    # Run macdeployqt
    "$MACDEPLOYQT" "$APP_BUNDLE" \
        -verbose=2 \
        -always-overwrite

    log_info "Qt frameworks deployed"
}

# Sign the application (optional, requires Developer ID)
sign_app() {
    if [ "$SKIP_SIGNING" = "1" ]; then
        log_warn "Code signing skipped (SKIP_SIGNING=1)"
        return 0
    fi

    if [ -z "$DEVELOPER_ID" ]; then
        log_warn "DEVELOPER_ID not set in .env.local, skipping code signing"
        log_warn "Copy .env.example to .env.local and configure to enable signing"
        return 0
    fi

    log_info "Signing application with Developer ID: $DEVELOPER_ID"

    APP_BUNDLE="$BUILD_DIR/$APP_NAME.app"

    # Clean up temporary/corrupted files created by macdeployqt
    # These files have random suffixes and cause signing to fail
    log_info "Cleaning up temporary files from macdeployqt..."
    find "$APP_BUNDLE" -type f \( -name "*.dylib.*" -o -name "Qt*.*????*" \) -delete 2>/dev/null || true
    find "$APP_BUNDLE/Contents/Frameworks" -type f ! -name "*.dylib" ! -name "Qt*" ! -name "Info.plist" ! -name "Resources" -name "*.*" | grep -E '\.[A-Za-z0-9]{5}$' | xargs rm -f 2>/dev/null || true

    # Remove any broken symlinks
    find "$APP_BUNDLE" -type l ! -exec test -e {} \; -delete 2>/dev/null || true

    # Sign frameworks from the inside out (sign nested content first)
    log_info "Signing frameworks..."
    find "$APP_BUNDLE/Contents/Frameworks" -name "*.framework" -type d | while read -r framework; do
        # Sign the framework binary inside Versions/A/ or Versions/Current/
        framework_name=$(basename "$framework" .framework)
        framework_binary="$framework/Versions/A/$framework_name"
        if [ -f "$framework_binary" ]; then
            codesign --force --sign "$DEVELOPER_ID" \
                --options runtime \
                --timestamp \
                "$framework_binary"
        fi
        # Sign the framework itself
        codesign --force --sign "$DEVELOPER_ID" \
            --options runtime \
            --timestamp \
            "$framework"
    done

    # Sign dylibs
    log_info "Signing dylibs..."
    find "$APP_BUNDLE" -name "*.dylib" -type f | while read -r dylib; do
        codesign --force --sign "$DEVELOPER_ID" \
            --options runtime \
            --timestamp \
            "$dylib"
    done

    # Sign plugins
    log_info "Signing plugins..."
    find "$APP_BUNDLE/Contents/PlugIns" -type f -perm +111 2>/dev/null | while read -r plugin; do
        codesign --force --sign "$DEVELOPER_ID" \
            --options runtime \
            --timestamp \
            "$plugin"
    done

    # Sign the main executable
    log_info "Signing main executable..."
    codesign --force --sign "$DEVELOPER_ID" \
        --options runtime \
        --timestamp \
        --entitlements "$SCRIPT_DIR/entitlements.plist" \
        "$APP_BUNDLE/Contents/MacOS/$APP_NAME"

    # Sign the entire app bundle
    log_info "Signing app bundle..."
    codesign --force --sign "$DEVELOPER_ID" \
        --options runtime \
        --timestamp \
        --entitlements "$SCRIPT_DIR/entitlements.plist" \
        "$APP_BUNDLE"

    # Verify the signature
    log_info "Verifying signature..."
    codesign --verify --deep --strict --verbose=2 "$APP_BUNDLE"

    log_info "Application signed successfully"
}

# Create DMG installer
create_dmg() {
    log_info "Creating DMG installer..."

    mkdir -p "$INSTALLER_OUT"

    APP_BUNDLE="$BUILD_DIR/$APP_NAME.app"
    DMG_NAME="$APP_NAME-$VERSION-macOS"
    DMG_PATH="$INSTALLER_OUT/$DMG_NAME.dmg"
    DMG_TEMP="$INSTALLER_OUT/$DMG_NAME-temp.dmg"
    VOLUME_NAME="$APP_NAME $VERSION"

    # Remove old DMG if exists
    rm -f "$DMG_PATH" "$DMG_TEMP"

    # Create temporary directory for DMG contents
    DMG_CONTENTS="$INSTALLER_OUT/dmg-contents"
    rm -rf "$DMG_CONTENTS"
    mkdir -p "$DMG_CONTENTS"

    # Copy app bundle
    cp -R "$APP_BUNDLE" "$DMG_CONTENTS/"

    # Create Applications symlink
    ln -s /Applications "$DMG_CONTENTS/Applications"

    # Create temporary DMG
    hdiutil create -srcfolder "$DMG_CONTENTS" \
        -volname "$VOLUME_NAME" \
        -fs HFS+ \
        -fsargs "-c c=64,a=16,e=16" \
        -format UDRW \
        "$DMG_TEMP"

    # Mount the temporary DMG
    MOUNT_DIR="/Volumes/$VOLUME_NAME"
    hdiutil attach "$DMG_TEMP" -mountpoint "$MOUNT_DIR"

    # Set DMG window appearance (optional customization)
    # This uses AppleScript to set window position and size
    osascript << EOF
tell application "Finder"
    tell disk "$VOLUME_NAME"
        open
        set current view of container window to icon view
        set toolbar visible of container window to false
        set statusbar visible of container window to false
        set bounds of container window to {400, 100, 900, 450}
        set viewOptions to the icon view options of container window
        set arrangement of viewOptions to not arranged
        set icon size of viewOptions to 80
        set position of item "$APP_NAME.app" of container window to {125, 175}
        set position of item "Applications" of container window to {375, 175}
        close
        open
        update without registering applications
        delay 2
    end tell
end tell
EOF

    # Sync and unmount
    sync
    hdiutil detach "$MOUNT_DIR"

    # Convert to compressed DMG
    hdiutil convert "$DMG_TEMP" \
        -format UDZO \
        -imagekey zlib-level=9 \
        -o "$DMG_PATH"

    # Cleanup
    rm -f "$DMG_TEMP"
    rm -rf "$DMG_CONTENTS"

    log_info "DMG created: $DMG_PATH"
}

# Notarize the DMG (optional, requires Apple Developer account)
notarize_dmg() {
    if [ "$SKIP_NOTARIZATION" = "1" ]; then
        log_warn "Notarization skipped (SKIP_NOTARIZATION=1)"
        return 0
    fi

    if [ -z "$APPLE_ID" ] || [ -z "$APPLE_PASSWORD" ] || [ -z "$TEAM_ID" ]; then
        log_warn "Apple credentials not set in .env.local, skipping notarization"
        log_warn "Copy .env.example to .env.local and configure to enable notarization"
        return 0
    fi

    log_info "Notarizing DMG..."

    DMG_PATH="$INSTALLER_OUT/$APP_NAME-$VERSION-macOS.dmg"

    # Submit for notarization
    xcrun notarytool submit "$DMG_PATH" \
        --apple-id "$APPLE_ID" \
        --password "$APPLE_PASSWORD" \
        --team-id "$TEAM_ID" \
        --wait

    # Staple the notarization ticket
    xcrun stapler staple "$DMG_PATH"

    log_info "DMG notarized and stapled"
}

# Main
main() {
    echo ""
    echo "=========================================="
    echo "  $APP_NAME macOS Installer Builder"
    echo "  Version: $VERSION"
    echo "=========================================="
    echo ""

    check_prerequisites
    build_app
    deploy_qt
    sign_app
    create_dmg
    notarize_dmg

    echo ""
    log_info "Build completed successfully!"
    log_info "Installer: $INSTALLER_OUT/$APP_NAME-$VERSION-macOS.dmg"
    echo ""
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --skip-build)
            SKIP_BUILD=1
            shift
            ;;
        --skip-sign)
            SKIP_SIGN=1
            shift
            ;;
        --qt-dir)
            QT_DIR="$2"
            shift 2
            ;;
        --version)
            VERSION="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  --skip-build    Skip building the application"
            echo "  --skip-sign     Skip code signing"
            echo "  --qt-dir DIR    Path to Qt installation"
            echo "  --version VER   Version string for the installer"
            echo "  -h, --help      Show this help message"
            echo ""
            echo "Configuration:"
            echo "  Copy .env.example to .env.local and configure your settings."
            echo "  .env.local is gitignored and safe for credentials."
            echo ""
            echo "  Settings in .env.local:"
            echo "    QT_DIR          Path to Qt installation (default: ~/Qt/6.10.1/macos)"
            echo "    DEVELOPER_ID    Developer ID certificate for code signing"
            echo "    APPLE_ID        Apple ID for notarization"
            echo "    APPLE_PASSWORD  App-specific password for notarization"
            echo "    TEAM_ID         Team ID for notarization"
            echo "    VERSION         Version string (default: 0.1.0)"
            echo "    SKIP_SIGNING    Set to 1 to skip code signing"
            echo "    SKIP_NOTARIZATION  Set to 1 to skip notarization"
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            exit 1
            ;;
    esac
done

main
