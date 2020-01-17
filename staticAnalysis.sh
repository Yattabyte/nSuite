#!/usr/bin/env bash

# Run Clang-Tidy using cmake
echo "Starting Clang-Tidy"
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBUILD_TESTING=ON -DCODE_COVERAGE=OFF -DSTATIC_ANALYSIS=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_CLANG_TIDY=clang-tidy .
cmake --build . --clean-first -- -j $(nproc)

# Address Sanitizers
echo "Starting Address Sanitizer"
cmake -DBUILD_TESTING=ON -DCODE_COVERAGE=OFF -DSTATIC_ANALYSIS=OFF -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-g -O0 -fsanitize=address" . 
cmake --build . --clean-first -- -j $(nproc)
ctest --verbose --output-on-failure -j $(nproc) .

# Undefined Behaviour Sanitizer
echo "Starting U.B. Sanitizer"
cmake -DBUILD_TESTING=ON -DCODE_COVERAGE=OFF -DSTATIC_ANALYSIS=OFF -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-g -O0 fsanitize=undefined" .
cmake --build . --clean-first -- -j $(nproc)
ctest --verbose --output-on-failure -j $(nproc) .

# Thread Sanitizer
echo "Starting Thread Sanitizer"
cmake -DBUILD_TESTING=ON -DCODE_COVERAGE=OFF -DSTATIC_ANALYSIS=OFF -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-g -O0 -fsanitize=thread" .
cmake --build . --clean-first -- -j $(nproc)
ctest --verbose --output-on-failure -j $(nproc) .

# Run Valgrind
echo "Starting Memory Sanitizer using Valgrind"
valgrind --version
ctest --verbose --output-on-failure -j $(nproc) -D ExperimentalMemCheck .
echo "$(<${TRAVIS_BUILD_DIR}/Testing/Temporary/MemoryChecker.1.log)"
  
# Run OCLint
echo "Starting OCLint"
oclint-json-compilation-database -i src -i tests  -e src/lz4

# Run CPPCheck
echo "Starting CPPCheck"
cppcheck --version
cppcheck src tests -isrc/lz4 --quiet