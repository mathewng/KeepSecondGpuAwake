#!/bin/bash
set -e

# Check for MinGW-w64 toolchain
if ! command -v x86_64-w64-mingw32-gcc &> /dev/null; then
    echo "Error: x86_64-w64-mingw32-gcc not found. Install mingw-w64."
    exit 1
fi

# Toolchain
CC=x86_64-w64-mingw32-gcc
WINDRES=x86_64-w64-mingw32-windres

# Paths
SRCDIR=src/d3d11
OUT=KeepSecondGpuAwake.exe

# Clean previous build artifacts
rm -f resource.res manifest.res $OUT

# Compile icon resource (COFF object format)
$WINDRES -O coff -i $SRCDIR/resource.rc -o resource.res

# Compile manifest resource (COFF object format)
$WINDRES -O coff -i $SRCDIR/manifest.rc -o manifest.res

# Compile and link
$CC -O2 -DUNICODE -D_UNICODE -o $OUT \
    $SRCDIR/main.c \
    $SRCDIR/D3DWnd.c \
    $SRCDIR/chunkchunk.c \
    resource.res \
    manifest.res \
    -ld3d11 -ldxgi -ladvapi32 -lgdi32 -luser32 -lkernel32 -lshell32 \
    -Wl,--subsystem,console \
    -Wl,--entry=AppMain

echo "Built: $OUT"
