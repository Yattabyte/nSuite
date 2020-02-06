#include "yatta.hpp"
#include <cassert>
#include <iostream>

// Convenience Definitions
using yatta::Buffer;

// Forward Declarations
void Buffer_ConstructionTest();
void Buffer_AssignmentTest();
void Buffer_MethodTest();
void Buffer_IOTest();
void Buffer_CompressionTest();
void Buffer_DiffTest();

// The structure we'll compress, decompress, diff, and patch
struct TestStructureA {
    int a = 0;
    float b = 0.F;
    char c[256] = { '\0' };
    bool operator==(const TestStructureA& other) const noexcept {
        return a == other.a && b == other.b && std::strcmp(c, other.c) == 0;
    }
};
struct TestStructureB {
    char a[128] = { '\0' };
    int b = 0;
    char c[128] = { '\0' };
    float d = 0.F;
    bool operator==(const TestStructureB& other) const noexcept {
        return std::strcmp(a, other.a) == 0 && b == other.b &&
               std::strcmp(c, other.c) == 0 && d == other.d;
    }
};

int main() {
    Buffer_ConstructionTest();
    Buffer_AssignmentTest();
    Buffer_MethodTest();
    Buffer_IOTest();
    Buffer_CompressionTest();
    Buffer_DiffTest();
    exit(0);
}

void Buffer_ConstructionTest() {
    // Ensure we can make empty buffers
    Buffer buffer;
    assert(buffer.empty() && !buffer.hasData());

    // Ensure we can construct a buffer
    Buffer largeBuffer(1234ULL);
    assert(largeBuffer.hasData() && !largeBuffer.empty());

    // Ensure move constructor works
    Buffer moveBuffer(Buffer(1234ULL));
    assert(moveBuffer.size() == 1234ULL);

    // Ensure copy constructor works
    largeBuffer[0] = static_cast<std::byte>(255U);
    const Buffer copyBuffer(largeBuffer);
    assert(copyBuffer[0] == largeBuffer[0]);
}

void Buffer_AssignmentTest() {
    // Ensure buffers are equal
    Buffer bufferA;
    Buffer bufferB(123456789ULL);
    bufferB[0] = static_cast<std::byte>(126U);
    bufferA = bufferB;
    assert(bufferA[0] == bufferB[0]);

    // Ensure bufferC is fully moved over to bufferA
    Buffer bufferC(456ULL);
    bufferC[0] = static_cast<std::byte>(64U);
    bufferA = std::move(bufferC);
    assert(bufferA[0] == static_cast<std::byte>(64U));
}

void Buffer_MethodTest() {
    // Ensure the buffer is empty
    Buffer buffer;
    assert(buffer.empty());

    // Ensure we can resize the buffer with data
    buffer.resize(1234ULL);
    assert(buffer.hasData());

    // Ensure the size is 1234
    assert(buffer.size() == 1234ULL);

    // Ensure the capacity is doubled
    assert(buffer.capacity() == 2468ULL);

    // Ensure we can explicitly set the capacity
    buffer.reserve(3000ULL);
    assert(buffer.capacity() == 3000ULL);

    // Ensure we can shrink the buffer
    buffer.shrink();
    assert(buffer.size() == 1234ULL && buffer.capacity() == 1234ULL);

    // Ensure we can completely free the buffer
    buffer.clear();
    buffer.shrink();
    assert(buffer.empty() && !buffer.hasData());
}

void Buffer_IOTest() {
    // Ensure we can push data
    Buffer buffer;
    constexpr size_t data1(123ULL);
    constexpr char data2('Z');
    constexpr bool data3(true);
    constexpr int data4[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
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
    assert(
        data1 == data1_out && data2 == data2_out && data3 == data3_out &&
        std::equal(
            std::cbegin(data4), std::cend(data4), std::cbegin(data4_out)) &&
        data5 == data5_out && std::string(data6) == std::string(data6_out));
}

void Buffer_CompressionTest() {
    // Ensure we cannot compress or decompress an empty or incorrect buffer
    Buffer buffer;
    const auto badResult1 = buffer.compress();
    const auto badResult2 = badResult1->decompress();
    assert(!badResult1 && !badResult2);
    // Create a buffer and load it with test data
    buffer.resize(sizeof(TestStructureA));
    constexpr TestStructureA testData{
        1234, 567.890F,
        "QWEQWEQWEEQWEQWEQWEQWEEEEEEEQWEQWEQWEQWEQQWEQWEQWEEEEEE623785623465283"
        "7444444443333333333333364637896463QWQWEQWEQWWEQWEQWEEEEEEEQWEQWEQWEEQW"
        "EQWEQWEQWNOUNOUNOUNOUNOUNOUNOU4EQWEQWEEEEEE623785623465283744444444333"
        "3333333333364637896463QWQWEQWEQWWEQWEQWEEEEE\0"
    };
    buffer.in_type(testData);

    // Attempt to compress the buffer
    const auto compressedBuffer = buffer.compress();
    assert(compressedBuffer.has_value() && !compressedBuffer->empty());

    // Attempt to decompress the compressed buffer
    const auto decompressedBuffer = compressedBuffer->decompress();
    assert(decompressedBuffer.has_value() && !decompressedBuffer->empty());

    // Dump buffer data back into test structure
    TestStructureA decompressedData;
    decompressedBuffer->out_type(decompressedData);

    // Ensure data matches
    assert(
        testData == decompressedData &&
        decompressedBuffer->hash() == buffer.hash());
}

void Buffer_DiffTest() {
    // Ensure we cannot diff or patch an empty or incorrect buffer
    Buffer bufferA;
    Buffer bufferB;
    const auto badResult1 = bufferA.diff(bufferB);
    const auto badResult2 = badResult1->patch(bufferB);
    assert(!badResult1 && !badResult2);

    constexpr TestStructureA dataA{
        1234, 567.890F,
        "This is an example of a long string within "
        "the structure named TestStructureA dataA."
        "9999999999999999999999999999999999999999999999"
        "999999999999999999999999999999999999999999999\0"
    };
    constexpr TestStructureB dataB{
        "This is an example of a long string within "
        "the structure named TestStructureB dataB.",
        4321,
        "8888888888888888888888888888888888888888888888"
        "888888888888888888888888888888888888888888888\0",
        3.14159265359F
    };
    bufferA.resize(sizeof(TestStructureA));
    bufferB.resize(sizeof(TestStructureB));
    bufferA.in_type(dataA);
    bufferB.in_type(dataB);

    // Ensure we've generated an instruction set
    const auto diffBuffer = bufferA.diff(bufferB);
    assert(diffBuffer.has_value());

    // Check that we've actually converted from A to B
    const auto patchedBuffer = bufferA.patch(*diffBuffer);
    assert(patchedBuffer.has_value());

    // Dump test structure back, confirm it matches
    // Recall bufferA had dataA pushed in, patched with diff buffer
    TestStructureB dataC;
    patchedBuffer->out_type(dataC);
    assert(dataB == dataC && patchedBuffer->hash() == bufferB.hash());
}