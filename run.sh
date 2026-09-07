#!/bin/bash
set -euo pipefail

# vendors
build_vendor() {
  local lib_name="$1"
  local build_dir="vendor/build/$lib_name"

  if [ ! -d "$build_dir" ]; then
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
ENGINE_SRCS=$(find src/engine -type f -name "*.c" 2>/dev/null || true)
GAME_SRCS=$(find src/game -type f -name "*.c" 2>/dev/null || true)
MAIN_SRC="src/main.c"

ENGINE_OBJS=""
for src in $ENGINE_SRCS; do
  obj="build/${src%.c}.o"
  ENGINE_OBJS="$ENGINE_OBJS $obj"
done

GAME_OBJS=""
for src in $GAME_SRCS; do
  obj="build/${src%.c}.o"
  GAME_OBJS="$GAME_OBJS $obj"
done

MAIN_OBJ="build/src/main.o"

# make ninja
mkdir -p build
cat << 'EOF' > build.ninja
# vars
cflags = -Wall -Wextra -O0 -g -Isrc/engine -Isrc/game -Ivendor/raylib/src
ldflags_main = -ldl -lm -lpthread -Lvendor/build/raylib/raylib -lraylib -Wl,-rpath,vendor/build/raylib/raylib
ldflags_game = -lm -Lvendor/build/raylib/raylib -lraylib

# rules
rule cc
  depfile = $out.d
  deps = gcc
  command = gcc -MD -MF $out.d $cflags -c $in -o $out

rule link_exe
  command = gcc $in -o $out $ldflags_main

rule link_shared
  command = gcc -shared $in -o $out $ldflags_game

EOF

{
  echo "# make objects"
  for src in $ENGINE_SRCS; do
    echo "build build/${src%.c}.o: cc $src"
  done
  for src in $GAME_SRCS; do
    echo "build build/${src%.c}.o: cc $src"
  done
  echo "build $MAIN_OBJ: cc $MAIN_SRC"
  echo ""

  echo "# make game shared lib"
  echo "build build/libgame.so: link_shared $GAME_OBJS"
  echo ""

  echo "# build exe"
  echo "build build/a: link_exe $MAIN_OBJ $ENGINE_OBJS"
  echo ""
  echo "default build/a build/libgame.so"
} >> build.ninja

ninja -t compdb cc > build/compile_commands.json

# build run
ninja
./build/a
