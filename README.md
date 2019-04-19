# nSuite Directory Tools

This toolset provides users with the ability to package and diff directories and files.


## Packaging
nSuite can package directories in 3 ways:
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
  - Can be unpacked using nSuite

  
## Diffing
nSuite can also be used to generate patch files. These can be applied to a directory using nSuite, or by using our stand-alone updater tool (also provided).

The updater tool automatically applies all .ndiff files it can find next to it, and if successfull, deletes them afterwards. This tool is a naiive implementation of an updater, and would ideally be expanded on by other developers for real-world use. 

The tool and diff files should be kept at the root of an affected directory. It will attempt to apply all patches it can find, even if the patched version is technically 'older'.

# Dependencies/Requirements
 - C++ 17
 - 64-bit
 - Windows 7/8/10
 - Uses [CMake](https://cmake.org/)
 - Requires the [LZ4 - Compression Library](https://github.com/lz4/lz4) to build, but **does not** come bundled with it
 - Using BSD-3-Clause license