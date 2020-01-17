#!/usr/bin/env bash

# Run Clang-Tidy using cmake
echo "Starting Clang-Tidy"
valgrind --version
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBUILD_TESTING=ON -DCODE_COVERAGE=OFF -DSTATIC_ANALYSIS=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_CLANG_TIDY=clang-tidy . || exit 1
cmake --build . --clean-first -- -j $(nproc) || exit 1

# Address Sanitizers
echo "Starting Address Sanitizer"
cmake -DBUILD_TESTING=ON -DCODE_COVERAGE=OFF -DSTATIC_ANALYSIS=OFF -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-g -O0 -fsanitize=address" . || exit 1 
cmake --build . --clean-first -- -j $(nproc) || exit 1
ctest --verbose --output-on-failure -j $(nproc) . || exit 1

# Undefined Behaviour Sanitizer
echo "Starting U.B. Sanitizer"
cmake -DBUILD_TESTING=ON -DCODE_COVERAGE=OFF -DSTATIC_ANALYSIS=OFF -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-g -O0 fsanitize=undefined" . || exit 1 
cmake --build . --clean-first -- -j $(nproc) || exit 1
ctest --verbose --output-on-failure -j $(nproc) . || exit 1

# Thread Sanitizer
echo "Starting Thread Sanitizer"
cmake -DBUILD_TESTING=ON -DCODE_COVERAGE=OFF -DSTATIC_ANALYSIS=OFF -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-g -O0 -fsanitize=thread" . || exit 1 
cmake --build . --clean-first -- -j $(nproc) || exit 1
ctest --verbose --output-on-failure -j $(nproc) . || exit 1 

# Run Valgrind
echo "Starting Memory Sanitizer using Valgrind"
ctest --verbose --output-on-failure -j $(nproc) -D ExperimentalMemCheck . || exit 1
echo "$(<${TRAVIS_BUILD_DIR}/Testing/Temporary/MemoryChecker.1.log)"
  
# Run OCLint
echo "Starting OCLint"
oclint-json-compilation-database -i src -i tests  -e src/lz4

# Run CPPCheck
echo "Starting CPPCheck"
cppcheck --version
cppcheck src tests -isrc/lz4 --quiet --error-exitcode=1