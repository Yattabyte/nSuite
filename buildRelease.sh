#!/usr/bin/env bash

cmake -DBUILD_TESTING=ON -DCODE_COVERAGE=OFF -DSTATIC_ANALYSIS=OFF -DCMAKE_BUILD_TYPE=Release . || exit 1
cmake --build . --clean-first -- -j $(nproc) || exit 1
ctest --verbose --output-on-failure -j $(nproc) || exit 1