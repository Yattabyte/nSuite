# nStallr

This project consists of 4 small utilities to bundle, extract, diff, and patch files.

It's still a work in progress, but at this point all 4 of these utilities function in some form.

## Installer

The installer is a small program, custom-built from the Installer-Maker utility. It has a compressed payload embedded within it, and by default extracts it into the directory it is launched from.

## Installer-Maker

The installer-maker utility compresses the directory it runs from, and writes out a new installer which it updates with these compressed contents.

## Patch-Maker

The patch-maker analyzes a pair of 'old' and 'new' files, diffing them, generating a set of instructions which it writes out to disk.

## Patcher

The patcher reads in a target 'old' file and a custom .diff file, which it uses to update the 'old' file into a 'new' file.


### Dependencies
This project is 64-bit only, and might only work in Windows.
Currently using [CMake](https://cmake.org/), currently tailored to generate VS2017 solutions but should function for other build environments.

Additionally, this project uses the [LZ4 - Compression Library](https://github.com/lz4/lz4) for (de)compression, but could easily be modified in the future to support other libraries.
