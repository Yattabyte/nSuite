# Yatta (formerly nSuite)
[![Linux](https://img.shields.io/travis/yattabyte/nsuite?label=Linux%20Build&logo=Travis)](https://travis-ci.com/Yattabyte/nSuite)
[![Windows](https://img.shields.io/appveyor/ci/yattabyte/nsuite?label=Windows%20Build&logo=Appveyor)](https://ci.appveyor.com/project/Yattabyte/nsuite)
[![CodeCov](https://img.shields.io/codecov/c/gh/yattabyte/nsuite/beta?label=Code%20Coverage&logo=CodeCov)](https://codecov.io/gh/Yattabyte/nSuite)
[![CodeFactor](https://img.shields.io/codefactor/grade/github/yattabyte/nsuite?label=Code%20Factor&logo=CodeFactor)](https://www.codefactor.io/repository/github/yattabyte/nsuite)
[![Codacy](https://img.shields.io/codacy/grade/2b38f4eaa90d4b238942d6daaf578655?label=Code%20Quality&logo=Codacy)](https://www.codacy.com/manual/Yattabyte/nSuite)
[![LGTM](https://img.shields.io/lgtm/grade/cpp/github/Yattabyte/nSuite?label=Code%20Quality&logo=LGTM)](https://lgtm.com/projects/g/Yattabyte/nSuite)
[![CodeDocs](https://codedocs.xyz/Yattabyte/nSuite.svg)](https://codedocs.xyz/Yattabyte/nSuite/)
[![license](https://img.shields.io/github/license/Yattabyte/nSuite?label=License&logo=github)](https://github.com/Yattabyte/nSuite/blob/beta/LICENSE)

This is a C++17 library which allows users to pack, unpack, diff, and patch buffers as well as directories.
Using the standard filesystem library, it can virtualize directories, package them, hash them, diff them, and patch them.

The library is optimized for 64-bit CPU's, and is actively tested on linux and windows with every commit.

## Library Status

The library is currently undergoing an entire rework.
Certain features and examples have been moved to the /deprecated/ directory until they're replaced - including old sample applications.
The current status of the project can be found in *Table 1* below.
For a list of all known supported operating systems and compiler combinations, see *Table 2* below.
And lastly, for all other requirements and dependencies, see *Table 3* below.

***Table 1:** Descriptions of continuous integration environments in use.*

| CI Environment   | Build         | Test                | Analyze                                                                                                                                   |
|:-----------------|:--------------|:--------------------|:-----------------------------------------------------------------------------------------------------------------------------------------:|
| Travis CI        | - [x] Linux   | - [x] Debug/Release | <ul><li>- [x] Code Coverage</li><li>- [x] Clang Tidy</li><li>- [x] Clang Sanitizers</li><li>- [x] CppCheck</li><li>- [x] OCLint</li></ul> |
| Appveyor         | - [x] Windows | - [x] Debug/Release | - [ ] n/a                                                                                                                                 |
| Codecov          | - [ ] n/a     | - [ ] n/a           | - [x] Code Coverage                                                                                                                       |
| LGTM             | - [ ] n/a     | - [ ] n/a           | - [x] Code Review                                                                                                                         |
| CodeFactor       | - [ ] n/a     | - [ ] n/a           | - [x] Code Review                                                                                                                         |
| Codacy           | - [ ] n/a     | - [ ] n/a           | - [x] Code Review                                                                                                                         |


***Table 2:** List of operating systems and compilers supported.*

| Operating System | Compiler                   | Debug/Release |
|:-----------------|:--------------------------:|:-------------:|
| Linux (x64)      | GCC 8/9, Clang 7/8/9       |      BOTH     |
| Windows (x64)    | MSVC 2017/2019, Clang+LLVM |      BOTH     |

***Table 3:** List of all other requirements and dependencies.*

| Category             | Description                                 | Required    |
|:---------------------|:-------------------------------------------:|:-----------:|
| Language             | C++17 (w/ filesystem)                       | YES         |
| CPU Architecture     | x64                                         | YES         |
| Build System         | [CMake](https://cmake.org/)                 | YES         |
| External Libraries   | [LZ4](https://github.com/lz4/lz4)           | PRE-BUNDLED |
| Documentation System | [Doxygen](http://www.doxygen.nl/index.html) | OPTIONAL    |

***This library is licensed under the BSD-3-Clause.***