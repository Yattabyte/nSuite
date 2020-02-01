# Yatta (formerly nSuite)

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

| CI Environment   | Tasks                     | Status                                                                                                                                                                                                 |
|:-----------------|:-------------------------:|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------:|
| Travis CI        | Build/Test/LCOV/Analysis/ | [![Linux](https://img.shields.io/travis/yattabyte/nsuite?label=Linux%20Build&logo=Travis&style=for-the-badge)](https://travis-ci.com/Yattabyte/nSuite)                                                 |
| Appveyor         | Build/Test                | [![Windows](https://img.shields.io/appveyor/ci/yattabyte/nsuite?label=Windows%20Build&logo=Appveyor&style=for-the-badge)](https://ci.appveyor.com/project/Yattabyte/nsuite)                            |
| Codecov          | Code Coverage             | [![CodeCov](https://img.shields.io/codecov/c/gh/yattabyte/nsuite/beta?label=Code%20Coverage&logo=CodeCov&style=for-the-badge)](https://codecov.io/gh/Yattabyte/nSuite)                                 |
| CodeFactor       | Code Analys               | [![CodeFactor](https://img.shields.io/codefactor/grade/github/yattabyte/nsuite?label=Code%20Factor&logo=CodeFactor&style=for-the-badge)](https://www.codefactor.io/repository/github/yattabyte/nsuite) |
| Codacy           | Code Analys               | [![Codacy](https://img.shields.io/codacy/grade/2b38f4eaa90d4b238942d6daaf578655?label=Code%20Quality&logo=Codacy&style=for-the-badge)](https://www.codacy.com/manual/Yattabyte/nSuite)                 |
| LGTM             | Code Analys               | [![LGTM](https://img.shields.io/lgtm/grade/cpp/github/Yattabyte/nSuite?logo=LGTM&style=for-the-badge)](https://lgtm.com/projects/g/Yattabyte/nSuite)                                                   |
| CodeDocs         | Documentation             | [![CodeDocs](https://codedocs.xyz/Yattabyte/nSuite.svg)](https://codedocs.xyz/Yattabyte/nSuite/)                                                                                                       |

***Table 2:** List of operating systems and compilers supported.*

| Operating System | Compiler             | Debug/Release |
|:-----------------|:--------------------:|:-------------:|
| Linux (x64)      | GCC 8/9, Clang 7/8/9 |      BOTH     |
| Windows (x64)    | MSVC 2017/2019, LLVM |      BOTH     |

***Table 3:** List of all other requirements and dependencies.*

| Category             | Description                                 | Required    |
|:---------------------|:-------------------------------------------:|:-----------:|
| Language             | C++17 (w/ filesystem)                       | YES         |
| CPU Architecture     | x64                                         | YES         |
| Build System         | [CMake](https://cmake.org/)                 | YES         |
| External Libraries   | [LZ4](https://github.com/lz4/lz4)           | PRE-BUNDLED |
| Documentation System | [Doxygen](http://www.doxygen.nl/index.html) | OPTIONAL    |

***This library is licensed under the BSD-3-Clause.***