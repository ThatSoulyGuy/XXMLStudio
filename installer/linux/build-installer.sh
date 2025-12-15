#!/bin/bash
#
# Linux Installer Build Script for XXMLStudio
# Creates DEB, RPM, and AppImage packages
#

set -e

# Configuration
APP_NAME="XXMLStudio"
VERSION="0.1.0"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
INSTALLER_OUT="$SCRIPT_DIR/out"
QT_DIR="${QT_DIR:-/opt/Qt/6.10.1/gcc_64}"

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

    if ! command -v cmake &> /dev/null; then
        log_error "cmake not found in PATH"
        exit 1
    fi

    # Check for linuxdeployqt or alternative tools
    if command -v linuxdeployqt &> /dev/null; then
        DEPLOY_TOOL="linuxdeployqt"
    elif command -v linuxdeploy &> /dev/null; then
        DEPLOY_TOOL="linuxdeploy"
    else
        log_warn "linuxdeployqt/linuxdeploy not found"
        log_warn "Download from: https://github.com/probonopd/linuxdeployqt"
        DEPLOY_TOOL=""
    fi

    log_info "All prerequisites satisfied"
}

# Build the application
build_app() {
    log_info "Building $APP_NAME..."

    cd "$PROJECT_ROOT"

    # Determine generator
    if command -v ninja &> /dev/null; then
        GENERATOR="Ninja"
    else
        GENERATOR="Unix Makefiles"
    fi

    # Configure with Release build
    cmake -B build -G "$GENERATOR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="$QT_DIR" \
        -DCMAKE_INSTALL_PREFIX=/usr

    # Build
    cmake --build build --config Release

    if [ ! -f "$BUILD_DIR/$APP_NAME" ]; then
        log_error "Build failed: $APP_NAME not found"
        exit 1
    fi

    log_info "Build completed successfully"
}

# Create AppDir structure
create_appdir() {
    log_info "Creating AppDir structure..."

    APPDIR="$INSTALLER_OUT/$APP_NAME.AppDir"

    # Clean previous
    rm -rf "$APPDIR"
    mkdir -p "$APPDIR/usr/bin"
    mkdir -p "$APPDIR/usr/lib"
    mkdir -p "$APPDIR/usr/share/applications"
    mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"

    # Copy executable
    cp "$BUILD_DIR/$APP_NAME" "$APPDIR/usr/bin/"

    # Copy desktop file
    cp "$SCRIPT_DIR/xxmlstudio.desktop" "$APPDIR/usr/share/applications/"
    cp "$SCRIPT_DIR/xxmlstudio.desktop" "$APPDIR/"

    # Copy icon
    if [ -f "$PROJECT_ROOT/resources/icons/app-icon.png" ]; then
        cp "$PROJECT_ROOT/resources/icons/app-icon.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/xxmlstudio.png"
        cp "$PROJECT_ROOT/resources/icons/app-icon.png" "$APPDIR/xxmlstudio.png"
    fi

    # Copy bundled toolchain if exists
    if [ -d "$PROJECT_ROOT/toolchain" ]; then
        log_info "Copying bundled toolchain..."
        mkdir -p "$APPDIR/usr/lib/xxml"
        cp -R "$PROJECT_ROOT/toolchain"/* "$APPDIR/usr/lib/xxml/"
    fi

    log_info "AppDir structure created"
}

# Deploy Qt libraries
deploy_qt() {
    log_info "Deploying Qt libraries..."

    APPDIR="$INSTALLER_OUT/$APP_NAME.AppDir"

    if [ -n "$DEPLOY_TOOL" ]; then
        if [ "$DEPLOY_TOOL" = "linuxdeployqt" ]; then
            linuxdeployqt "$APPDIR/usr/share/applications/xxmlstudio.desktop" \
                -qmake="$QT_DIR/bin/qmake" \
                -bundle-non-qt-libs \
                -no-translations
        elif [ "$DEPLOY_TOOL" = "linuxdeploy" ]; then
            linuxdeploy --appdir "$APPDIR" \
                --plugin qt \
                --desktop-file "$APPDIR/usr/share/applications/xxmlstudio.desktop" \
                --icon-file "$APPDIR/xxmlstudio.png"
        fi
    else
        log_warn "No deployment tool available, manually copying Qt libs..."

        # Copy Qt libraries manually
        QT_LIBS="libQt6Core.so.6 libQt6Gui.so.6 libQt6Widgets.so.6 libQt6Network.so.6 libQt6Concurrent.so.6 libQt6Svg.so.6 libQt6DBus.so.6"

        for lib in $QT_LIBS; do
            if [ -f "$QT_DIR/lib/$lib" ]; then
                cp "$QT_DIR/lib/$lib" "$APPDIR/usr/lib/"
            fi
        done

        # Copy Qt plugins
        mkdir -p "$APPDIR/usr/plugins"
        cp -R "$QT_DIR/plugins/platforms" "$APPDIR/usr/plugins/"
        cp -R "$QT_DIR/plugins/imageformats" "$APPDIR/usr/plugins/"
        cp -R "$QT_DIR/plugins/iconengines" "$APPDIR/usr/plugins/"

        # Create qt.conf
        cat > "$APPDIR/usr/bin/qt.conf" << EOF
[Paths]
Prefix = ..
Plugins = plugins
EOF
    fi

    log_info "Qt deployment completed"
}

# Create AppImage
create_appimage() {
    log_info "Creating AppImage..."

    APPDIR="$INSTALLER_OUT/$APP_NAME.AppDir"

    # Download appimagetool if not present
    APPIMAGETOOL="$INSTALLER_OUT/appimagetool"
    if [ ! -f "$APPIMAGETOOL" ]; then
        log_info "Downloading appimagetool..."
        wget -q "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage" \
            -O "$APPIMAGETOOL"
        chmod +x "$APPIMAGETOOL"
    fi

    # Create AppRun script
    cat > "$APPDIR/AppRun" << 'EOF'
#!/bin/bash
SELF=$(readlink -f "$0")
HERE=${SELF%/*}
export PATH="${HERE}/usr/bin:${PATH}"
export LD_LIBRARY_PATH="${HERE}/usr/lib:${LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="${HERE}/usr/plugins"
exec "${HERE}/usr/bin/XXMLStudio" "$@"
EOF
    chmod +x "$APPDIR/AppRun"

    # Build AppImage
    ARCH=x86_64 "$APPIMAGETOOL" "$APPDIR" "$INSTALLER_OUT/$APP_NAME-$VERSION-x86_64.AppImage"

    log_info "AppImage created: $INSTALLER_OUT/$APP_NAME-$VERSION-x86_64.AppImage"
}

# Create DEB and RPM using CPack
create_packages() {
    log_info "Creating DEB and RPM packages..."

    cd "$BUILD_DIR"

    # Install to staging directory
    DESTDIR="$INSTALLER_OUT/staging" cmake --install . --prefix /usr

    # Run CPack
    cpack -G DEB
    cpack -G RPM

    # Move packages to output
    mv *.deb "$INSTALLER_OUT/" 2>/dev/null || true
    mv *.rpm "$INSTALLER_OUT/" 2>/dev/null || true

    log_info "Packages created in $INSTALLER_OUT"
}

# Main
main() {
    echo ""
    echo "=========================================="
    echo "  $APP_NAME Linux Installer Builder"
    echo "  Version: $VERSION"
    echo "=========================================="
    echo ""

    mkdir -p "$INSTALLER_OUT"

    check_prerequisites
    build_app
    create_appdir
    deploy_qt
    create_appimage
    create_packages

    echo ""
    log_info "Build completed successfully!"
    log_info "Installers: $INSTALLER_OUT/"
    echo ""
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --skip-build)
            SKIP_BUILD=1
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
            echo "  --qt-dir DIR    Path to Qt installation"
            echo "  --version VER   Version string for the installer"
            echo "  -h, --help      Show this help message"
            echo ""
            echo "Environment variables:"
            echo "  QT_DIR          Path to Qt installation (default: /opt/Qt/6.10.1/gcc_64)"
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            exit 1
            ;;
    esac
done

main
