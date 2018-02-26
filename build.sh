#!/bin/sh

mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_TOOLCHAIN_FILE=../toolchain-powerpc.cmake -G Ninja ..
ninja
