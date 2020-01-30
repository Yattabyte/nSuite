#include "yatta.hpp"
#include <iostream>


// Convenience Definitions
constexpr int SUCCESS = 0;
constexpr int FAILURE = 1;
using yatta::Buffer;

// Forward Declarations
bool Buffer_ConstructionTest();
bool Buffer_AssignmentTest();
bool Buffer_MethodTest();
bool Buffer_IOTest();
bool Buffer_CompressionTest();
bool Buffer_DiffTest();

int main() noexcept
{
    if (Buffer_ConstructionTest() &&
        Buffer_AssignmentTest() &&
        Buffer_MethodTest() &&
        Buffer_IOTest() &&
        Buffer_CompressionTest() &&
        Buffer_DiffTest())
        return SUCCESS;

    std::cout << "Error while running Buffer Tests\n";
    return FAILURE;
}

bool Buffer_ConstructionTest()
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
                    return true; // Success
                }
            }
        }
    }

    return false; // Failure
}

bool Buffer_AssignmentTest()
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
                return true; // Success
            }
        }
    }

    return false; // Failure
}

bool Buffer_MethodTest()
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

    return false; // Failure
}

bool Buffer_IOTest()
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
                return true; // Success
            }
        }
    }

    return false; // Failure
}

bool Buffer_CompressionTest()
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
                    return true; // Success
                }
            }
        }
    }

    return false; // Failure
}

bool Buffer_DiffTest()
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
                    return true; // Success
                }
            }
        }
    }

    return false; // Failure
}