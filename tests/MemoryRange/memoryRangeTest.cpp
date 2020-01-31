#include "yatta.hpp"
#include <iostream>
#include <assert.h>


// Convenience Definitions
constexpr int SUCCESS = 0;
constexpr int FAILURE = 1;
using yatta::MemoryRange;

// Forward Declarations
void MemoryRange_ConstructionTest();
void MemoryRange_AssignmentTest();
void MemoryRange_MethodTest();
void MemoryRange_IOTest();
void MemoryRange_ExceptionTest();

int main()
{
    MemoryRange_ConstructionTest();
    MemoryRange_AssignmentTest();
    MemoryRange_MethodTest();
    MemoryRange_IOTest();
    MemoryRange_ExceptionTest();
    exit(SUCCESS);
}

void MemoryRange_ConstructionTest()
{
    // Ensure we can make empty memory ranges
    const MemoryRange memRange;
    assert(memRange.empty() && !memRange.hasData());

    // Ensure we can construct a memory range
    const auto largeBuffer = std::make_unique<std::byte[]>(1234ULL);
    MemoryRange largeMemRange(1234ULL, largeBuffer.get());
    assert(largeMemRange.hasData() && !largeMemRange.empty());

    // Ensure move constructor works
    MemoryRange moveMemRange = std::move(largeMemRange);
    assert(moveMemRange.size() == 1234ULL);

    // Ensure copy constructor works
    moveMemRange[0] = static_cast<std::byte>(255U);
    const MemoryRange copyMemRange(moveMemRange);
    assert(copyMemRange[0] == moveMemRange[0]);

    // Ensure pointers match
    assert(copyMemRange.bytes() == moveMemRange.bytes() && copyMemRange.bytes() == largeBuffer.get());
}

void MemoryRange_AssignmentTest()
{
    // Ensure ranges are equal
    const auto bufferB = std::make_unique<std::byte[]>(123456789ULL);
    MemoryRange rangeA;
    MemoryRange rangeB(123456789ULL, bufferB.get());
    rangeB[0] = static_cast<std::byte>(126U);
    rangeA = rangeB;
    assert((rangeA[0] == rangeB[0]) && (rangeA.bytes() == rangeB.bytes()));

    // Ensure rangeC is fully moved over to rangeA
    const auto bufferC = std::make_unique<std::byte[]>(456ULL);
    MemoryRange rangeC(456ULL, bufferC.get());
    rangeC[0] = static_cast<std::byte>(64U);
    rangeA = rangeC;
    assert(rangeA[0] == static_cast<std::byte>(64U));
}

void MemoryRange_MethodTest()
{
    // Ensure the memory range is reassignable
    MemoryRange memRange;
    assert(memRange.empty() && !memRange.hasData());

    // Ensure memory range has data
    const auto buffer = std::make_unique<std::byte[]>(1234ULL);
    memRange = MemoryRange(1234ULL, buffer.get());
    assert(memRange.hasData() && !memRange.empty());

    // Ensure memory range size is the same as the buffer
    assert(memRange.size() == 1234ULL);

    // Ensure we can hash the memory range
    assert(memRange.hash() != yatta::ZeroHash);

    // Ensure we can return a char array
    assert(memRange.charArray() != nullptr);

    // Ensure we can return a byte array
    assert(memRange.bytes() != nullptr);

    // Ensure both char array and byte array are the same underlying pointer
    assert(static_cast<const void*>(memRange.charArray()) == static_cast<const void*>(memRange.bytes()));

    // Ensure we can create a valid sub-range
    auto subRange = memRange.subrange(0, 617ULL);
    assert(!subRange.empty() && subRange.hasData());

    // Ensure we can iterate over the subrange
    int byteCount(0);
    for (auto& byte : subRange) {
        byte = static_cast<std::byte>(123U);
        ++byteCount;
    }
    assert(byteCount == 617);

    // Ensure we can iterate over the subrange with arbitrary types
    int iterationCount(0);
    for (auto* it = subRange.cbegin_t<size_t>(); it != subRange.cend_t<size_t>(); ++it)
        ++iterationCount;
    assert(iterationCount == static_cast<int>(617ULL / sizeof(size_t)));
}

void MemoryRange_IOTest()
{
    // Ensure object IO is correct
    auto buffer = std::make_unique<std::byte[]>(sizeof(int) + sizeof(std::byte));
    MemoryRange memRange(sizeof(int) + sizeof(std::byte), buffer.get());
    constexpr int in_int(64);
    constexpr std::byte in_byte(static_cast<std::byte>(123U));
    memRange.in_type(in_int);
    memRange.in_type(in_byte, sizeof(int));
    int out_int(0);
    std::byte out_byte(static_cast<std::byte>(0U));
    memRange.out_type(out_int);
    memRange.out_type(out_byte, sizeof(int));
    assert(in_int == out_int && in_byte == out_byte);

    // Ensure raw pointer IO is correct
    const char word[28] = "This is a sample sentence.\0";
    constexpr size_t sentenceSize = sizeof(char) * 28ULL;
    buffer = std::make_unique<std::byte[]>(sizeof(std::byte) + sentenceSize);
    memRange = MemoryRange(sizeof(std::byte) + sentenceSize, buffer.get());
    memRange.in_type(in_byte);
    memRange.in_raw(&word, sentenceSize, sizeof(std::byte));
    struct TestStructure {
        std::byte a;
        char c[28];
    } combinedOutput{};
    memRange.out_raw(&combinedOutput, sizeof(TestStructure));
    assert(combinedOutput.a == in_byte && std::string(combinedOutput.c) == word);

    // Test string template specializations
    buffer = std::make_unique<std::byte[]>(1234ULL);
    memRange = MemoryRange(1234ULL, buffer.get());
    const std::string string("Hello World");
    memRange.in_type(string);
    std::string outputString;
    memRange.out_type(outputString);
    assert(string == outputString);
}

void MemoryRange_ExceptionTest()
{
    bool exception[20] = { 0 };

    /////////////////////////////////
    /// index operator exceptions ///
    /////////////////////////////////

    // Ensure we can't access out of bounds
    try {
        MemoryRange emptyRange;
        emptyRange[0] = static_cast<std::byte>(123U);
    }
    catch (const std::exception&) {
        exception[0] = true;
    }
    assert(exception[0]);

    // Const version
    try {
        const MemoryRange emptyRange;
        assert(emptyRange[0] == emptyRange[0]);
    }
    catch (const std::exception&) {
        exception[1] = true;
    }
    assert(exception[1]);

    ///////////////////////////
    /// subrange exceptions ///
    ///////////////////////////

    // Catch null pointer exception
    try {
        const MemoryRange emptyRange;
        emptyRange.subrange(0, 0);
    }
    catch (std::exception&) {
        exception[2] = true;
    }
    assert(exception[2]);

    // Catch out of range exception
    try {
        const auto smallBuffer = std::make_unique<std::byte[]>(1ULL);
        const MemoryRange smallRange(1, smallBuffer.get());
        smallRange.subrange(0, 2);
    }
    catch (std::exception&) {
        exception[3] = true;
    }
    assert(exception[3]);

    /////////////////////////////////////
    /// in_raw and out_raw exceptions ///
    /////////////////////////////////////

    // Ensure we can't write out of bounds
    MemoryRange memRange;
    try {
        memRange[0] = static_cast<std::byte>(123U);
    }
    catch (const std::exception&) {
        exception[4] = true;
    }
    assert(exception[4]);

    // Ensure we can't read out of bounds
    try {
        const MemoryRange constRange;
        assert(constRange[0] == constRange[0]);
    }
    catch (const std::exception&) {
        exception[5] = true;
    }
    assert(exception[5]);

    // Ensure we can't write in raw memory to a null pointer
    try {
        memRange.in_raw(nullptr, 0);
    }
    catch (const std::exception&) {
        exception[6] = true;
    }
    assert(exception[6]);

    // Ensure we can't write out raw memory from a null pointer
    try {
        memRange.out_raw(nullptr, 0);
    }
    catch (const std::exception&) {
        exception[7] = true;
    }
    assert(exception[7]);

    // Ensure we can't write null data to a memory range
    std::byte byte;
    memRange = MemoryRange(1, &byte);
    try {
        memRange.in_raw(nullptr, 0);
    }
    catch (const std::exception&) {
        exception[8] = true;
    }
    assert(exception[8]);

    // Ensure we can't write out data to a null pointer
    try {
        memRange.out_raw(nullptr, 0);
    }
    catch (const std::exception&) {
        exception[9] = true;
    }
    assert(exception[9]);

    // Ensure we can't write in raw memory out of bounds
    try {
        memRange.in_raw(&byte, 8ULL);
    }
    catch (const std::exception&) {
        exception[10] = true;
    }
    assert(exception[10]);

    // Ensure we can't write out raw memory out of bounds
    try {
        std::byte outByte;
        memRange.out_raw(&outByte, 8ULL);
    }
    catch (const std::exception&) {
        exception[11] = true;
    }
    assert(exception[11]);

    ///////////////////////////////////////
    /// in_type and out_type exceptions ///
    ///      for ANY template type      ///
    ///////////////////////////////////////
    // Ensure we can't write onto a null pointer
    try {
        MemoryRange emptyRange;
        size_t obj;
        emptyRange.in_type(obj);
    }
    catch (const std::exception&) {
        exception[12] = true;
    }
    assert(exception[12]);

    // Ensure we can't read out from a null pointer
    try {
        MemoryRange emptyRange;
        size_t obj;
        emptyRange.out_type(obj);
    }
    catch (const std::exception&) {
        exception[13] = true;
    }
    assert(exception[13]);

    // Ensure we can't write out of bounds
    try {
        const auto smallBuffer = std::make_unique<std::byte[]>(1ULL);
        MemoryRange smallRange(1, smallBuffer.get());
        size_t obj;
        smallRange.in_type(obj);
    }
    catch (const std::exception&) {
        exception[14] = true;
    }
    assert(exception[14]);

    // Ensure we can't read out of bounds
    try {
        const auto smallBuffer = std::make_unique<std::byte[]>(1ULL);
        MemoryRange smallRange(1, smallBuffer.get());
        size_t obj;
        smallRange.out_type(obj);
    }
    catch (const std::exception&) {
        exception[15] = true;
    }
    assert(exception[15]);

    ///////////////////////////////////////
    /// in_type and out_type exceptions ///
    ///    for STRING specialization    ///
    ///////////////////////////////////////
    // Ensure we can't write onto a null pointer
    try {
        MemoryRange emptyRange;
        std::string string;
        emptyRange.in_type(string);
    }
    catch (const std::exception&) {
        exception[16] = true;
    }
    assert(exception[16]);

    // Ensure we can't read out from a null pointer
    try {
        MemoryRange emptyRange;
        std::string string;
        emptyRange.out_type(string);
    }
    catch (const std::exception&) {
        exception[17] = true;
    }
    assert(exception[17]);

    // Ensure we can't write out of bounds
    try {
        const auto smallBuffer = std::make_unique<std::byte[]>(1ULL);
        MemoryRange smallRange(1, smallBuffer.get());
        std::string string;
        smallRange.in_type(string);
    }
    catch (const std::exception&) {
        exception[18] = true;
    }
    assert(exception[18]);

    // Ensure we can't read out of bounds
    try {
        const auto smallBuffer = std::make_unique<std::byte[]>(1ULL);
        MemoryRange smallRange(1, smallBuffer.get());
        std::string string;
        smallRange.out_type(string);
    }
    catch (const std::exception&) {
        exception[19] = true;
    }
    assert(exception[19]);
}