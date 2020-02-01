#!/usr/bin/env bash

# Run Clang-Tidy using cmake
echo "
**************************************************
Starting Clang-Tidy
**************************************************"
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBUILD_TESTING=ON -DCODE_COVERAGE=OFF -DSTATIC_ANALYSIS=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_CLANG_TIDY=clang-tidy .
cmake --build . --clean-first -- -j $(nproc)
ctest --output-on-failure -j $(nproc) -C Debug .

# Run Valgrind
echo "
**************************************************
Starting Memory Sanitizer (valgrind)
**************************************************"
valgrind --version
ctest --output-on-failure -j $(nproc) -C Debug -D ExperimentalMemCheck .
echo "Dumping Logs"
echo $(<${TRAVIS_BUILD_DIR}/Testing/Temporary/MemoryChecker.1.log)
echo $(<${TRAVIS_BUILD_DIR}/Testing/Temporary/MemoryChecker.2.log)
echo $(<${TRAVIS_BUILD_DIR}/Testing/Temporary/MemoryChecker.3.log)

# Address Sanitizers
echo "
**************************************************
Starting Address Sanitizer
**************************************************"
cmake -DBUILD_TESTING=ON -DCODE_COVERAGE=OFF -DSTATIC_ANALYSIS=OFF -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-g -O0 -fsanitize=address" -UCMAKE_CXX_CLANG_TIDY .
cmake --build . --clean-first -- -j $(nproc)
ctest --output-on-failure -j $(nproc) -C Debug .

# Undefined Behaviour Sanitizer
echo "
**************************************************
Starting Undefined-Behaviour Sanitizer
**************************************************"
cmake -DBUILD_TESTING=ON -DCODE_COVERAGE=OFF -DSTATIC_ANALYSIS=OFF -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-g -O0 -fsanitize=undefined" -UCMAKE_CXX_CLANG_TIDY .
cmake --build . --clean-first -- -j $(nproc)
ctest --output-on-failure -j $(nproc) -C Debug .

# Thread Sanitizer
echo "
**************************************************
Starting Thread Sanitizer
**************************************************"
cmake -DBUILD_TESTING=ON -DCODE_COVERAGE=OFF -DSTATIC_ANALYSIS=OFF -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-g -O0 -fsanitize=thread" -UCMAKE_CXX_CLANG_TIDY .
cmake --build . --clean-first -- -j $(nproc)
ctest --output-on-failure -j $(nproc) -C Debug .

# Run OCLint
echo "
**************************************************
Starting OCLint
**************************************************"
oclint-json-compilation-database -i src -i tests  -e src/lz4

# Run CPPCheck
echo "
**************************************************
Starting CPPCheck
**************************************************"
cppcheck --version
cppcheck src tests -isrc/lz4 --enable=all --quiet