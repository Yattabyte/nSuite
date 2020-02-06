#!/usr/bin/env bash

# Address Sanitizers
echo "
**************************************************
Starting Address Sanitizer
**************************************************"
cmake -DSTATIC_ANALYSIS=OFF -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} + "-fsanitize=address" -DCMAKE_LINKER_FLAGS=${CMAKE_LINKER_FLAGS} + "-fsanitize=undefined" -UCMAKE_CXX_CLANG_TIDY .
cmake --build . -- -j $(nproc)
ctest --output-on-failure -j $(nproc) -C Debug .

# Undefined Behaviour Sanitizer
echo "
**************************************************
Starting Undefined-Behaviour Sanitizer
**************************************************"
cmake -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} + "-fsanitize=undefined" -DCMAKE_LINKER_FLAGS=${CMAKE_LINKER_FLAGS} + "-fsanitize=undefined" .
cmake --build . -- -j $(nproc)
ctest --output-on-failure -j $(nproc) -C Debug .

# Thread Sanitizer
echo "
**************************************************
Starting Thread Sanitizer
**************************************************"
cmake -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} + "-fsanitize=thread" -DCMAKE_LINKER_FLAGS=${CMAKE_LINKER_FLAGS} + "-fsanitize=thread" .
cmake --build . -- -j $(nproc)
ctest --output-on-failure -j $(nproc) -C Debug .