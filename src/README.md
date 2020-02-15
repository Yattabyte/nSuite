# Library
This library provides 4 general purpose classes:
- *Yatta::MemoryRange* for safe encapsulation of a contiguous memory range
- *Yatta::Buffer* for easy buffer creation and manipulation
- *Yatta::Directory* for easy directory virtualization and manipulation
- *Yatta::Threader* for easy multi-threading functionality
  

## MemoryRange Overview
The ***MemoryRange*** class represents a range of contiguous memory, comprising a pointer to memory and a size.
It provides a means of querying these basic attributes, and whether or not the range is useable.
Further, it allows for safe access of elements, throwing if attempted to index out of bounds.
For example, a *MemoryRange* can be indexed by the operator[], a char array, a std::byte array, and begin/end iterators. Const variations and type-cast templates are provided as well.
Lastly, *MemoryRange's* provide templates to write in/out objects or raw pointers at given indices.

### MemoryRange Example
```c++
// to do
```


## Buffer Overview
The ***Buffer*** class represents an expandable, contiguous, manipulatable range of memory.
Deriving from the *MemoryRange* class, this class expands the notion of a memory range by allowing it to expand and shrink.
Further, it also provides two sets of useful functions:
- compressing/decompressing
- diffing/patching
Lastly, *Buffer's* provide templates to push/pop objects or raw data into/out of them.

### Buffer Example
```c++
// to do
```


## Directory Overview
The ***Directory*** class represents a virtual file-folder, encompasing the objects within.
It provides a means of fetching files from disk, as well as:
- compressing/decompressing
- diffing/patching

### Directory Example
```c++
// to do
```


## Threader Overview
The ***Threader*** class represents a thread-pool object, who owns a fixed number of system threads.
This class provides a means to add functions to its internal queue of functions to execute in a separate thread.
Additionally, it can be queried for completion or shutdown at will.

### Threader Example
```c++
// to do
```