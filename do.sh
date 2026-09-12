#!/bin/bash
set -euo pipefail

NAME="a"

# reload lib only
if [[ "${1:-}" == "--reload" ]]; then
  if [ ! -f "build.ninja" ]; then
    echo "build.ninja not found." >&2
    exit 1
  fi
  ninja build/libhandle.so
  exit 0
fi

# vendors
build_vendor() {
  local lib_name="$1"
  local build_dir="vendor/build/$lib_name"

  if [ ! -d "$build_dir" ]; then
    echo "Building '$build_dir'..."
    cmake -S "vendor/$lib_name" \
          -B "$build_dir" \
          -DBUILD_SHARED_LIBS=ON \
          -DCMAKE_BUILD_TYPE=Release && \
    cmake --build "$build_dir"
  else
    echo "Lib '$build_dir' already exists. Skipping."
  fi
}
build_vendor raylib

# vars
SRCS=$(find src -type f -name "*.c" ! -path "src/main.c" ! -path "src/handle*" 2>/dev/null || true)
OBJS=""
for src in $SRCS; do
  obj="build/${src%.c}.o"
  OBJS="$OBJS $obj"
done

HANDLE_SRCS=$(find src/handle -type f -name "*.c" 2>/dev/null || true)
HANDLE_OBJS=""
for src in $HANDLE_SRCS; do
  obj="build/${src%.c}.o"
  HANDLE_OBJS="$HANDLE_OBJS $obj"
done

MAIN_SRC="src/main.c"
MAIN_OBJ="build/src/main.o"

SCRIPT_PATH=$(realpath -- "${BASH_SOURCE[0]}")
PROJECT_DIR=$(dirname -- "$SCRIPT_PATH")

if [[ "${1:-}" == "--release" ]]; then
  CFLAGS_ADD="-O3"
  LDFLAGS_ADD=""
else
  CFLAGS_ADD="-fsanitize=address,undefined -O0 -g"
  LDFLAGS_ADD="-fsanitize=address,undefined"
fi

# make ninja
mkdir -p build
{
  echo "# vars"
  echo "cflags = -std=c23 -Wall -Wextra $CFLAGS_ADD"
  echo "ldflags = -ldl -lm -lpthread -rdynamic $LDFLAGS_ADD"
  echo "incflags = -Isrc -Isrc/core -Ivendor/raylib/src"
  echo "libflags = -Lvendor/build/raylib/raylib -lraylib -Wl,-rpath,vendor/build/raylib/raylib"
  echo ""
  echo "# rules"
  echo "rule cc"
  echo "  depfile = \$out.d"
  echo "  deps = gcc"
  echo "  command = gcc -MD -MF \$out.d \$cflags \$incflags -c \$in -o \$out -D_PROJECT_DIR=\"\\\"$PROJECT_DIR\\\"\" -D_EXE_DIR=\"\\\"$PROJECT_DIR/build\\\"\""
  echo ""
  echo "rule cc_pic"
  echo "  depfile = \$out.d"
  echo "  deps = gcc"
  echo "  command = gcc -MD -MF \$out.d -fPIC \$cflags \$incflags -c \$in -o \$out -D_PROJECT_DIR=\"\\\"$PROJECT_DIR\\\"\" -D_EXE_DIR=\"\\\"$PROJECT_DIR/build\\\"\""
  echo ""
  echo "rule exe"
  echo "  command = gcc \$in -o \$out \$ldflags \$libflags"
  echo ""
  echo "rule shared"
  echo "  command = gcc -shared \$in -o \$out \$ldflags \$libflags"
  echo ""
  echo "# make objects"
  for src in $SRCS; do
    echo "build build/${src%.c}.o: cc $src"
  done
  for src in $HANDLE_SRCS; do
    echo "build build/${src%.c}.o: cc_pic $src"
  done
  echo "build $MAIN_OBJ: cc $MAIN_SRC"
  echo ""
  echo "# build exe"
  echo "build build/$NAME: exe $MAIN_OBJ $OBJS"
  echo ""
  echo "# build handle"
  echo "build build/libhandle.so: shared $HANDLE_OBJS"
  echo ""
  echo "default build/$NAME build/libhandle.so"
} > build.ninja

ninja -t compdb cc > build/compile_commands.json

# build run
ninja
echo "Out: ./build/a"
