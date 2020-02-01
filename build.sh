#!/usr/bin/env bash

# Debug Build
echo "
**************************************************
Starting Debug Build
**************************************************"
cmake -DBUILD_TESTING=ON -DCODE_COVERAGE=OFF -DSTATIC_ANALYSIS=OFF -DCMAKE_BUILD_TYPE=Debug . || exit 1
cmake --build . --clean-first -- -j $(nproc) || exit 1
ctest --output-on-failure -j $(nproc) -C Debug . || exit 1

# Release Build
echo "
**************************************************
Starting Release Build
**************************************************"
cmake -DBUILD_TESTING=ON -DCODE_COVERAGE=OFF -DSTATIC_ANALYSIS=OFF -DCMAKE_BUILD_TYPE=Release . || exit 1
cmake --build . --clean-first -- -j $(nproc) || exit 1
ctest --output-on-failure -j $(nproc) -C Release . || exit 1