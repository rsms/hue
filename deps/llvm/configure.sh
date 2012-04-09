#!/bin/bash
cd "$(dirname $0)"
LLVM_PROJECT_DIR="$(pwd)"
LLVM_SRC_DIR="$LLVM_PROJECT_DIR/src"
LLVM_DST_DIR="$LLVM_PROJECT_DIR/bin"
LLVM_BUILD_DIR="$LLVM_PROJECT_DIR/build"

cd "$LLVM_BUILD_DIR"

"$LLVM_SRC_DIR/configure" \
  "--prefix=$LLVM_DST_DIR" \
  --enable-optimized \
  --enable-jit \
  --enable-targets=x86,x86_64,arm

#make -j
