#!/usr/bin/env bash

# Run CPPCheck
echo "
**************************************************
Starting CPPCheck
**************************************************"
cppcheck src tests -isrc/lz4 -I src/ -I tests/ --enable=all --quiet --suppress=missingIncludeSystem

# Run Clang-Tidy using cmake
echo "
**************************************************
Starting Clang-Tidy
**************************************************"
cmake -DBUILD_TESTING=ON -DCODE_COVERAGE=OFF -DSTATIC_ANALYSIS=OFF -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_CLANG_TIDY=clang-tidy .
cmake --build . --clean-first -- -j $(nproc)
ctest --output-on-failure -j $(nproc) -C Debug .

# Run Valgrind
echo "
**************************************************
Starting Memory Sanitizer (valgrind)
**************************************************"
cmake -DSTATIC_ANALYSIS=ON -UCMAKE_CXX_CLANG_TIDY .
cmake --clean-first .
ctest --output-on-failure -j $(nproc) -C Debug -D ExperimentalBuild .
ctest --output-on-failure -j $(nproc) -C Debug -D ExperimentalTest .
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
cmake -DSTATIC_ANALYSIS=OFF -DCMAKE_CXX_FLAGS="-g -O0 -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize=address" -DCMAKE_EXE_LINKER_FLAGS="-g -O0 -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize=address" .
cmake --build . --clean-first -- -j $(nproc)
ctest --output-on-failure --quiet -j $(nproc) -C Debug .

# Undefined Behaviour Sanitizer
echo "
**************************************************
Starting Undefined-Behaviour Sanitizer
**************************************************"
cmake -DCMAKE_CXX_FLAGS="-g -O0 -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize=undefined" -DCMAKE_EXE_LINKER_FLAGS="-g -O0 -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize=undefined" .
cmake --build . --clean-first -- -j $(nproc)
ctest --output-on-failure --quiet -j $(nproc) -C Debug .

# Thread Sanitizer
echo "
**************************************************
Starting Thread Sanitizer
**************************************************"
cmake -DCMAKE_CXX_FLAGS="-g -O0 -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize=thread" -DCMAKE_EXE_LINKER_FLAGS="-g -O0 -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize=thread" .
cmake --build . --clean-first -- -j $(nproc)
ctest --output-on-failure --quiet -j $(nproc) -C Debug .

# OCLint quality reporting
echo "
**************************************************
Starting OCLint
**************************************************"
cmake -DCMAKE_CXX_FLAGS="-g -O0" -DCMAKE_EXE_LINKER_FLAGS="-g -O0" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .
cmake --build . --clean-first -- -j $(nproc)
oclint-json-compilation-database -i src -i tests  -e src/lz4