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

| CI Environment   | Tasks            | Status                                                                                                                                                                    |
|:-----------------|:----------------:|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------:|
| Travis CI        | Build/Test/LCOV  | [![Travis CI Status](https://travis-ci.com/Yattabyte/nSuite.svg?branch=beta)](https://travis-ci.com/Yattabyte/nSuite)                                                     |
| Appveyor         | Build/Test       | [![Build status](https://ci.appveyor.com/api/projects/status/7gheavgnj8cooyxx/branch/beta?svg=true)](https://ci.appveyor.com/project/Yattabyte/nsuite/branch/beta)        |
| Codecov          | Code Coverage    | [![codecov](https://codecov.io/gh/Yattabyte/nSuite/branch/beta/graph/badge.svg)](https://codecov.io/gh/Yattabyte/nSuite)                                                  |
| CodeFactor       | Code Review      | [![CodeFactor](https://www.codefactor.io/repository/github/yattabyte/nsuite/badge)](https://www.codefactor.io/repository/github/yattabyte/nsuite)                         |
| Codacy           | Code Review      | [![Codacy Badge](https://api.codacy.com/project/badge/Grade/2b38f4eaa90d4b238942d6daaf578655)](https://www.codacy.com/manual/Yattabyte/nSuite)                            |
| LGTM             | Code Review      | [![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/Yattabyte/nSuite.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/Yattabyte/nSuite/context:cpp) |
| CodeDocs         | Documentation    | [![CodeDocs Badge](https://codedocs.xyz/Yattabyte/nSuite.svg)](https://codedocs.xyz/Yattabyte/nSuite/)                                                                    |

***Table 2:** List of operating systems and compilers supported.*

| Operating System | Compiler             | Debug/Release |
|:-----------------|:--------------------:|:-------------:|
| Linux (x64)      | GCC 8/9, Clang 7/8/9 |      BOTH     |
| Windows (x64)    | MSVC 2017/2019       |      BOTH     |

***Table 3:** List of all other requirements and dependencies.*

| Category             | Description                                 | Required    |
|:---------------------|:-------------------------------------------:|:-----------:|
| Language             | C++17 (w/ filesystem)                       | YES         |
| CPU Architecture     | x64                                         | YES         |
| Build System         | [CMake](https://cmake.org/)                 | YES         |
| External Libraries   | [LZ4](https://github.com/lz4/lz4)           | PRE-BUNDLED |
| Documentation System | [Doxygen](http://www.doxygen.nl/index.html) | OPTIONAL    |
| Documentation System | [Doxygen](http://www.doxygen.nl/index.html) | OPTIONAL    |

***This library is licensed under the BSD-3-Clause.***