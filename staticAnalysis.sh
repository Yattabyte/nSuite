#!/usr/bin/env bash

# Address Sanitizers
echo "
**************************************************
Starting Address Sanitizer
**************************************************"
cmake -DSTATIC_ANALYSIS=OFF -DCMAKE_CXX_FLAGS="-g -O0 -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls" -UCMAKE_CXX_CLANG_TIDY .
cmake --build . -- -j $(nproc)
ctest --output-on-failure -j $(nproc) -C Debug .

# Undefined Behaviour Sanitizer
echo "
**************************************************
Starting Undefined-Behaviour Sanitizer
**************************************************"
cmake -DCMAKE_CXX_FLAGS="-g -O0 -fsanitize=undefined -fno-omit-frame-pointer -fno-optimize-sibling-calls" .
cmake --build . -- -j $(nproc)
ctest --output-on-failure -j $(nproc) -C Debug .

# Thread Sanitizer
echo "
**************************************************
Starting Thread Sanitizer
**************************************************"
cmake -DCMAKE_CXX_FLAGS="-g -O0 -fsanitize=thread -fno-omit-frame-pointer -fno-optimize-sibling-calls" .
cmake --build . -- -j $(nproc)
ctest --output-on-failure -j $(nproc) -C Debug .