#!/usr/bin/env bash

set -e

echo "* Checking for available compilers..."
HAS_GCC=0
HAS_CLANG=0

if command -v gcc >/dev/null 2>&1; then
    echo "  -> Found gcc"
    HAS_GCC=1
fi

if command -v clang >/dev/null 2>&1; then
    echo "  -> Found clang"
    HAS_CLANG=1
fi

if [ $HAS_GCC -eq 1 ]; then
    COMPILER="gcc"
elif [ $HAS_CLANG -eq 1 ]; then
    COMPILER="clang"
else
    echo "! Error: Neither gcc nor clang was found in your PATH." >&2
    exit 1
fi

echo "* Compiling myBuild (Release) using $COMPILER..."

$COMPILER \
  -O3 \
  -s \
  -DNDEBUG \
  -fstack-protector-strong \
  -D_FORTIFY_SOURCE=2 \
  -Wno-unused-result \
  -I./include \
  -I./deps/arena/include \
  -I./deps/CString/include \
  -I./deps/container/include \
  -I./deps/yyjson/include \
  ./deps/arena/lib/arena.c \
  ./deps/CString/lib/cstring.c \
  ./deps/container/lib/cvector.c \
  ./deps/yyjson/src/yyjson.c \
  ./src/main.c \
  ./src/cli.c \
  ./src/config_handler.c \
  ./src/file_handler.c \
  ./src/package_manager.c \
  ./src/utils.c \
  ./src/project_handler.c \
  -o myBuild

echo "* Build successful! Executable created at ./myBuild"
echo "--------------------------------------------------------"

read -p "? Do you want to install 'myBuild' globally so it's available in your PATH? (y/N): " choice

case "$choice" in 
    [yY][eE][sS]|[yY])
        echo "* Installing to /usr/local/bin..."
        BINARY_PATH="$(pwd)/myBuild"
        
        if sudo ln -sf "$BINARY_PATH" /usr/local/bin/myBuild; then
            echo "* Done! You can now run 'myBuild' from anywhere in your terminal."
        else
            echo "! Failed to create symlink. Installation aborted."
        fi
        ;;
    *)
        echo "* Skipped global installation. Executable remains at ./myBuild"
        ;;
esac
