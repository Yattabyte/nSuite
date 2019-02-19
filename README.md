# Installer

This project creates an installer-maker application, which compresses the directory it ran from into an installer application it self-generates.
The installer application decompresses the content it holds into the directory it executes from.

Still a work in progress, plan on adding options for updating an installer, as well as patching the directory the installer is running from.
Currently using CMAKE.

### Required Libraries

This project requires the following (free) libraries:
* [LZ4 - Compression Library](https://github.com/lz4/lz4)