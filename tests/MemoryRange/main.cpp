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

int main() noexcept
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
    if (memRange.empty()) {
        // Ensure we can make a large memory range
        const auto largeBuffer = std::make_unique<std::byte[]>(1234ULL);
        MemoryRange largeMemRange(1234ULL, largeBuffer.get());
        if (largeMemRange.hasData()) {
            // Ensure move constructor works
            MemoryRange moveMemRange(std::move(largeMemRange));
            if (moveMemRange.size() == 1234ULL) {
                // Ensure copy constructor works
                moveMemRange[0] = static_cast<std::byte>(255U);
                const MemoryRange& copyMemRange(moveMemRange);
                if (copyMemRange[0] == moveMemRange[0]) {
                    // Ensure pointers match
                    if (&copyMemRange[0] == &largeBuffer[0]) {
                        return true; // Success
                    }                    
                }
            }
        }
    }

    return false; // Failure
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
    if (rangeA[0] == rangeB[0]) {
        const auto bufferC = std::make_unique<std::byte[]>(456ULL);
        MemoryRange rangeC(456ULL, bufferC.get());
        rangeC[0] = static_cast<std::byte>(64U);
        rangeA = rangeC;
        // Ensure rangeC is fully moved over to rangeA
        if (rangeA[0] == static_cast<std::byte>(64U)) {
            return true; // Success
        }
    }

    return false; // Failure
}

bool MemoryRange_MethodTest()
{
    MemoryRange memRange;
    // Ensure the memory range is reassignable
    if (memRange.empty()) {
        const auto buffer = std::make_unique<std::byte[]>(1234ULL);
        memRange = MemoryRange(1234ULL, buffer.get());
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
                                return true; // Success
                            }
                        }
                    }
                }
            }
        }
    }

    return false; // Failure
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
    if (in_int == out_int) {
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

        if (combinedOutput.a == in_int && std::string(combinedOutput.b) == word) {
            return true; // Success
        }
    }

    return false; // Failure
}