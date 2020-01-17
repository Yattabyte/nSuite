#!/usr/bin/env bash
 
 if [[ "${CODE_COVERAGE}" = "true" ]]; then
    LCOV_URL="https://github.com/linux-test-project/lcov/releases/download/v1.14/lcov-1.14.tar.gz"
    mkdir lcov && travis_retry wget --no-check-certificate -O - ${LCOV_URL} | tar --strip-components=1 -xz -C lcov
    cd lcov
    make install
    export PATH=${DEPS_DIR}/lcov/bin:${PATH}
 fi