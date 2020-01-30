# Library
The core of this library is found in just 2 classes:
  - NST::Buffer

  - NST::Directory


## Buffer Overview
The buffer class represents an expandable, contiguous, manipulatable block of memory.
Behaves similarly to std::vector, but is not an adapter of it.
It allocates a region of memory greater-than or equal to a specified size, or can point to another specified region.
Provides data(), cArray(), and operator[]() methods to access underlying data. Can also use readData()/writeData() to transfer data to/from a pointer at a given offset.


### Buffer Manipulation Methods
- compress
- decompress
- diff
- patch


## Directory Overview
The Directory class represents a virtualized folder, sourcing its data from a directory on disk, from a previously packaged-and-serialized directory (written to disk), or from any file with an embedded nSuite formatted package resource.
It contains a list of files and their attributes, as well as all their data for as long as the directory is initialized.


### Directory Manipulation Methods
- make_package
- apply_folder
- make_delta
- apply_delta


## Code Examples

### Buffer Creation
It is really easy to create a buffer.
Lets say you want to create a buffer object to represent some other data of a given size:

```c++
Buffer buffer(DATA_PTR, DATA_SIZE, true);
```

The constructor will copy the data found within the range specified into the buffer.
However, if  you set the last parameter to `false`, the buffer won't copy or destroy the data, only point to it.
This is usefull if you're sourcing hardcoded or const data but still need to perform compression/diff operations on it.

Alternatively, you can create empty buffers of a specific size, and load data into them.

```c++
Buffer buffer(DATA_SIZE);
buffer.writeData(DATA_PTR, DATA_SIZE);
```


### Buffer Manipulation
Here are some examples of modifying the data within a buffer after its creation.
We first load a string into the buffer, resize it, change some of the data, and then output it.

```c++
// Create a buffer containing "Example Data"
Buffer buffer("Example Data", 12);

// Can expand the buffer to 16 bytes, capacity = 12*2 = 24, so won't perform new alloc + copy
buffer.resize(16);

// Replace 'Data' with 'Buffer'
buffer.writeData("Buffer", 6, 8);

// Append '!' character
buffer[14] = std::byte('!');

// Retrieve entire buffer as a string = "Example Buffer!"
std::string entireBuffer(buffer.cArray(), buffer.size());
```

### Buffer compression/decompression
In this example we create a buffer from some pre-existing data, and compress it.
Once it's compressed, we can do anything we want with it.
Here though, we only immediately decompress it and compare the beginning and ending hashes, ensuring they're equal.

```c++
// Create a buffer from some data source
Buffer buffer(DATA_PTR, DATA_SIZE);

if (auto buff_comp = buffer.compress()) {

    if (auto buff_decomp = buff_comp.decompress()) {
        // Verify that the decompressed buffer == the original buffer using hashes
        const auto originalHash = buffer.hash();
        const auto decompressedHash = buff_decomp->hash();
        bool hashesMatch = originalHash == decompressedHash;
    }
}
```


### Buffer diffing/patching
In this example we create 2 buffers from some pre-existing data sets, and diff them.
Usually the diff data would be saved or processed in some fashion, however here we immediately use it to patch the input data, then compare the hashes, ensuring they're equal.

```c++
// Create buffers from the old + new data
Buffer buffer_old(DATA_PTR, DATA_SIZE);
Buffer buffer_new(DATA2_PTR, DATA2_SIZE);

if (auto buff_diff = buffer_old.diff(buffer_new)) {

    if (auto buff_patched = buffer_old.patch(*buff_diff)) {
        // Verify that the patched buffer == the original buffer using hashes
        const auto originalHash = buffer_new.hash();
        const auto patchedHash = buff_patched->hash();
        bool hashesMatch = originalHash == patchedHash;
    }
}
```