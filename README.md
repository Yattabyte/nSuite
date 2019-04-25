# nSuite Data Tools

This library allows users to package and diff directories, files, and buffers. This project's focus is on the following 2 sets of 2 functions, in respect to both directories and buffers:
- compression/decompression
- diffing/patching

In addition to the library, this project comes equipped with example programs that directly implement all the packing, unpacking, diffing, and patching operations supported.

## Library
The core of this library can be found between 2 namespaces:  
- BFT (BufferTools)
  - Provides buffer compression, decompression, diff, patch, and hash functions
- DRT (DirectoryTools)
  - Provides directory compression, decompression, diff, patch, and other utility functions

## Example tools
### Packaging
The nSuite Wizard can package directories in 3 ways:
- A fully fledged installer with a GUI (Windows)
  - Customizable by writing attributes into a manifest file
  - Generates an uninstaller (adds it to the registry)
  - .npack file embedded within
  
- A lightweight portable package/installer
  - Extracts to a folder in the directory it runs from
  - Runs in a terminal, no user input
  - Doesn't modify registry - no uninstaller
  - .npack file embedded within
  
- A .npack file
  - Can be unpacked using the nSuite wizard

  
### Diffing
The nSuite Wizard can also generate diff files, which can either be applied to another directory using the wizard, or by using the stand-alone example updater tool.

# Dependencies/Requirements
 - C++ 17
 - 64-bit
 - Windows 7/8/10
 - Uses [CMake](https://cmake.org/)
 - Requires the [LZ4 - Compression Library](https://github.com/lz4/lz4) to build, but **does not** come bundled with it
 - Using BSD-3-Clause license
