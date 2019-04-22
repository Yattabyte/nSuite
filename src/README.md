# Library
The core of this library can be found between 2 namespaces:  
- BFT (BufferTools)
- DFT (DirectoryTools)

#### The BufferTools namespace provides the following 5 functions:
- [CompressBuffer](#CompressBuffer)
- [DecompressBuffer](#DecompressBuffer)
- [DiffBuffers](#DiffBuffers)
- [PatchBuffer](#PatchBuffer)
- [HashBuffer](#HashBuffer)

### The DirectoryTools namespace exposes 4 similar helper functions:
- [CompressDirectory](#CompressDirectory)
- [DecompressDirectory](#DecompressDirectory)
- [DiffDirectory](#DiffDirectory)
- [PatchDirectory](#PatchDirectory)

# Functions
## BFT namespace
### CompressBuffer
Compresses a source buffer into an equal or smaller-sized destination buffer.  
After compression, it applies a small header describing how large the uncompressed buffer is.
```c++
bool BFT::CompressBuffer(
  char * sourceBuffer, 
  const size_t & sourceSize,
  char ** destinationBuffer,
  size_t & destinationSize
);
```
Parameter:  `sourceBuffer`       the original buffer to read from.  
Parameter:  `sourceSize`         the size of the source buffer in bytes.  
Parameter:  `destinationBuffer`  pointer to the destination buffer, which will hold compressed contents.  
Parameter:  `destinationSize`    reference updated with the size in bytes of the compressed destinationBuffer.  
Return:                          true if compression success, false otherwise.  

Example Usage:
```c++
// Create a buffer, fill it with data
char * buffer = new char[100];

// Compress
char * compressedBuffer(nullptr);
size_t compressedSize(0);
bool result = BFT::CompressBuffer(buffer, 100, &compressedBuffer, compressedSize);
delete[] buffer;
if (result) {
  // Do something with compressedBuffer (size of compressedSize)
  delete[] compressedBuffer;
}
```

### DecompressBuffer
Decompressess a source buffer into an equal or larger-sized destination buffer.  
Prior to decompression, it reads from a small header describing how large of a buffer it needs to allocate.
```c++
bool BFT::DecompressBuffer(
  char * sourceBuffer, 
  const size_t & sourceSize, 
  char ** destinationBuffer, 
  size_t & destinationSize
);
```
Parameter:  `sourceBuffer`       the original buffer to read from.  
Parameter:  `sourceSize`         the size of the source buffer in bytes.  
Parameter:  `destinationBuffer`  pointer to the destination buffer, which will hold decompressed contents.  
Parameter:  `destinationSize`    reference updated with the size in bytes of the decompressed destinationBuffer.  
Return:                          true if decompression success, false otherwise.  

Example Usage:
```c++
// Create a buffer, fill it with data
char * compressedBuffer = new char[size_of_compressed_buffer];

// Decompress
char * buffer(nullptr);
size_t bufferSize(0);
bool result = BFT::DecompressBuffer(compressedBuffer, size_of_compressed_buffer, &buffer, bufferSize);
delete[] compressedBuffer;
if (result) {
  // Do something with decompressedBuffer (size of bufferSize)
  delete[] buffer;
}
```

### DiffBuffers
Processes two input buffers, diffing them.  
Generates a compressed instruction set dictating how to get from the old buffer to the new buffer.
```c++
bool BFT::DiffBuffers(
  char * buffer_old, 
  const size_t & size_old, 
  char * buffer_new, 
  const size_t & size_new, 
  char ** buffer_diff, 
  size_t & size_diff, 
  size_t * instructionCount = nullptr
);
```
Parameter:  `buffer_old`        the older of the 2 buffers.  
Parameter:  `size_old`          the size of the old buffer.  
Parameter:  `buffer_new`        the newer of the 2 buffers.  
Parameter:  `size_new`          the size of the new buffer.  
Parameter:  `buffer_diff`       pointer to store the diff buffer at.  
Parameter:  `size_diff`         reference updated with the size of the compressed diff buffer.  
Parameter:  `instructionCount`  (optional) pointer to update with the number of instructions processed.  
Return:                         true if diff success, false otherwise.  

Example Usage:
```c++
// Buffers for two input files
char * fileA = new char[100];
char * fileB = new char[255];

// ...Fill the buffers with some content... //

// Diff Files (generate instruction set from fileA - fileB)
char * diffBuffer(nullptr);
size_t diffSize(0), instructionCount(0);
bool result = BFT::DiffBuffers(fileA, 100, fileB, 255, &diffBuffer, diffSize, &instructionCount);
delete[] fileA;
delete[] fileB;
if (result) {
  // Do something with diffBuffer (size of diffSize)
  delete[] diffBuffer;
}
```

### PatchBuffer
Reads from a compressed instruction set, uses it to patch the 'older' buffer into the 'newer' buffer
```c++
bool BFT::PatchBuffer(
  char * buffer_old, 
  const size_t & size_old, 
  char ** buffer_new,
  size_t & size_new, 
  char * buffer_diff, 
  const size_t & size_diff, 
  size_t * instructionCount = nullptr
);
```
Parameter:  `buffer_old`        the older of the 2 buffers.  
Parameter:  `size_old`          the size of the old buffer.  
Parameter:  `buffer_new`        pointer to store the newer of the 2 buffers.  
Parameter:  `size_new`          reference updated with the size of the new buffer.  
Parameter:  `buffer_diff`       the compressed diff buffer (instruction set).  
Parameter:  `size_diff`         the size of the compressed diff buffer.  
Parameter:  `instructionCount`  (optional) pointer to update with the number of instructions processed.  
Return:                         true if patch success, false otherwise.  

Example Usage:
```c++
// Buffers for two input files
char * fileA = new char[100];
char * diffBuffer = new char[size_of_diff];

// ...Fill the buffers with some content... //

// Patch File (generate fileB from fileA + diffBuffer instructions)
char * fileB(nullptr);
size_t fileBSize(0), instructionCount(0);
bool result = BFT::PatchBuffer(fileA, 100, &fileB, fileBSize, diffBuffer, size_of_diff, &instructionCount);
delete[] fileA;
delete[] diffBuffer;
if (result) {
  // Do something with fileB (size of fileBSize)
  delete[] fileB;
}
```

### HashBuffer
Generates a hash value for the buffer provided, using the buffers' contents.
```c++
size_t BFT::HashBuffer(
  char * buffer, 
  const size_t & size
);
```
Parameter:  `buffer`  the older of the 2 buffers.  
Parameter:  `size`    the size of the old buffer.  
Return:               hash value for the buffer.    

Example Usage:
```c++
// Buffers for two input files
char * fileA = new char[100];
char * fileB = new char[255];

// ...Fill the buffers with some content... //

size_t hashA = BFT::HashBuffer(fileA, 100);
size_t hashB = BFT::HashBuffer(fileB, 255);

if (hashA != hashB) {
  // Diff the buffers or do something with the knowledge that they differ
}
```
