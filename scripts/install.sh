#!/bin/bash
set -e  # Exit on any error

SCRIPT_DIR=$(dirname "$(realpath "$0")")

INSTALL_PREFIX="/usr/local"
OVERWRITE=false

while [[ "$#" -gt 0 ]]; do
    case $1 in
        -o|--overwrite) OVERWRITE=true; shift ;;
        -p|--prefix) INSTALL_PREFIX="$2"; shift 2 ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

BUILD_DIR="$SCRIPT_DIR/../build"

if [ "$OVERWRITE" = true ]; then
    echo "Overwrite flag set: Cleaning previous build..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "Configuring the project with CMake..."
cmake -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" "$SCRIPT_DIR/.."

echo "Building the project..."
cmake --build . -- -j$(nproc)

echo "Installing the project to $INSTALL_PREFIX..."
if [ "$EUID" -ne 0 ]; then
    echo "Running 'sudo make install'..."
    sudo cmake --build . --target install
else
    cmake --build . --target install
fi

echo "Installation finished!"
echo "Headers and libraries are installed in $INSTALL_PREFIX/include and $INSTALL_PREFIX/lib."
