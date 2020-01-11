# nSuite

This library allows users to package, compress, and diff buffers & directories. 
The code is written in C++17, and makes heavy use of [std::filesystem](https://en.cppreference.com/w/cpp/header/filesystem).

Example programs are provided which implement the packing, unpacking, diffing, and patching operations defined in this library.

# Status
**Travis CI** [![Travis CI Status](https://travis-ci.com/Yattabyte/nSuite.svg?branch=beta)](https://travis-ci.com/Yattabyte/nSuite)  
  - OS: Linux
  - Compilers: GCC 8/9, Clang 7/8/9  
  
**Appveyor** [![Build status](https://ci.appveyor.com/api/projects/status/7gheavgnj8cooyxx/branch/beta?svg=true)](https://ci.appveyor.com/project/Yattabyte/nsuite/branch/beta)  
  - OS: Windows
  - Compilers: Visual Studio 2017 MSVC, Visual Studio 2019 MSVC


## Library
All classes and methods provided by this library are found within the NST namespace. The most important classes are:
  - NST::Buffer
    - A container class representing a contiguous, expandable, and manipulatable block of memory.
	- Provides methods for compression/decompression and diffing/patching
  - NST::Directory
    - A virtual folder, holding data sourced from disk or from our compressed format
	- Provides methods for packing/unpacking, delta/update


## nSuite Wizard
The nSuite wizard is a stand-alone example program that can generate installers & packagers, as well as diff directories/packages.


### Packaging
The nSuite wizard can package directories in 3 ways (ordered by increasing complexity):
- A .npack file
  - Can be unpacked using the nSuite wizard

- A lightweight portable package/installer
  - Extracts to a folder in the directory it runs from
  - Runs in a terminal, no user input
  - Doesn't modify registry - no uninstaller
  - .npack file embedded within

- A fully-fledged installer with a GUI (Windows)
  - Customizable by writing attributes into a manifest file
  - Generates an uninstaller (adds it to the registry)
  - .npack file embedded within
  

### Diffing
The nSuite Wizard can also generate diff files, which can either be applied to another directory using the wizard, or by using the stand-alone example updater tool.  
All input directories are parsed into NST::Directory objects, so the "old" and/or "new" directories can be actual folders on disk, .npack files, or programs with packages embedded in them (e.g. installers made by nSuite)


# Dependencies/Requirements/Acknowledgements
 - C++17
 - 64-bit
 - Windows 7/8/10
 - Uses [CMake](https://cmake.org/)
 - Documentation built using [Doxygen](http://www.doxygen.nl/index.html) (optional)
 - Requires the [LZ4 - Compression Library](https://github.com/lz4/lz4) to build, but **does not** come bundled with it
 - Using BSD-3-Clause license
