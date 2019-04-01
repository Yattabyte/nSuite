# nStaller Tools

This project is both a library and a toolset that allows developers to generate and distribute portable installers for a given input directory, as well as diff the contents of 2 input directories into a single patch file.
The library includes functions to compress/decompress and diff/patch both files and directoires.
The toolset wraps this functionality into a few example programs.

## nSuite.exe
The nSuite tool is intended to be used by developers or those who wish to package/diff/distribute one or many files. It is run by command-line, and requires one of the following sets of arguments to be fulfilled:
- #### `-installer -src=<path> -dst=<path>`
  - Packages + compress an entire **source** directory into a **destionation** installer *.exe file* (filename optional)  
  
- #### `-pack -src=<path> -dst=<path>`
  - Same as the installer, though doesn't embed into an installer, just a *.npack* file (filename optional)
  - Package files can be read-through by the diffing command, so many versions of a directory can be easily stored on disk in single snapshots.
  
- #### `-unpack -src=<path> -dst=<path>`
  - Decompresses the files held in the **source** *.npack* file, dumping into the **destination** directory.
  
- #### `-diff -old=<path> -new=<path> -dst=<path>`
  - Finds all the common, new, and old files between the **old** and **new** directories. All common files are analyzed bytewise for their differences, and patch instructions are generated. All instructions are compressed and stored in a **destination** *.ndiff* file. ***All*** files are hashed, to ensure the right versions of files are consumed at patch-time.
  - Can use *.npack* files as the **old** ***or*** **new** directories (serving as snapshots). Diffing packages means storing less files and folders across multiple versions on disk -> just 1 snapshot per version.
  
 - #### `-patch -src=<path> -dst=<path>`
   - Uses a **source** *.ndiff* file and executes all the instructions contained within, patching the **destination** directory. All files within the directory are hashed, and must match the hashes found in the patch. Additionally, the post-patch results must match what's expected in the hash. If any of the strict conditions aren't met, it will halt prior to any files being modified (preventing against file corruption).
   
   
## Installer
The installer tool is a portable version of the unpack command, and is generated by nSuite. Each one will have a custom *.npack* file embedded within. The installer is very basic and installs to the directory it runs from.

## Updater
The updater tool is a portable version of the patch command. It automatically applies all *.ndiff* files it can find, and if successfull, deletes them after. This tool is a naiive implementation of an updater, and would ideally be expanded on by other developers. For instance, if patches were found that would modify an app from v.1 -> v.2 -> v.3 -> v.1, the updater won't try to stop at v.3 (as there are no version headers applied to patches). Further, this updater cannot connect to any servers to fetch patch data, but that would be the next logical step after implementing versioning. 

# Dependencies/Requirements
 - 64-bit only
 - Might only work in Windows
 - Uses [CMake](https://cmake.org/)
 - Requires the [LZ4 - Compression Library](https://github.com/lz4/lz4) to build, but **does not** come bundled with it
 - Using BSD-3-Clause license
