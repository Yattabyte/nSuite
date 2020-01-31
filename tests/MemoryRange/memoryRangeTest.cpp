#include "yatta.hpp"
#include <iostream>


// Convenience Definitions
constexpr int SUCCESS = 0;
constexpr int FAILURE = 1;
using yatta::MemoryRange;

// Forward Declarations
bool MemoryRange_ConstructionTest();
bool MemoryRange_AssignmentTest();
bool MemoryRange_MethodTest();
bool MemoryRange_IOTest();
bool MemoryRange_ExceptionTest();

int main()
{
    if (!MemoryRange_ConstructionTest() ||
        !MemoryRange_AssignmentTest() ||
        !MemoryRange_MethodTest() ||
        !MemoryRange_IOTest() ||
        !MemoryRange_ExceptionTest())
        exit(FAILURE);
    exit(SUCCESS);
}

bool MemoryRange_ConstructionTest()
{
    // Ensure we can make empty memory ranges
    const MemoryRange memRange;
    if (!memRange.empty())
        exit(FAILURE);

    // Ensure we can make a large memory range
    const auto largeBuffer = std::make_unique<std::byte[]>(1234ULL);
    MemoryRange largeMemRange(1234ULL, largeBuffer.get());
    if (!largeMemRange.hasData())
        exit(FAILURE);

    // Ensure move constructor works
    MemoryRange moveMemRange(std::move(largeMemRange));
    if (moveMemRange.size() != 1234ULL)
        exit(FAILURE);

    // Ensure copy constructor works
    moveMemRange[0] = static_cast<std::byte>(255U);
    const MemoryRange& copyMemRange(moveMemRange);
    if (copyMemRange[0] != moveMemRange[0])
        exit(FAILURE);

    // Ensure pointers match
    if (copyMemRange.bytes() != moveMemRange.bytes() || copyMemRange.bytes() != largeBuffer.get())
        exit(FAILURE);

    // Success
    return true;
}

bool MemoryRange_AssignmentTest()
{
    const auto bufferA = std::make_unique<std::byte[]>(1234ULL);
    const auto bufferB = std::make_unique<std::byte[]>(123456789ULL);
    MemoryRange rangeA(1234ULL, bufferA.get());
    MemoryRange rangeB(123456789ULL, bufferB.get());
    rangeA[0] = static_cast<std::byte>(255U);
    rangeB[0] = static_cast<std::byte>(126U);

    // Ensure ranges are equal
    rangeA = rangeB;
    if ((rangeA[0] != rangeB[0]) || (rangeA.bytes() != rangeB.bytes()))
        exit(FAILURE);

    // Ensure rangeC is fully moved over to rangeA
    const auto bufferC = std::make_unique<std::byte[]>(456ULL);
    MemoryRange rangeC(456ULL, bufferC.get());
    rangeC[0] = static_cast<std::byte>(64U);
    rangeA = rangeC;
    if (rangeA[0] != static_cast<std::byte>(64U))
        exit(FAILURE);

    // Success
    return true;
}

bool MemoryRange_MethodTest()
{
    // Ensure the memory range is reassignable
    MemoryRange memRange;
    if (!memRange.empty())
        exit(FAILURE);

    // Ensure memory range has data
    const auto buffer = std::make_unique<std::byte[]>(1234ULL);
    memRange = MemoryRange(1234ULL, buffer.get());
    if (!memRange.hasData())
        exit(FAILURE);

    // Ensure memory range size is the same as the buffer
    if (memRange.size() != 1234ULL)
        exit(FAILURE);

    // Ensure we can hash the memory range
    if (const auto hash = memRange.hash(); hash == 0ULL || hash == yatta::ZeroHash)
        exit(FAILURE);

    // Ensure we can return a char array
    if (const char* const cArray = memRange.charArray(); cArray == nullptr)
        exit(FAILURE);

    // Ensure we can return a byte array
    if (const std::byte* const bytes = memRange.bytes(); bytes == nullptr)
        exit(FAILURE);

    // Ensure both char array and byte array are the same underlying pointer
    if (static_cast<const void*>(memRange.charArray()) != static_cast<const void*>(memRange.bytes()))
        exit(FAILURE);

    // Ensure we can create a valid sub-range
    auto subRange = memRange.subrange(0, 617ULL);
    if (subRange.empty() || !subRange.hasData())
        exit(FAILURE);

    // Ensure we can iterate over the subrange
    int byteCount(0);
    for (auto& byte : subRange) {
        byte = static_cast<std::byte>(123U);
        ++byteCount;
    }
    if (byteCount != 617)
        exit(FAILURE);

    // Ensure we can iterate over the subrange with arbitrary types
    constexpr int expectedCount = static_cast<int>(617ULL / sizeof(size_t));
    int iterationCount(0);
    for (auto* it = subRange.cbegin_t<size_t>(); it != subRange.cend_t<size_t>(); ++it)
        ++iterationCount;
    if (iterationCount != expectedCount)
        exit(FAILURE);

    // Success
    return true;
}

bool MemoryRange_IOTest()
{
    // Ensure object IO is correct
    auto buffer = std::make_unique<std::byte[]>(sizeof(int));
    MemoryRange memRange(sizeof(int), buffer.get());
    constexpr int in_int(64);
    memRange.in_type(in_int);
    int out_int(0);
    memRange.out_type(out_int);
    if (in_int != out_int)
        exit(FAILURE);

    // Ensure raw pointer IO is correct
    const char word[28] = "This is a sample sentence.\0";
    constexpr size_t sentenceSize = sizeof(char) * 28ULL;
    buffer = std::make_unique<std::byte[]>(sizeof(int) + sentenceSize);
    memRange = MemoryRange(sizeof(int) + sentenceSize, buffer.get());
    memRange.in_type(in_int);
    memRange.in_raw(&word, sentenceSize, sizeof(int));
    struct TestStructure {
        int a;
        char b[28];
    } combinedOutput{};
    memRange.out_raw(&combinedOutput, sizeof(TestStructure));
    if (combinedOutput.a != in_int || std::string(combinedOutput.b) != word)
        exit(FAILURE);

    // Test string template specializations
    buffer = std::make_unique<std::byte[]>(1234ULL);
    memRange = MemoryRange(1234ULL, buffer.get());
    const std::string string("Hello World");
    memRange.in_type(string);
    std::string outputString;
    memRange.out_type(outputString);
    if (string != outputString)
        exit(FAILURE);

    // Success
    return true;
}

bool MemoryRange_ExceptionTest()
{
    /////////////////////////////////
    /// index operator exceptions ///
    /////////////////////////////////

    // Ensure we can't access out of bounds
    try {
        MemoryRange emptyRange;
        emptyRange[0] = static_cast<std::byte>(123U);
        exit(FAILURE);
    }
    catch (const std::exception&) {}
    // Const version
    try {
        const MemoryRange emptyRange;
        if (emptyRange[0] != emptyRange[0])
            exit(FAILURE);
        exit(FAILURE);
    }
    catch (const std::exception&) {}

    ///////////////////////////
    /// subrange exceptions ///
    ///////////////////////////

    // Ensure we can't create an invalid subrange
    try {
        // Catch null pointer exception
        const MemoryRange emptyRange;
        const auto subRange = emptyRange.subrange(0, 0);
        if (subRange.hasData())
            exit(FAILURE);
        exit(FAILURE);
    }
    catch (std::exception&) {}
    try {
        // Catch out of range exception
        const auto smallBuffer = std::make_unique<std::byte[]>(1ULL);
        const MemoryRange smallRange(1, smallBuffer.get());
        const auto subRange = smallRange.subrange(0, 2);
        if (subRange.hasData())
            exit(FAILURE);
        exit(FAILURE);
    }
    catch (std::exception&) {}

    /////////////////////////////////////
    /// in_raw and out_raw exceptions ///
    /////////////////////////////////////

    // Ensure we can't write out of bounds
    MemoryRange memRange;
    try {
        memRange[0] = static_cast<std::byte>(123U);
        exit(FAILURE);
    }
    catch (const std::exception&) {}

    // Ensure we can't read out of bounds
    try {
        const MemoryRange constRange;
        if (constRange[0] != constRange[0])
            exit(FAILURE);
        exit(FAILURE);
    }
    catch (const std::exception&) {}

    // Ensure we can't write in raw memory to a null pointer
    try {
        memRange.in_raw(nullptr, 0);
        exit(FAILURE);
    }
    catch (const std::exception&) {}

    // Ensure we can't write out raw memory from a null pointer
    try {
        memRange.out_raw(nullptr, 0);
        exit(FAILURE);
    }
    catch (const std::exception&) {}

    // Ensure we can't write null data to a memory range
    std::byte byte;
    memRange = MemoryRange(1, &byte);
    try {
        memRange.in_raw(nullptr, 0);
        exit(FAILURE);
    }
    catch (const std::exception&) {}

    // Ensure we can't write out data to a null pointer
    try {
        memRange.out_raw(nullptr, 0);
        exit(FAILURE);
    }
    catch (const std::exception&) {}

    // Ensure we can't write in raw memory out of bounds
    try {
        memRange.in_raw(&byte, 8ULL);
        exit(FAILURE);
    }
    catch (const std::exception&) {}

    // Ensure we can't write out raw memory out of bounds
    try {
        std::byte outByte;
        memRange.out_raw(&outByte, 8ULL);
        exit(FAILURE);
    }
    catch (const std::exception&) {}

    ///////////////////////////////////////
    /// in_type and out_type exceptions ///
    ///      for ANY template type      ///
    ///////////////////////////////////////
    // Ensure we can't write onto a null pointer
    try {
        MemoryRange emptyRange;
        size_t obj;
        emptyRange.in_type(obj);
        exit(FAILURE);
    }
    catch (const std::exception&) {}
    // Ensure we can't read out from a null pointer
    try {
        MemoryRange emptyRange;
        size_t obj;
        emptyRange.out_type(obj);
        exit(FAILURE);
    }
    catch (const std::exception&) {}
    // Ensure we can't write out of bounds
    try {
        const auto smallBuffer = std::make_unique<std::byte[]>(1ULL);
        MemoryRange smallRange(1, smallBuffer.get());
        size_t obj;
        smallRange.in_type(obj);
        exit(FAILURE);
    }
    catch (const std::exception&) {}
    // Ensure we can't read out of bounds
    try {
        const auto smallBuffer = std::make_unique<std::byte[]>(1ULL);
        MemoryRange smallRange(1, smallBuffer.get());
        size_t obj;
        smallRange.out_type(obj);
        exit(FAILURE);
    }
    catch (const std::exception&) {}

    ///////////////////////////////////////
    /// in_type and out_type exceptions ///
    ///    for STRING specialization    ///
    ///////////////////////////////////////
    // Ensure we can't write onto a null pointer
    try {
        MemoryRange emptyRange;
        std::string string;
        emptyRange.in_type(string);
        exit(FAILURE);
    }
    catch (const std::exception&) {}
    // Ensure we can't read out from a null pointer
    try {
        MemoryRange emptyRange;
        std::string string;
        emptyRange.out_type(string);
        exit(FAILURE);
    }
    catch (const std::exception&) {}
    // Ensure we can't write out of bounds
    try {
        const auto smallBuffer = std::make_unique<std::byte[]>(1ULL);
        MemoryRange smallRange(1, smallBuffer.get());
        std::string string;
        smallRange.in_type(string);
        exit(FAILURE);
    }
    catch (const std::exception&) {}
    // Ensure we can't read out of bounds
    try {
        const auto smallBuffer = std::make_unique<std::byte[]>(1ULL);
        MemoryRange smallRange(1, smallBuffer.get());
        std::string string;
        smallRange.out_type(string);
        exit(FAILURE);
    }
    catch (const std::exception&) {}

    // Success
    return true;
}