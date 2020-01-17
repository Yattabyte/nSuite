#!/usr/bin/env bash

if [[ "${STATIC_ANALYSIS}" = "true" ]]; then  
  # Run CMake
  cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DAGE_BUILD_WITH_CHECKS=ON -DBUILD_TESTING=ON -DCODE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug . || exit 1
  make clean && make -k -j $(nproc) || exit 1
  
  # Run normal tests
  ctest -j $(nproc) --output-on-failure . || exit 1
  
  # Run Valgrind
  ctest -j $(nproc) --output-on-failure -D ExperimentalMemCheck . || exit 1
  
  # Run Oclint
  oclint-json-compilation-database -i include -i src
fi