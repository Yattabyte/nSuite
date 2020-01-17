#!/usr/bin/env bash

# Run Clang-Tidy using cmake
echo "Starting Clang-Tidy"
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBUILD_TESTING=ON -DCODE_COVERAGE=OFF -DSTATIC_ANALYSIS=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_CLANG_TIDY=clang-tidy . || exit 1
cmake --build . --clean-first -- -j $(nproc) || exit 1
  
# Run Valgrind
echo "Starting Valgrind"
ctest --verbose --output-on-failure -j $(nproc) -D ExperimentalMemCheck . || exit 1
echo "$(<${TRAVIS_BUILD_DIR}/Testing/Temporary/MemoryChecker.1.log)"
  
# Run OCLint
echo "Starting OCLint"
oclint-json-compilation-database -i src -i tests  -e src/lz4

# Run CPPCheck
echo "Starting CPPCheck"
cppcheck --project=compile_commands.json -isrc/lz4