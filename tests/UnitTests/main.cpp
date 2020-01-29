#include "yatta.hpp"
#include <algorithm>
#include <iostream>
#include <string>

using yatta::Buffer;
using yatta::MemoryRange;
using yatta::Directory;


//// Forward Declarations
static bool MemoryRange_ConstructionTest();
static bool MemoryRange_AssignmentTest();
static bool MemoryRange_MethodTest();
static bool MemoryRange_IOTest();
static bool Buffer_ConstructionTest();
static bool Buffer_AssignmentTest();
static bool Buffer_MethodTest();
static bool Buffer_IOTest();
static bool Buffer_CompressionTest();
static bool Buffer_DiffTest();
static bool Directory_ConstructionTest();
static bool Directory_MethodTest();
static bool Directory_ManipulationTest();


template <typename FirstFunc, typename ...RestFuncs>
static bool RunTests(const FirstFunc& firstFunc, const RestFuncs&... restFuncs) noexcept
{
    auto result = firstFunc();

    // For each remaining member of the parameter pack, recursively call this function
    if constexpr (sizeof...(restFuncs) > 0) {
        const auto restResults = RunTests(restFuncs...);
        result = result && restResults;
    }

    return result;
}

int main() noexcept
{
    return RunTests(
        MemoryRange_ConstructionTest,
        MemoryRange_AssignmentTest,
        MemoryRange_MethodTest,
        MemoryRange_IOTest,

        Buffer_ConstructionTest,
        Buffer_AssignmentTest,
        Buffer_MethodTest,
        Buffer_IOTest,
        Buffer_CompressionTest,
        Buffer_DiffTest,

        Directory_ConstructionTest,
        Directory_MethodTest,
        Directory_ManipulationTest
    ) ? 0 : 1;
}

static bool MemoryRange_ConstructionTest()
{
    // Ensure we can make empty memory ranges
    Buffer buffer;
    const MemoryRange memRange(buffer.size(), buffer.bytes());
    if (memRange.empty()) {
        // Ensure we can make a large memory range
        Buffer largeBuffer(1234ULL);
        const MemoryRange largeMemRange(largeBuffer.size(), largeBuffer.bytes());
        if (largeMemRange.hasData()) {
            // Ensure move constructor works
            const MemoryRange moveMemRange(MemoryRange(largeBuffer.size(), largeBuffer.bytes()));
            if (moveMemRange.size() == 1234ULL) {
                // Ensure copy constructor works
                largeBuffer[0] = static_cast<std::byte>(255U);
                const MemoryRange& copyMemRange(moveMemRange);
                if (copyMemRange[0] == largeBuffer[0]) {
                    // Ensure pointers match
                    if (&copyMemRange[0] == &largeBuffer[0]) {
                        std::cout << "Memory Range Construction Test - Success\n";
                        return true; // Success
                    }
                }
            }
        }
    }

    std::cout << "Memory Range Construction Test - Failure\n";
    return false; // Failure
}

static bool MemoryRange_AssignmentTest()
{
    Buffer bufferA(1234ULL);
    Buffer bufferB(123456789ULL);
    MemoryRange rangeA(bufferA.size(), bufferA.bytes());
    MemoryRange rangeB(bufferB.size(), bufferB.bytes());
    rangeA[0] = static_cast<std::byte>(255U);
    rangeB[0] = static_cast<std::byte>(126U);

    // Ensure ranges are equal
    rangeA = rangeB;
    if (rangeA[0] == rangeB[0]) {
        Buffer bufferC(456);
        MemoryRange rangeC(bufferC.size(), bufferC.bytes());
        rangeC[0] = static_cast<std::byte>(64U);
        rangeA = rangeC;
        // Ensure rangeC is fully moved over to rangeA
        if (rangeA[0] == static_cast<std::byte>(64U)) {
            std::cout << "Memory Range Assignment Test - Success\n";
            return true; // Success
        }
    }

    std::cout << "Memory Range Assignment Test - Failure\n";
    return false; // Failure
}

static bool MemoryRange_MethodTest()
{
    Buffer buffer;
    MemoryRange memRange(buffer.size(), buffer.bytes());
    // Ensure the memory range is reassignable
    if (memRange.empty()) {
        buffer.resize(1234ULL);
        memRange = MemoryRange(buffer.size(), buffer.bytes());
        // Ensure memory range has data
        if (memRange.hasData()) {
            // Ensure memory range size is the same as the buffer
            if (memRange.size() == 1234ULL) {
                // Ensure we can hash the memory range
                if (const auto hash = memRange.hash(); hash != 0ULL && hash != yatta::ZeroHash) {
                    // Ensure we can return a char array
                    if (const char* const cArray = memRange.charArray(); cArray != nullptr) {
                        // Ensure we can return a byte array
                        if (const std::byte* const bytes = memRange.bytes(); bytes != nullptr) {
                            // Ensure both char array and byte array are the same underlying pointer
                            if (static_cast<const void*>(cArray) == static_cast<const void*>(bytes)) {
                                std::cout << "Memory Range Method Test - Success\n";
                                return true; // Success
                            }
                        }
                    }
                }
            }
        }
    }

    std::cout << "Memory Range Method Test - Failure\n";
    return false; // Failure
}

static bool MemoryRange_IOTest()
{
    // Ensure object IO is correct
    Buffer buffer(sizeof(int));
    MemoryRange memRange(buffer.size(), buffer.bytes());
    constexpr int in_int(64);
    memRange.in_type(in_int);
    int out_int(0);
    memRange.out_type(out_int);
    if (in_int == out_int) {
        // Ensure raw pointer IO is correct
        const char word[28] = "This is a sample sentence.\0";
        constexpr size_t sentenceSize = sizeof(char) * 28ULL;
        buffer.resize(sentenceSize + sizeof(int));
        memRange = MemoryRange(buffer.size(), buffer.bytes());
        memRange.in_raw(&word, sentenceSize, sizeof(int));

        struct TestStructure {
            int a;
            char b[28];
        } combinedOutput{};
        memRange.out_raw(&combinedOutput, sizeof(TestStructure));

        if (combinedOutput.a == in_int && std::string(combinedOutput.b) == word) {
            std::cout << "Memory Range IO Test - Success\n";
            return true; // Success
        }
    }

    std::cout << "Memory Range IO Test - Failure\n";
    return false; // Failure
}

static bool Directory_ConstructionTest()
{
    // Ensure we can make an empty directory
    Directory directory;
    if (directory.empty()) {
        // Ensure we can virtualize directories
        Directory dirA(Directory::GetRunningDirectory() + "/old");
        if (dirA.hasFiles()) {
            // Ensure move constructor works
            Directory moveDirectory = Directory(Directory::GetRunningDirectory() + "/old");
            if (moveDirectory.fileCount() == dirA.fileCount()) {
                // Ensure copy constructor works
                const Directory& copyDir(moveDirectory);
                if (copyDir.fileSize() == moveDirectory.fileSize()) {
                    std::cout << "Directory Construction Test - Success\n";
                    return true; // Success
                }
            }
        }
    }

    std::cout << "Directory Construction Test - Failure\n";
    return false; // Failure
}

static bool Buffer_ConstructionTest()
{
    // Ensure we can make empty buffers
    Buffer buffer;
    if (buffer.empty()) {
        // Ensure we can make a large buffer
        Buffer largeBuffer(1234ULL);
        if (largeBuffer.hasData()) {
            // Ensure move constructor works
            Buffer moveBuffer(Buffer(1234ULL));
            if (moveBuffer.size() == 1234ULL) {
                // Ensure copy constructor works
                largeBuffer[0] = static_cast<std::byte>(255U);
                const Buffer copyBuffer(largeBuffer);
                if (copyBuffer[0] == largeBuffer[0]) {
                    std::cout << "Buffer Construction Test - Success\n";
                    return true; // Success
                }
            }
        }
    }

    std::cout << "Buffer Construction Test - Failure\n";
    return false; // Failure
}

static bool Buffer_AssignmentTest()
{
    Buffer bufferA(1234ULL);
    Buffer bufferB(123456789ULL);
    bufferA[0] = static_cast<std::byte>(255U);
    bufferB[0] = static_cast<std::byte>(126U);

    // Ensure buffers are equal
    // To Do: Add equality operator
    bufferA = bufferB;
    if (bufferA[0] == bufferB[0]) {
        Buffer bufferC(456);
        bufferC[0] = static_cast<std::byte>(64U);
        bufferA = std::move(bufferC);
        // Ensure bufferC is fully moved over to bufferA
        if (bufferA[0] == static_cast<std::byte>(64U)) {
            // Ensure we throw if going out of bounds
            try {
                bufferA[1234] = static_cast<std::byte>(64U);
                const std::byte constLookup = bufferA[1234];
                bufferA[0] = constLookup;
            }
            catch (const std::runtime_error&) {
                std::cout << "Buffer Assignment Test - Success\n";
                return true; // Success
            }
        }
    }

    std::cout << "Buffer Assignment Test - Failure\n";
    return false; // Failure
}

static bool Buffer_MethodTest()
{
    Buffer buffer;
    // Ensure the buffer is empty
    if (buffer.empty()) {
        buffer.resize(1234ULL);
        // Ensure we can resize the buffer with data
        if (buffer.hasData()) {
            // Ensure the size is 1234
            if (buffer.size() == 1234ULL) {
                // Ensure the capacity is doubled
                if (buffer.capacity() == 2468ULL) {
                    // Ensure we can hash the buffer
                    if (const auto hash = buffer.hash(); hash != 0ULL && hash != yatta::ZeroHash) {
                        // Ensure we can return a char array
                        if (const char* const cArray = buffer.charArray(); cArray != nullptr) {
                            // Ensure we can return a byte array
                            if (const std::byte* const bytes = buffer.bytes(); bytes != nullptr) {
                                // Ensure both char array and byte array are the same underlying pointer
                                if (static_cast<const void*>(cArray) == static_cast<const void*>(bytes)) {
                                    // Ensure we can shrink the buffer
                                    buffer.shrink();
                                    if (buffer.size() == 1234ULL && buffer.capacity() == 1234ULL) {
                                        // Ensure we can completely free the buffer
                                        buffer.clear();
                                        buffer.shrink();
                                        if (buffer.empty()) {
                                            std::cout << "Buffer Method Test - Success\n";
                                            return true; // Success
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    std::cout << "Buffer Method Test - Failure\n";
    return false; // Failure
}

static bool Buffer_IOTest()
{
    // Ensure we can't perform IO on an empty buffer
    try {
        Buffer buffer;
        constexpr size_t largeValue(123456789ULL);
        buffer.in_type(largeValue);
    }
    catch (std::runtime_error&) {
        // Ensure object IO is correct
        Buffer buffer(sizeof(std::byte));
        constexpr auto in_byte(static_cast<std::byte>(64));
        buffer.in_type(in_byte);
        std::byte out_byte;
        buffer.out_type(out_byte);
        if (in_byte == out_byte) {
            // Ensure raw pointer IO is correct
            const char word[28] = "This is a sample sentence.\0";
            constexpr size_t sentenceSize = sizeof(char) * 28ULL;
            buffer.resize(sentenceSize + sizeof(std::byte));
            buffer.in_raw(&word, sentenceSize, sizeof(std::byte));

            struct TestStructure {
                std::byte a;
                char b[28];
            } combinedOutput{};
            buffer.out_raw(&combinedOutput, sizeof(TestStructure));

            if (combinedOutput.a == in_byte && std::string(combinedOutput.b) == word) {
                std::cout << "Buffer IO Test - Success\n";
                return true; // Success
            }
        }
    }

    std::cout << "Buffer IO Test - Failure\n";
    return false; // Failure
}

static bool Buffer_CompressionTest()
{
    // Ensure we cannot compress or decompress an empty or incorrect buffer
    Buffer buffer;
    const auto badResult1 = buffer.compress();
    const auto badResult2 = badResult1->decompress();
    if (!badResult1 && !badResult2) {
        // The structure we'll compress and decompress
        struct TestStructure {
            int a = 0;
            float b = 0.F;
            char c[256] = { '\0' };
        };
        // Create a buffer and load it with test data
        buffer.resize(sizeof(TestStructure));
        constexpr TestStructure testData{
            1234,
            567.890F,
            "QWEQWEQWEEQWEQWEQWEQWEEEEEEEQWEQWEQWEQWEQQWEQWEQWEEEEEE623785623"
            "4652837444444443333333333333364637896463QWQWEQWEQWWEQWEQWEEEEEEE"
            "QWEQWEQWEEQWEQWEQWEQWNOUNOUNOUNOUNOUNOUNOU4EQWEQWEEEEEE623785623"
            "4652837444444443333333333333364637896463QWQWEQWEQWWEQWEQWEEEEE\0"
        };
        buffer.in_type(testData);

        // Attempt to compress the buffer
        if (const auto compressedBuffer = buffer.compress()) {
            // Attempt to decompress the compressed buffer
            if (const auto decompressedBuffer = compressedBuffer->decompress()) {
                // Dump buffer data back into test structure
                TestStructure decompressedData;
                decompressedBuffer->out_type(decompressedData);

                // Ensure data matches
                if (testData.a == decompressedData.a && testData.b == decompressedData.b && std::strcmp(testData.c, decompressedData.c) == 0) {
                    std::cout << "Buffer Compression/Decompression Test - Success\n";
                    return true; // Success
                }
            }
        }
    }

    std::cout << "Buffer Compression/Decompression Test - Failure\n";
    return false; // Failure
}

static bool Buffer_DiffTest()
{
    // Ensure we cannot diff or patch an empty or incorrect buffer
    Buffer bufferA;
    Buffer bufferB;
    const auto badResult1 = bufferA.diff(bufferB);
    const auto badResult2 = badResult1->patch(bufferB);
    if (!badResult1 && !badResult2) {
        struct Foo {
            int a = 0;
            float b = 0.0F;
            char c[128] = { '\0' };
        } dataA{ 1234, 567.890F, "This is an example of a long string within the structure named Foo.\0" };
        struct Bar {
            float b = 0.0F;
            int c = 0;
            char a[128] = { '\0' };
        } dataB{ 890.567F, 4321, "This is an example of a long string within the structure named Bar.\0", };
        bufferA.resize(sizeof(Foo));
        bufferB.resize(sizeof(Bar));
        bufferA.in_type(dataA);
        bufferB.in_type(dataB);

        // Ensure we've generated an instruction set
        if (const auto diffBuffer = bufferA.diff(bufferB)) {
            // Check that we've actually converted from A to B
            if (const auto patchedBuffer = bufferA.patch(*diffBuffer)) {
                Bar dataC;
                patchedBuffer->out_type(dataC);
                if (std::strcmp(dataB.a, dataC.a) == 0 && dataB.b == dataC.b && dataB.c == dataC.c && patchedBuffer->hash() == bufferB.hash()) {
                    std::cout << "Buffer Diff/Patch Test - Success\n";
                    return true; // Success
                }
            }
        }
    }

    std::cout << "Buffer Diff/Patch Test - Failure\n";
    return false; // Failure
}

static bool Directory_MethodTest()
{
    // Verify empty directories
    Directory directory;
    if (directory.empty()) {
        // Verify that we can input folders
        directory.in_folder(Directory::GetRunningDirectory() + "/old");
        if (directory.hasFiles()) {
            // Ensure we have 4 files all together
            if (directory.fileCount() == 4ULL) {
                // Ensure the total size is as expected
                if (directory.fileSize() == 147777ULL) {
                    // Ensure we can hash an actual directory
                    if (const auto hash = directory.hash(); hash != yatta::ZeroHash) {
                        // Ensure we can clear a directory
                        directory.clear();
                        if (directory.empty()) {
                            // Ensure we can hash an empty directory
                            if (directory.hash() == yatta::ZeroHash) {
                                std::cout << "Directory Method Test - Success\n";
                                return true; // Success
                            }
                        }
                    }
                }
            }
        }
    }

    std::cout << "Directory Method Test - Failure\n";
    return false; // Failure
}

static bool Directory_ManipulationTest()
{
    // Verify empty directories
    Directory directory;
    if (directory.empty()) {
        // Verify that we can add multiple folders together
        directory.in_folder(Directory::GetRunningDirectory() + "/old");
        directory.in_folder(Directory::GetRunningDirectory() + "/new");
        // Ensure we have 8 files all together
        if (directory.fileCount() == 8ULL) {
            // Ensure the total size is as expected
            if (directory.fileSize() == 189747ULL) {
                // Reset the directory to just the 'old' folder, hash it
                directory = Directory(Directory::GetRunningDirectory() + "/old");
                if (const auto oldHash = directory.hash(); oldHash != yatta::ZeroHash) {
                    // Overwrite the /old folder, make sure hashes match
                    directory.out_folder(Directory::GetRunningDirectory() + "/old");
                    directory = Directory(Directory::GetRunningDirectory() + "/old");
                    if (const auto newHash = directory.hash(); newHash != yatta::ZeroHash && newHash == oldHash) {
                        // Ensure we can dump a directory as a package
                        if (const auto package = directory.out_package("package")) {
                            if (package->hasData()) {
                                // Ensure we can import a package
                                directory.clear();
                                directory.in_package(*package);
                                // Ensure this new directory matches the old one
                                if (directory.fileSize() == 147777ULL && directory.fileCount() == 4ULL) {
                                    // Ensure new hash matches
                                    if (const auto packHash = directory.hash(); newHash != yatta::ZeroHash && packHash == newHash) {
                                        // Try to diff the old and new directories
                                        Directory newDirectory(Directory::GetRunningDirectory() + "/new");
                                        if (const auto deltaBuffer = directory.out_delta(newDirectory)) {
                                            // Try to patch the old directory into the new directory
                                            if (directory.in_delta(*deltaBuffer)) {
                                                // Ensure the hashes match
                                                if (directory.hash() == newDirectory.hash()) {
                                                    // Overwrite the /new folder, make sure the hashes match
                                                    directory.out_folder(Directory::GetRunningDirectory() + "/new");
                                                    directory = Directory(Directory::GetRunningDirectory() + "/new");
                                                    if (directory.hash() == newDirectory.hash()) {
                                                        std::cout << "Directory IO Test - Success\n";
                                                        return true; // Success
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    std::cout << "Directory IO Test - Failure\n";
    return false; // Failure
}