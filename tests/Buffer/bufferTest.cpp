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

int main()
{
    if (!Buffer_ConstructionTest() ||
        !Buffer_AssignmentTest() ||
        !Buffer_MethodTest() ||
        !Buffer_IOTest() ||
        !Buffer_CompressionTest() ||
        !Buffer_DiffTest())
        exit(FAILURE); // LCOV_EXCL_LINE

    exit(SUCCESS);
}

bool Buffer_ConstructionTest()
{
    // Ensure we can make empty buffers
    Buffer buffer;
    if (!buffer.empty() || buffer.hasData())
        return false; // LCOV_EXCL_LINE

    // Ensure we can construct a buffer
    Buffer largeBuffer(1234ULL);
    if (!largeBuffer.hasData() || largeBuffer.empty())
        return false; // LCOV_EXCL_LINE

    // Ensure move constructor works
    Buffer moveBuffer(Buffer(1234ULL));
    if (moveBuffer.size() != 1234ULL)
        return false; // LCOV_EXCL_LINE

    // Ensure copy constructor works
    largeBuffer[0] = static_cast<std::byte>(255U);
    const Buffer copyBuffer(largeBuffer);
    if (copyBuffer[0] != largeBuffer[0])
        return false; // LCOV_EXCL_LINE

    // Success
    return true;
}

bool Buffer_AssignmentTest()
{
    // Ensure buffers are equal
    Buffer bufferA;
    Buffer bufferB(123456789ULL);
    bufferB[0] = static_cast<std::byte>(126U);
    bufferA = bufferB;
    if (bufferA[0] != bufferB[0])
        return false; // LCOV_EXCL_LINE

    // Ensure bufferC is fully moved over to bufferA
    Buffer bufferC(456);
    bufferC[0] = static_cast<std::byte>(64U);
    bufferA = std::move(bufferC);
    if (bufferA[0] != static_cast<std::byte>(64U))
        return false; // LCOV_EXCL_LINE

    // Success
    return true;
}

bool Buffer_MethodTest()
{
    Buffer buffer;
    // Ensure the buffer is empty
    if (!buffer.empty() || buffer.hasData())
        return false; // LCOV_EXCL_LINE

    // Ensure we can resize the buffer with data
    buffer.resize(1234ULL);
    if (!buffer.hasData() || buffer.empty())
        return false; // LCOV_EXCL_LINE

    // Ensure the size is 1234
    if (buffer.size() != 1234ULL)
        return false; // LCOV_EXCL_LINE

    // Ensure the capacity is doubled
    if (buffer.capacity() != 2468ULL)
        return false; // LCOV_EXCL_LINE

    // Ensure we can explicitly set the capacity
    buffer.reserve(3000ULL);
    if (buffer.capacity() != 3000ULL)
        return false; // LCOV_EXCL_LINE

    // Ensure we can shrink the buffer
    buffer.shrink();
    if (buffer.size() != 1234ULL || buffer.capacity() != 1234ULL)
        return false; // LCOV_EXCL_LINE

    // Ensure we can completely free the buffer
    buffer.clear();
    buffer.shrink();
    if (!buffer.empty() || buffer.hasData())
        return false; // LCOV_EXCL_LINE

    // Success
    return true;
}

bool Buffer_IOTest()
{
    // Ensure we can push data
    Buffer buffer;
    constexpr size_t data1(123ULL);
    constexpr char data2('§');
    constexpr bool data3(true);
    constexpr int data4[10] = { 0,1,2,3,4,5,6,7,8,9 };
    const std::string data5 = "hello world";
    constexpr char data6[5] = { 'a', 'b', 'c', 'd', '\0' };
    buffer.push_type(data1);
    buffer.push_type(data2);
    buffer.push_type(data3);
    buffer.push_type(data4);
    buffer.push_type(data5);
    buffer.push_raw(data6, sizeof(char) * 5ULL);

    // Ensure we can pop data
    size_t data1_out(0ULL);
    char data2_out('\0');
    bool data3_out(false);
    int data4_out[10] = { 0 };
    std::string data5_out;
    char data6_out[5] = { '\0' };
    buffer.pop_raw(data6_out, sizeof(char) * 5ULL);
    buffer.pop_type(data5_out);
    buffer.pop_type(data4_out);
    buffer.pop_type(data3_out);
    buffer.pop_type(data2_out);
    buffer.pop_type(data1_out);

    // Ensure the data matches
    if (data1 != data1_out ||
        data2 != data2_out ||
        data3 != data3_out ||
        !std::equal(std::cbegin(data4), std::cend(data4), std::cbegin(data4_out)) ||
        data5 != data5_out ||
        std::string(data6) != std::string(data6_out))
        return false; // LCOV_EXCL_LINE

    // Success
    return true;
}

bool Buffer_CompressionTest()
{
    // Ensure we cannot compress or decompress an empty or incorrect buffer
    Buffer buffer;
    const auto badResult1 = buffer.compress();
    const auto badResult2 = badResult1->decompress();
    if (badResult1 || badResult2)
        return false; // LCOV_EXCL_LINE

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
    const auto compressedBuffer = buffer.compress();
    if (!compressedBuffer.has_value() || compressedBuffer->empty())
        return false; // LCOV_EXCL_LINE

    // Attempt to decompress the compressed buffer
    const auto decompressedBuffer = compressedBuffer->decompress();
    if (!decompressedBuffer.has_value() || decompressedBuffer->empty())
        return false; // LCOV_EXCL_LINE

    // Dump buffer data back into test structure
    TestStructure decompressedData;
    decompressedBuffer->out_type(decompressedData);

    // Ensure data matches
    if (testData.a != decompressedData.a || testData.b != decompressedData.b || std::strcmp(testData.c, decompressedData.c) != 0)
        return false; // LCOV_EXCL_LINE

    // Success
    return true;
}

bool Buffer_DiffTest()
{
    // Ensure we cannot diff or patch an empty or incorrect buffer
    Buffer bufferA;
    Buffer bufferB;
    const auto badResult1 = bufferA.diff(bufferB);
    const auto badResult2 = badResult1->patch(bufferB);
    if (badResult1 || badResult2)
        return false; // LCOV_EXCL_LINE

    struct Foo {
        int a = 0;
        float b = 0.0F;
        char c[128] = { '\0' };
    } dataA{ 1234, 567.890F, "This is an example of a long string within the structure named Foo.99999999999999999999999999999999999999999999999999999999999\0" };
    struct Bar {
        float b = 0.0F;
        int c = 0;
        char a[128] = { '\0' };
    } dataB{ 890.567F, 4321, "This is an example of a long string within the structure named Bar.88888888888888888888888888888888888888888888888888888888888\0", };
    bufferA.resize(sizeof(Foo));
    bufferB.resize(sizeof(Bar));
    bufferA.in_type(dataA);
    bufferB.in_type(dataB);

    // Ensure we've generated an instruction set
    const auto diffBuffer = bufferA.diff(bufferB);
    if (!diffBuffer.has_value() || diffBuffer->empty())
        return false; // LCOV_EXCL_LINE

    // Check that we've actually converted from A to B
    const auto patchedBuffer = bufferA.patch(*diffBuffer);
    if (!patchedBuffer.has_value() || patchedBuffer->empty())
        return false; // LCOV_EXCL_LINE

    Bar dataC;
    patchedBuffer->out_type(dataC);
    if (std::strcmp(dataB.a, dataC.a) != 0 || dataB.b != dataC.b || dataB.c != dataC.c || patchedBuffer->hash() != bufferB.hash())
        return false; // LCOV_EXCL_LINE

    // Success
    return true;
}