#################
# Release Build #
#################

#!/bin/bash

cmake -DBUILD_TESTING=ON -DCODE_COVERAGE=OFF -DCMAKE_BUILD_TYPE=Release . || exit 1
cmake --build . --clean-first -- -j $(nproc) || exit 1
ctest --verbose --output-on-failure -j $(nproc) || exit 1