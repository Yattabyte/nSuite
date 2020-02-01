#!/usr/bin/env bash

echo "
**************************************************
Starting Code Coverage Build
**************************************************"
lcov --version
gcov --version
cmake -DBUILD_TESTING=ON -DCODE_COVERAGE=ON -DSTATIC_ANALYSIS=OFF -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-g -O0 -fno-inline -fprofile-arcs -ftest-coverage --coverage" . || exit 1
cmake --build . -- -j $(nproc) || exit 1
ctest --output-on-failure -j $(nproc) -C Debug . || exit 1

echo "
**************************************************
Starting Code Coverage Analysis
**************************************************"
lcov --directory . --capture --output-file coverage.info --gcov-tool gcov-8
lcov --remove coverage.info '/usr/*' --output-file coverage.info
lcov --list coverage.info
bash <(curl -s https://codecov.io/bash) -f coverage.info || echo "Codecov did not collect coverage reports"