#!/usr/bin/env bash

if [[ "${CODE_COVERAGE}" = "true" ]]; then
  lcov --version
  gcov --version
  cmake -DBUILD_TESTING=ON -DCODE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-g -O0 -fprofile-arcs -ftest-coverage --coverage" . || exit 1
  cmake --build . -- -j $(nproc) || exit 1
  ctest --verbose --output-on-failure -j $(nproc) || exit 1
  lcov --directory . --capture --output-file coverage.info --gcov-tool gcov-9
  lcov --remove coverage.info '/usr/*' "${HOME}"'/.cache/*' --output-file coverage.info
  lcov --list coverage.info
  bash <(curl -s https://codecov.io/bash) -f coverage.info || echo "Codecov did not collect coverage reports" exit
fi