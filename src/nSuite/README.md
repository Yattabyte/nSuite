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
- [compress](#Buffer Compression)
- [decompress]
- [diff]
- [patch]


## Directory Overview
The Directory class represents a virtualized folder, sourcing its data from a directory on disk, from a previously packaged-and-serialized directory (written to disk), or from any file with an embedded nSuite formatted package resource.  
It contains a list of files and their attributes, as well as all their data for as long as the directory is initialized.  


### Directory Manipulation Methods
- make_package
- apply_folder
- make_delta
- apply_delta


# Code Examples

## Basic buffer creation

```c++
// Create a buffer containing "Example Data"
Buffer buffer(12);
std::memcpy(buffer.data(), "Example Data", 12);


// Expand the buffer to 16 bytes
// Underlying capacity is (12 * 2), won't need new alloc + copy
buffer.resize(16);	


// Replace 'Data' with 'Buffer'
buffer.writeData("Buffer", 6, 8);	


// Append '!' character
buffer[14] = std::byte('!');	


// Retrieve second word
char secondWord[7];
buffer.readData(&secondWord, 7, 8);	


// Retrieve entire buffer as a string = "Example Buffer!"
std::string entireBuffer(buffer.cArray(), buffer.size());
```


## Other buffer creation methods

```c++
// Creates a buffer in the most verbose way possible
Buffer buffer_worst(DATA_SIZE);
std::memcopy(buffer_worst.data(), DATA_PTR, DATA_SIZE);


// Creates a buffer in a more descriptive, but less than ideal way
Buffer buffer_better(DATA_SIZE);
buffer_better.writeData(DATA_PTR, DATA_SIZE, 0);


// Creates an ideal buffer, copying the input region
Buffer buffer_best(DATA_PTR, DATA_SIZE);


// Optionally, create a buffer, only pointing to the input region
Buffer buffer_pointer(DATA_PTR, DATA_SIZE, false);
```


## Buffer compression/decompression

```c++
// Create a buffer from some data source
Buffer buffer(DATA_PTR, DATA_SIZE);


// Compress the buffer
auto buff_comp = buffer.compress();


if (buff_comp) {
	// ... do something with the compressed buffer, such as write to disk...
	
	
	// Maybe on another machine, decompress the buffer
	auto buff_decomp = buff_comp.decompress();
	
	
	if (buff_decomp) {		
		// ... do something with the decompressed buffer, such as analyze it...
	
	
		// Can verify that the decompressed buffer == the original buffer using hashes
		const auto originalHash = buffer.hash();
		const auto decompressedHash = buff_decomp->hash();		
		bool hashesMatch = originalHash == decompressedHash;
	}
}
```


## Buffer diffing/patching

```c++
// Create buffers from the old + new data
Buffer buffer_old(DATA_PTR, DATA_SIZE);
Buffer buffer_new(DATA2_PTR, DATA2_SIZE);


// Diff the buffers
auto buff_diff = buffer_old.diff(buffer_new);


if (buff_diff) {
	// ... do something with the diff buffer, such as write to disk...
	writeDataToDisk(buff_comp->data(), buff_comp->size(), Directory);
	
	
	
	// Maybe on another machine, patch the buffer
	// Would be wise to ensure that old buffers match using hashes
	auto buff_patched = buffer_old.patch(*buff_diff);
	if (buff_patched) {		
		// ... do something with the patchged buffer, such as analyze it...
		
		
		// Can verify that the patched buffer == the original buffer using hashes
		const auto originalHash = buffer_new.hash();
		const auto patchedHash = buff_patched->hash();		
		bool hashesMatch = originalHash == patchedHash;
	}
}
```