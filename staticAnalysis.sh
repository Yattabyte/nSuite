#!/usr/bin/env bash

# Address Sanitizers
echo "
**************************************************
Starting Address, U.B., and Thread Sanitizers
**************************************************"
cmake -DBUILD_TESTING=ON -DCODE_COVERAGE=OFF -DSTATIC_ANALYSIS=OFF -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-g -O0 -fsanitize=address,undefined,thread -fno-omit-frame-pointer -fno-optimize-sibling-calls" -UCMAKE_CXX_CLANG_TIDY .
cmake --build . --clean-first -- -j $(nproc)
ctest --output-on-failure -j $(nproc) -C Debug .