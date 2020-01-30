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

int main()
{
    if (MemoryRange_ConstructionTest() &&
        MemoryRange_AssignmentTest() &&
        MemoryRange_MethodTest() &&
        MemoryRange_IOTest())
        return SUCCESS;

    std::cout << "Error while running Memory Range Tests\n";
    return FAILURE;
}

bool MemoryRange_ConstructionTest()
{
    // Ensure we can make empty memory ranges
    const MemoryRange memRange;
    if (!memRange.empty())
        return false;

    // Ensure we can make a large memory range
    const auto largeBuffer = std::make_unique<std::byte[]>(1234ULL);
    MemoryRange largeMemRange(1234ULL, largeBuffer.get());
    if (!largeMemRange.hasData())
        return false;

    // Ensure move constructor works
    MemoryRange moveMemRange(std::move(largeMemRange));
    if (moveMemRange.size() != 1234ULL)
        return false;

    // Ensure copy constructor works
    moveMemRange[0] = static_cast<std::byte>(255U);
    const MemoryRange& copyMemRange(moveMemRange);
    if (copyMemRange[0] != moveMemRange[0])
        return false;

    // Ensure pointers match
    if (copyMemRange.bytes() != moveMemRange.bytes() || copyMemRange.bytes() != largeBuffer.get())
        return false;

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
        return false;

    // Ensure rangeC is fully moved over to rangeA
    const auto bufferC = std::make_unique<std::byte[]>(456ULL);
    MemoryRange rangeC(456ULL, bufferC.get());
    rangeC[0] = static_cast<std::byte>(64U);
    rangeA = rangeC;
    if (rangeA[0] != static_cast<std::byte>(64U))
        return false;

    // Success
    return true;
}

bool MemoryRange_MethodTest()
{
    // Ensure the memory range is reassignable
    MemoryRange memRange;
    if (!memRange.empty())
        return false;

    // Ensure memory range has data
    const auto buffer = std::make_unique<std::byte[]>(1234ULL);
    memRange = MemoryRange(1234ULL, buffer.get());
    if (!memRange.hasData())
        return false;

    // Ensure memory range size is the same as the buffer
    if (memRange.size() != 1234ULL)
        return false;

    // Ensure we can hash the memory range
    if (const auto hash = memRange.hash(); hash == 0ULL || hash == yatta::ZeroHash)
        return false;

    // Ensure we can return a char array
    if (const char* const cArray = memRange.charArray(); cArray == nullptr)
        return false;

    // Ensure we can return a byte array
    if (const std::byte* const bytes = memRange.bytes(); bytes == nullptr)
        return false;

    // Ensure both char array and byte array are the same underlying pointer
    if (static_cast<const void*>(memRange.charArray()) != static_cast<const void*>(memRange.bytes()))
        return false;

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
        return false;

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
        return false;

    // Success
    return true;
}