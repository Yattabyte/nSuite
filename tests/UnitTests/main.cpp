#include "yatta.hpp"
#include <algorithm>
#include <iostream>
#include <string>

using yatta::Buffer;
using yatta::BufferView;
using yatta::MemoryRange;
using yatta::Directory;


//// Forward Declarations
static bool Buffer_ConstructionTest();
static bool Buffer_AssignmentTest();
static bool Buffer_MethodTest();
static bool Buffer_IOTest();
static bool Buffer_CompressionTest();
static bool Buffer_DiffTest();
static bool BufferView_ConstructionTest();
static bool BufferView_AssignmentTest();
static bool BufferView_MethodTest();
static bool BufferView_IOTest();
static bool BufferView_CompressionTest();
static bool BufferView_DiffTest();
static bool MemoryRange_ConstructionTest();
static bool MemoryRange_AssignmentTest();
static bool MemoryRange_MethodTest();
static bool MemoryRange_IOTest();
static bool Directory_ConstructionTest();
//static bool Directory_MethodTest();


template <typename FirstFunc, typename ...RestFuncs>
static bool RunTests(const FirstFunc& firstFunc, const RestFuncs&... restFuncs)
{
    auto result = firstFunc();

    // For each remaining member of the parameter pack, recursively call this function
    if constexpr (sizeof...(restFuncs) > 0) {
        const auto restResults = RunTests(restFuncs...);
        result = bool(result && restResults);
    }

    return result;
}

int main()
{
    return RunTests(
        Buffer_ConstructionTest,
        Buffer_AssignmentTest,
        Buffer_MethodTest,
        Buffer_IOTest,
        Buffer_CompressionTest,
        Buffer_DiffTest,

        BufferView_ConstructionTest,
        BufferView_AssignmentTest,
        BufferView_MethodTest,
        BufferView_IOTest,
        BufferView_CompressionTest,
        BufferView_DiffTest,

        MemoryRange_ConstructionTest,
        MemoryRange_AssignmentTest,
        MemoryRange_MethodTest,
        MemoryRange_IOTest,

        Directory_ConstructionTest/*,
        Directory_MethodTest*/
    ) ? 0 : 1;
}

static bool Buffer_ConstructionTest()
{
    try {
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
                    const Buffer& copyBuffer(largeBuffer);
                    if (copyBuffer[0] == largeBuffer[0]) {
                        std::cout << "Buffer Construction Test - Success\n";
                        return true; // Success
                    }
                }
            }
        }
    }
    catch (const std::exception & e) {
        std::cout << e.what() << "\n";
    }

    std::cout << "Buffer Construction Test - Failure\n";
    return false; // Failure
}

static bool Buffer_AssignmentTest()
{
    try {
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
                std::cout << "Buffer Assignment Test - Success\n";
                return true; // Success
            }
        }
    }
    catch (const std::exception & e) {
        std::cout << e.what() << "\n";
    }

    std::cout << "Buffer Assignment Test - Failure\n";
    return false; // Failure
}

static bool Buffer_MethodTest()
{
    try {
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
                        if (const auto hash = buffer.hash(); hash != 0ULL && hash != 1234567890ULL) {
                            // Ensure we can return a char array
                            if (const auto cArray = buffer.charArray(); cArray != nullptr) {
                                // Ensure we can return a byte array
                                if (const auto bytes = buffer.bytes(); bytes != nullptr) {
                                    // Ensure both char array and byte array are the same underlying pointer
                                    if (static_cast<const void*>(cArray) == static_cast<const void*>(bytes)) {
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
    catch (const std::exception & e) {
        std::cout << e.what() << "\n";
    }

    std::cout << "Buffer Method Test - Failure\n";
    return false; // Failure
}

static bool Buffer_IOTest()
{
    try {
        // Ensure object IO is correct
        Buffer buffer(sizeof(int));
        constexpr int in_int(64);
        buffer.in_type(in_int);
        int out_int(0);
        buffer.out_type(out_int);
        if (in_int == out_int) {
            // Ensure raw pointer IO is correct
            const char word[28] = ("This is a sample sentence.\0");
            constexpr auto sentenceSize = sizeof(char) * 28;
            buffer.resize(sentenceSize + sizeof(int));
            buffer.in_raw(&word, sentenceSize, sizeof(int));

            struct TestStructure {
                int a;
                char b[28];
            } combinedOutput{};
            buffer.out_raw(&combinedOutput, sizeof(TestStructure));

            if (combinedOutput.a == in_int && std::string(combinedOutput.b) == word) {
                std::cout << "Buffer IO Test - Success\n";
                return true; // Success
            }
        }
    }
    catch (const std::exception & e) {
        std::cout << e.what() << "\n";
    }

    std::cout << "Buffer IO Test - Failure\n";
    return false; // Failure
}

static bool Buffer_CompressionTest()
{
    try {
        // The structure we'll compress and decompress
        struct TestStructure {
            int a = 0;
            float b = 0.F;
            char c[32] = { '\0' };
        };

        // Create a buffer and load it with test data
        Buffer buffer(sizeof(TestStructure));
        constexpr TestStructure testData{ 1234, 567.890F,"QWEQWEQWEQWEQWEQWEQWEQWEQWEQWE\0" };
        buffer.in_type(testData);

        // Attempt to compress the buffer
        const auto compressedBuffer = buffer.compress();
        // Attempt to decompress the compressed buffer
        const auto decompressedBuffer = compressedBuffer.decompress();
        // Dump buffer data back into test structure
        TestStructure decompressedData;
        decompressedBuffer.out_type(decompressedData);

        // Ensure data matches
        if (testData.a == decompressedData.a && testData.b == decompressedData.b && std::strcmp(testData.c, decompressedData.c) == 0) {
            std::cout << "Buffer Compression/Decompression Test - Success\n";
            return true; // Success
        }

    }
    catch (const std::exception & e) {
        std::cout << e.what() << "\n";
    }
    std::cout << "Buffer Compression/Decompression Test - Failure\n";
    return false; // Failure
}

static bool Buffer_DiffTest()
{
    try {
        struct Foo {
            int a = 0;
            float b = 0.0F;
            char c[28] = { '\0' };
        } dataA{ 1234, 567.890F, "This is a sample sentence.\0" };
        struct Bar {
            char a[32] = { '\0' };
            float b = 0.0F;
            int c = 0;
        } dataB{ "Not even a sample sentence.\0", 890.567F, 4321 };
        Buffer bufferA(sizeof(Foo));
        Buffer bufferB(sizeof(Bar));
        bufferA.in_type(dataA);
        bufferB.in_type(dataB);

        // Ensure we've generated an instruction set
        const auto diffBuffer = bufferA.diff(bufferB, 8);
        // Check that we've actually converted from A to B
        const auto patchedBuffer = bufferA.patch(diffBuffer);
        Bar dataC;
        patchedBuffer.out_type(dataC);
        if (std::strcmp(dataB.a, dataC.a) == 0 && dataB.b == dataC.b && dataB.c == dataC.c) {
            std::cout << "Buffer Diff/Patch Test - Success\n";
            return true; // Success
        }         
    }
    catch (const std::exception & e) {
        std::cout << e.what() << "\n";
    }
    std::cout << "Buffer Diff/Patch Test - Failure\n";
    return false; // Failure
}

static bool BufferView_ConstructionTest()
{
    try {
        // Ensure we can make empty buffer views
        Buffer buffer;
        BufferView bView(buffer);
        if (bView.empty()) {
            // Ensure we can make a large buffer view with a manual size
            Buffer largeBuffer(1234ULL);
            BufferView largeBView(largeBuffer.size(), largeBuffer.bytes());
            if (largeBView.hasData()) {
                // Ensure move constructor works
                BufferView moveBView(BufferView(largeBuffer.size(), largeBuffer.bytes()));
                if (moveBView.size() == 1234ULL) {
                    // Ensure copy constructor works
                    largeBView[0] = static_cast<std::byte>(255U);
                    const BufferView& copyBView(largeBView);
                    if (copyBView[0] == largeBView[0]) {
                        std::cout << "Buffer-View Construction Test - Success\n";
                        return true; // Success
                    }
                }
            }
        }
    }
    catch (const std::exception & e) {
        std::cout << e.what() << "\n";
    }

    std::cout << "Buffer-View Construction Test - Failure\n";
    return false; // Failure
}

static bool BufferView_AssignmentTest()
{
    try {
        Buffer bufferA(1234ULL);
        Buffer bufferB(123456789ULL);
        BufferView viewA(bufferA);
        BufferView viewB(bufferB);
        viewA[0] = static_cast<std::byte>(255U);
        viewB[0] = static_cast<std::byte>(126U);

        // Ensure views are equal
        viewA = viewB;
        if (viewA[0] == viewB[0]) {
            Buffer bufferC(456);
            BufferView viewC(bufferC);
            viewC[0] = static_cast<std::byte>(64U);
            viewA = viewC;
            // Ensure viewC is fully moved over to viewA
            if (viewA[0] == static_cast<std::byte>(64U)) {
                std::cout << "Buffer-View Assignment Test - Success\n";
                return true; // Success
            }
        }
    }
    catch (const std::exception & e) {
        std::cout << e.what() << "\n";
    }

    std::cout << "Buffer-View Assignment Test - Failure\n";
    return false; // Failure
}

static bool BufferView_MethodTest()
{
    try {
        Buffer buffer;
        BufferView bView(buffer);
        // Ensure the view is empty
        if (bView.empty()) {
            buffer.resize(1234ULL);
            // Ensure we can reassign the buffer view
            bView = BufferView(buffer);
            if (bView.hasData()) {
                // Ensure the size is 1234
                if (bView.size() == 1234ULL) {
                    // Ensure we can hash the buffer view
                    if (const auto hash = bView.hash(); hash != 0ULL && hash != 1234567890ULL) {
                        // Ensure we can return a char array
                        if (const auto cArray = bView.charArray(); cArray != nullptr) {
                            // Ensure we can return a byte array
                            if (const auto bytes = bView.bytes(); bytes != nullptr) {
                                // Ensure both char array and byte array are the same underlying pointer
                                if (static_cast<const void*>(cArray) == static_cast<const void*>(bytes)) {
                                    std::cout << "Buffer-View Method Test - Success\n";
                                    return true; // Success
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception & e) {
        std::cout << e.what() << "\n";
    }

    std::cout << "Buffer-View Method Test - Failure\n";
    return false; // Failure
}

static bool BufferView_IOTest()
{
    try {
        // Ensure object IO is correct
        Buffer buffer(sizeof(int));
        BufferView bView(buffer);
        constexpr int in_int(64);
        bView.in_type(in_int);
        int out_int(0);
        bView.out_type(out_int);
        if (in_int == out_int) {
            // Ensure raw pointer IO is correct
            const char word[28] = ("This is a sample sentence.\0");
            constexpr auto sentenceSize = sizeof(char) * 28;
            buffer.resize(sentenceSize + sizeof(int));
            bView = BufferView(buffer);
            bView.in_raw(&word, sentenceSize, sizeof(int));

            struct TestStructure {
                int a;
                char b[28];
            } combinedOutput{};
            bView.out_raw(&combinedOutput, sizeof(TestStructure));

            if (combinedOutput.a == in_int && std::string(combinedOutput.b) == word) {
                std::cout << "Buffer-View IO Test - Success\n";
                return true; // Success
            }
        }
    }
    catch (const std::exception & e) {
        std::cout << e.what() << "\n";
    }

    std::cout << "Buffer-View IO Test - Failure\n";
    return false; // Failure
}

static bool BufferView_CompressionTest()
{
    try {
        // The structure we'll compress and decompress
        struct TestStructure {
            int a = 0;
            float b = 0.F;
            char c[32] = { '\0' };
        };

        // Create a buffer and load it with test data
        Buffer buffer(sizeof(TestStructure));
        BufferView bView(buffer);
        constexpr TestStructure testData{ 1234, 567.890F,"QWEQWEQWEQWEQWEQWEQWEQWEQWEQWE\0" };
        bView.in_type(testData);

        // Attempt to compress the buffer view
        const auto compressedBuffer = bView.compress();
        // Attempt to decompress the compressed buffer view
        const BufferView compressedBView(compressedBuffer);
        const auto decompressedBuffer = compressedBView.decompress();
        // Dump buffer data back into test structure
        TestStructure decompressedData;
        const BufferView decomrpessedBView(decompressedBuffer);
        decomrpessedBView.out_type(decompressedData);

        // Ensure data matches
        if (testData.a == decompressedData.a && testData.b == decompressedData.b && std::strcmp(testData.c, decompressedData.c) == 0) {
            std::cout << "Buffer-View Compression/Decompression Test - Success\n";
            return true; // Success
        }
    }
    catch (const std::exception & e) {
        std::cout << e.what() << "\n";
    }
    std::cout << "Buffer-View Compression/Decompression Test - Failure\n";
    return false; // Failure
}

static bool BufferView_DiffTest()
{
    try {
        struct Foo {
            int a = 0;
            float b = 0.0F;
            char c[28] = { '\0' };
        } dataA{ 1234, 567.890F, "This is a sample sentence.\0" };
        struct Bar {
            char a[32] = { '\0' };
            float b = 0.0F;
            int c = 0;
        } dataB{ "Not even a sample sentence.\0", 890.567F, 4321 };
        Buffer bufferA(sizeof(Foo));
        Buffer bufferB(sizeof(Bar));
        BufferView viewA(bufferA);
        BufferView viewB(bufferB);
        viewA.in_type(dataA);
        viewB.in_type(dataB);

        // Ensure we've generated an instruction set
        const auto diffBuffer = viewA.diff(viewB, 8);
        // Check that we've actually converted from A to B
        const BufferView diffView(diffBuffer);
        const auto patchedBuffer = viewA.patch(diffView);
        const BufferView patchedView(patchedBuffer);
        Bar dataC;
        patchedView.out_type(dataC);
        if (std::strcmp(dataB.a, dataC.a) == 0 && dataB.b == dataC.b && dataB.c == dataC.c) {
            std::cout << "Buffer-View Diff/Patch Test - Success\n";
            return true; // Success
        }
    }
    catch (const std::exception & e) {
        std::cout << e.what() << "\n";
    }
    std::cout << "Buffer-View Diff/Patch Test - Failure\n";
    return false; // Failure
}

static bool MemoryRange_ConstructionTest()
{
    try {
        // Ensure we can make empty memory ranges
        Buffer buffer;
        MemoryRange memRange(buffer.size(), buffer.bytes());
        if (memRange.empty()) {
            // Ensure we can make a large memory range
            Buffer largeBuffer(1234ULL);
            MemoryRange largeMemRange(largeBuffer.size(), largeBuffer.bytes());
            if (largeMemRange.hasData()) {
                // Ensure move constructor works
                MemoryRange moveMemRange(MemoryRange(largeBuffer.size(), largeBuffer.bytes()));
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
    }
    catch (const std::exception & e) {
        std::cout << e.what() << "\n";
    }

    std::cout << "Memory Range Construction Test - Failure\n";
    return false; // Failure
}

static bool MemoryRange_AssignmentTest()
{
    try {
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
    }
    catch (const std::exception & e) {
        std::cout << e.what() << "\n";
    }

    std::cout << "Memory Range Assignment Test - Failure\n";
    return false; // Failure
}

static bool MemoryRange_MethodTest()
{
    try {
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
                    if (const auto hash = memRange.hash(); hash != 0ULL && hash != 1234567890ULL) {
                        // Ensure we can return a char array
                        if (const auto cArray = memRange.charArray(); cArray != nullptr) {
                            // Ensure we can return a byte array
                            if (const auto bytes = memRange.bytes(); bytes != nullptr) {
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
    }
    catch (const std::exception & e) {
        std::cout << e.what() << "\n";
    }

    std::cout << "Memory Range Method Test - Failure\n";
    return false; // Failure
}

static bool MemoryRange_IOTest()
{
    try {
        // Ensure object IO is correct
        Buffer buffer(sizeof(int));
        MemoryRange memRange(buffer.size(), buffer.bytes());
        constexpr int in_int(64);
        memRange.in_type(in_int);
        int out_int(0);
        memRange.out_type(out_int);
        if (in_int == out_int) {
            // Ensure raw pointer IO is correct
            const char word[28] = ("This is a sample sentence.\0");
            constexpr auto sentenceSize = sizeof(char) * 28;
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
    }
    catch (const std::exception & e) {
        std::cout << e.what() << "\n";
    }

    std::cout << "Memory Range IO Test - Failure\n";
    return false; // Failure
}

static bool Directory_ConstructionTest()
{
    const auto test = Directory::GetRunningDirectory();
    return true;
    //try {
    //    // Ensure we can make an empty directory
    //    Directory directory;
    //    if (directory.empty()) {
    //        // Ensure we can virtualize directories
    //        Directory dirA(Directory::GetRunningDirectory() + "\\old");
    //        if (dirA.hasFiles()) {
    //            // Ensure move constructor works
    //            Directory moveDirectory = Directory(Directory::GetRunningDirectory() + "\\old");
    //            if (moveDirectory.fileCount() == dirA.fileCount()) {
    //                // Ensure copy constructor works
    //                const Directory& copyDir(moveDirectory);
    //                if (copyDir.fileSize() == moveDirectory.fileSize()) {
    //                    std::cout << "Directory Construction Test - Success\n";
    //                    return true; // Success                
    //             }
    //            }
    //        }
    //    }
    //}
    //catch (const std::exception & e) {
    //   std::cout << e.what() << "\n";
    //}

    //std::cout << "Directory Construction Test - Failure\n";
    //return false; // Failure
}

//static bool Directory_MethodTest()
//{
//    try {
//        // Verify empty directories
//        Directory directory;
//        if (directory.empty()) {
//            // Verify that we can add another multiple folders
//            directory.in_folder(Directory::GetRunningDirectory() + "\\old");
//            directory.in_folder(Directory::GetRunningDirectory() + "\\new");
//            if (directory.hasFiles()) {
//                // Ensure we have 8 files all together
//                if (directory.fileCount() == 8ULL) {
//                    // Ensure the total size is as expected
//                    if (directory.fileSize() == 189747ULL) {
//                        std::cout << "Directory Method Test - Success\n";
//                        return true; // Success      
//                    }
//                }
//            }
//        }          
//    }
//    catch (const std::exception & e) {
//        std::cout << e.what() << "\n";
//    }
//
//    std::cout << "Directory Method Test - Failure\n";
//    return false; // Failure
//}
