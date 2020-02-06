#!/usr/bin/env bash

# Run CPPCheck
echo "
**************************************************
Starting CPPCheck
**************************************************"
cppcheck --version
cppcheck src tests -isrc/lz4 --enable=all --quiet

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
valgrind --version
cmake -DSTATIC_ANALYSIS=ON -UCMAKE_CXX_CLANG_TIDY .
cmake --build . -- -j $(nproc)
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
cmake -DSTATIC_ANALYSIS=OFF -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} + " -fsanitize=address" -DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS} + " -fsanitize=undefined" .
cmake --build . -- -j $(nproc)
ctest --output-on-failure -j $(nproc) -C Debug .

# Undefined Behaviour Sanitizer
echo "
**************************************************
Starting Undefined-Behaviour Sanitizer
**************************************************"
cmake -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} + " -fsanitize=undefined" -DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS} + " -fsanitize=undefined" .
cmake --build . -- -j $(nproc)
ctest --output-on-failure -j $(nproc) -C Debug .

# Thread Sanitizer
echo "
**************************************************
Starting Thread Sanitizer
**************************************************"
cmake -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} + " -fsanitize=thread" -DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS} + " -fsanitize=thread" .
cmake --build . -- -j $(nproc)
ctest --output-on-failure -j $(nproc) -C Debug .