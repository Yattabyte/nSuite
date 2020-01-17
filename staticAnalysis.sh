#!/usr/bin/env bash

# Run CMake
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBUILD_TESTING=ON -DCODE_COVERAGE=OFF -DSTATIC_ANALYSIS=ON -DCMAKE_BUILD_TYPE=Debug . || exit 1
make clean && make -k -j $(nproc) || exit 1
  
# Run normal tests
ctest --verbose --output-on-failure -j $(nproc) . || exit 1
  
# Run Valgrind
ctest --verbose --output-on-failure -j $(nproc) -D ExperimentalMemCheck . || exit 1
echo "$(<${TRAVIS_BUILD_DIR}/Testing/Temporary/MemoryChecker.1.log)"
  
# Run Oclint
oclint-json-compilation-database -i src -e src/lz4