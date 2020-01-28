#include "buffer.hpp"
#include "threader.hpp"
#include "lz4/lz4.h"
#include <algorithm>
#include <climits>
#include <cstring>
#include <numeric>
#include <vector>


// Convenience definitions
using yatta::Buffer;
using yatta::MemoryRange;
using yatta::Threader;

/** Data structures for buffer compression headers. */
struct CompressionHeader {
    char m_title[16ULL] = { '\0' };
    size_t m_uncompressedSize = 0ULL;
};
/** Data structures for buffer differential headers. */
struct DifferentialHeader {
    char m_title[16ULL] = { '\0' };
    size_t m_targetSize = 0ULL;
};
/** Super-class for buffer diff instructions. */
struct Differential_Instruction {
    // (de)Constructors
    virtual ~Differential_Instruction() = default;
    explicit Differential_Instruction(const char& t) noexcept
        : m_type(t) {}
    Differential_Instruction(const Differential_Instruction& other) = delete;
    Differential_Instruction(Differential_Instruction&& other) = delete;
    Differential_Instruction& operator=(const Differential_Instruction& other) = delete;
    Differential_Instruction& operator=(Differential_Instruction&& other) = delete;


    // Interface Declaration
    /** Retrieve the byte-size of this instruction. */
    [[nodiscard]] virtual size_t size() const noexcept = 0;
    /** Execute this instruction. */
    virtual void execute(Buffer& bufferNew, const MemoryRange& bufferOld) const = 0;
    /** Write-out this instruction to a buffer. */
    virtual void write(Buffer& outputBuffer, size_t& byteIndex) const = 0;
    /** Read-in this instruction from a buffer. */
    virtual void read(const Buffer& inputBuffer, size_t& byteIndex) = 0;


    // Attributes
    const char m_type = 0;
    size_t m_index = 0ULL;
};
/** Specifies a region in the 'old' file to read from, and where to put it in the 'new' file. */
struct Copy_Instruction final : public Differential_Instruction {
    // Constructor
    Copy_Instruction() noexcept : Differential_Instruction('C') {}


    // Interface Implementation
    [[nodiscard]] size_t size() const noexcept final {
        return static_cast<size_t>(sizeof(char) + (sizeof(size_t) * 3ULL));
    }
    void execute(Buffer& bufferNew, const MemoryRange& bufferOld) const final {
        const auto old_subRange = bufferOld.subrange(m_beginRead, m_endRead - m_beginRead);
        std::copy(old_subRange.cbegin(), old_subRange.cend(), &bufferNew[m_index]);
    }
    void write(Buffer& outputBuffer, size_t& byteIndex) const final {
        // Write Type
        outputBuffer.in_type(m_type, byteIndex);
        byteIndex += static_cast<size_t>(sizeof(char));
        // Write Index
        outputBuffer.in_type(m_index, byteIndex);
        byteIndex += static_cast<size_t>(sizeof(size_t));
        // Write Begin
        outputBuffer.in_type(m_beginRead, byteIndex);
        byteIndex += static_cast<size_t>(sizeof(size_t));
        // Write End
        outputBuffer.in_type(m_endRead, byteIndex);
        byteIndex += static_cast<size_t>(sizeof(size_t));
    }
    void read(const Buffer& inputBuffer, size_t& byteIndex) final {
        // Type already read
        // Read Index
        inputBuffer.out_type(m_index, byteIndex);
        byteIndex += static_cast<size_t>(sizeof(size_t));
        // Read Begin
        inputBuffer.out_type(m_beginRead, byteIndex);
        byteIndex += static_cast<size_t>(sizeof(size_t));
        // Read End
        inputBuffer.out_type(m_endRead, byteIndex);
        byteIndex += static_cast<size_t>(sizeof(size_t));
    }


    // Public Attributes
    size_t m_beginRead = 0ULL, m_endRead = 0ULL;
};
/** Contains a block of data to insert into the 'new' file, at a given point. */
struct Insert_Instruction final : public Differential_Instruction {
    // Constructor
    Insert_Instruction() noexcept : Differential_Instruction('I') {}


    // Interface Implementation
    [[nodiscard]] size_t size() const noexcept final {
        return static_cast<size_t>(
            sizeof(char) +
            (sizeof(size_t) * 2ULL) +
            (sizeof(char) * m_newData.size())
            );
    }
    void execute(Buffer& bufferNew, const MemoryRange& /*unused*/) const final {
        std::copy(m_newData.cbegin(), m_newData.cend(), &bufferNew[m_index]);
    }
    void write(Buffer& outputBuffer, size_t& byteIndex) const final {
        // Write Type
        outputBuffer.in_type(m_type, byteIndex);
        byteIndex += static_cast<size_t>(sizeof(char));
        // Write Index
        outputBuffer.in_type(m_index, byteIndex);
        byteIndex += static_cast<size_t>(sizeof(size_t));
        // Write Length
        auto length = m_newData.size();
        outputBuffer.in_type(length, byteIndex);
        byteIndex += static_cast<size_t>(sizeof(size_t));
        if (length != 0U) {
            // Write Data
            outputBuffer.in_raw(m_newData.data(), length, byteIndex);
            byteIndex += static_cast<size_t>(sizeof(char))* length;
        }
    }
    void read(const Buffer& inputBuffer, size_t& byteIndex) final {
        // Type already read
        // Read Index
        inputBuffer.out_type(m_index, byteIndex);
        byteIndex += static_cast<size_t>(sizeof(size_t));
        // Read Length
        size_t length(0ULL);
        inputBuffer.out_type(length, byteIndex);
        byteIndex += static_cast<size_t>(sizeof(size_t));
        if (length != 0ULL) {
            // Read Data
            m_newData.resize(length);
            inputBuffer.out_raw(m_newData.data(), length, byteIndex);
            byteIndex += static_cast<size_t>(sizeof(char))* length;
        }
    }


    // Attributes
    std::vector<std::byte> m_newData;
};
/** Contains a single value a to insert into the 'new' file, at a given point, repeating multiple times. */
struct Repeat_Instruction final : public Differential_Instruction {
    // Constructor
    Repeat_Instruction() noexcept : Differential_Instruction('R') {}


    // Interface Implementation
    [[nodiscard]] size_t size() const noexcept final {
        return static_cast<size_t>(sizeof(char) + (sizeof(size_t) * 2ULL) + sizeof(char));
    }
    void execute(Buffer& bufferNew, const MemoryRange& /*unused*/) const final {
        std::fill(&bufferNew[0], &bufferNew[std::min(m_amount, bufferNew.size())], m_value);
    }
    void write(Buffer& outputBuffer, size_t& byteIndex) const final {
        // Write Type
        outputBuffer.in_type(m_type, byteIndex);
        byteIndex += static_cast<size_t>(sizeof(char));
        // Write Index
        outputBuffer.in_type(m_index, byteIndex);
        byteIndex += static_cast<size_t>(sizeof(size_t));
        // Write Amount
        outputBuffer.in_type(m_amount, byteIndex);
        byteIndex += static_cast<size_t>(sizeof(size_t));
        // Write Value
        outputBuffer.in_type(m_value, byteIndex);
        byteIndex += static_cast<size_t>(sizeof(char));
    }
    void read(const Buffer& inputBuffer, size_t& byteIndex) final {
        // Type already read
        // Read Index
        inputBuffer.out_type(m_index, byteIndex);
        byteIndex += static_cast<size_t>(sizeof(size_t));
        // Read Amount
        inputBuffer.out_type(m_amount, byteIndex);
        byteIndex += static_cast<size_t>(sizeof(size_t));
        // Read Value
        inputBuffer.out_type(m_value, byteIndex);
        byteIndex += static_cast<size_t>(sizeof(char));
    }


    // Attributes
    size_t m_amount = 0ULL;
    std::byte m_value = static_cast<std::byte>(0);
};


// Public (de)Constructors

Buffer::Buffer(const size_t& size) :
    MemoryRange(size, nullptr),
    m_capacity(size * 2ULL),
    m_data(std::make_unique<std::byte[]>(m_capacity))
{
    m_dataPtr = m_data.get();
}

Buffer::Buffer(const Buffer& other) :
    MemoryRange(other),
    m_capacity(other.m_capacity),
    m_data(std::make_unique<std::byte[]>(other.m_capacity))
{
    m_dataPtr = m_data.get();
    std::copy(other.m_data.get(), other.m_data.get() + other.m_range, m_data.get());
}

Buffer::Buffer(Buffer&& other) noexcept :
    MemoryRange(other),
    m_capacity(other.m_capacity),
    m_data(std::move(other.m_data))
{
    other.m_capacity = 0ULL;
    other.m_data = nullptr;
}


// Public Assignment Operators

Buffer& Buffer::operator=(const Buffer& other)
{
    if (this != &other) {
        m_range = other.m_range;
        m_capacity = other.m_capacity;
        m_data = std::make_unique<std::byte[]>(other.m_capacity);
        m_dataPtr = m_data.get();
        std::copy(other.m_data.get(), other.m_data.get() + other.m_range, m_data.get());
    }
    return *this;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept
{
    if (this != &other) {
        m_range = other.m_range;
        m_capacity = other.m_capacity;
        m_data = std::move(other.m_data);
        m_dataPtr = m_data.get();

        other.m_range = 0ULL;
        other.m_capacity = 0ULL;
        other.m_data = nullptr;
        other.m_dataPtr = nullptr;
    }
    return *this;
}


// Public Inquiry Methods

bool Buffer::empty() const noexcept
{
    return m_data == nullptr || m_range == 0ULL || m_capacity == 0ULL;
}

size_t Buffer::capacity() const noexcept
{
    return m_capacity;
}


// Public Manipulation Methods

void Buffer::resize(const size_t& size)
{
    // Create the data container if it is missing
    if (m_data == nullptr) {
        m_capacity = size * 2ULL;
        m_data = std::make_unique<std::byte[]>(size * 2ULL);
        m_dataPtr = m_data.get();
    }

    // Check if our previous buffer is too small
    else if (size > m_capacity) {
        // Allocate new container
        auto newData = std::make_unique<std::byte[]>(size * 2ULL);

        // Copy over old data
        std::copy(m_data.get(), m_data.get() + m_range, newData.get());

        // Swap data containers
        m_capacity = size * 2ULL;
        std::swap(m_data, newData);
        m_dataPtr = m_data.get();
    }

    m_range = size;
}

void Buffer::shrink()
{
    // Ensure there is data to shrink
    if (m_data == nullptr)
        return;

    // Allocate new container
    auto newData = std::make_unique<std::byte[]>(m_range);

    // Copy over old data
    std::copy(m_data.get(), m_data.get() + m_range, newData.get());

    // Swap data containers
    m_capacity = m_range;
    std::swap(m_data, newData);
    m_dataPtr = m_data.get();
}

void Buffer::clear() noexcept
{
    m_data.release();
    m_range = 0ULL;
    m_capacity = 0ULL;
    m_data = nullptr;
    m_dataPtr = nullptr;
}

// Public Derivation Methods

std::optional<Buffer> Buffer::compress() const
{
    return Buffer::compress(*this);
}

std::optional<Buffer> Buffer::compress(const Buffer& buffer)
{    
    return Buffer::compress(Buffer::MemoryRange(buffer));
}

std::optional<Buffer> Buffer::compress(const MemoryRange& memoryRange)
{
    // Ensure this buffer has some data to compress
    if (memoryRange.empty())
        return {}; // Failure

    // Create a larger buffer twice the size, plus a unique header
    const auto sourceSize = memoryRange.size();
    const auto destinationSize = sourceSize * 2ULL;
    constexpr auto headerSize = sizeof(CompressionHeader);
    Buffer compressedBuffer(headerSize + destinationSize);
    CompressionHeader compressionHeader{ "yatta compress", sourceSize };

    // Copy header data into new buffer at the beginning
    compressedBuffer.in_type(compressionHeader);

    // Try to compress the source buffer
    const auto compressedSize = LZ4_compress_default(
        memoryRange.charArray(),
        &compressedBuffer.charArray()[headerSize], // Offset by header's amount
        static_cast<int>(sourceSize),
        static_cast<int>(destinationSize)
    );

    // Ensure we have a non-zero sized buffer
    if (compressedSize == 0ULL)
        return {}; // Failure

    // We now know the actual compressed size, downsize our oversized buffer to the compressed size
    compressedBuffer.resize(headerSize + compressedSize);
    compressedBuffer.shrink();

    // Success
    return compressedBuffer;
}

std::optional<Buffer> Buffer::decompress() const
{
    return Buffer::decompress(*this);
}

std::optional<Buffer> Buffer::decompress(const Buffer& buffer)
{
    return Buffer::decompress(Buffer::MemoryRange(buffer));
}

std::optional<Buffer> Buffer::decompress(const MemoryRange& memoryRange)
{
    // Ensure this buffer has some data to decompress
    constexpr auto headerSize = sizeof(CompressionHeader);
    if (memoryRange.size() < headerSize)
        return {}; // Failure

    // Read in header
    CompressionHeader header;
    memoryRange.out_type(header);

    // Ensure header title matches
    if (std::strcmp(header.m_title, "yatta compress") != 0)
        return {}; // Failure

    // Uncompress the remaining data
    Buffer uncompressedBuffer(header.m_uncompressedSize);
    const auto decompressionResult = LZ4_decompress_safe(
        &memoryRange.charArray()[headerSize],
        uncompressedBuffer.charArray(),
        static_cast<int>(memoryRange.size() - headerSize),
        static_cast<int>(uncompressedBuffer.size())
    );

    // Ensure we have a non-zero sized decompressed buffer
    if (decompressionResult <= 0)
        return {}; // Failure

    // Success
    return uncompressedBuffer;
}

std::optional<Buffer> Buffer::diff(const Buffer& target) const
{
    return Buffer::diff(*this, target);
}

std::optional<Buffer> Buffer::diff(const Buffer& source, const Buffer& target)
{
    return Buffer::diff(Buffer::MemoryRange(source), Buffer::MemoryRange(target));
}

struct MatchInfo {
    size_t length = 0ULL, start1 = 0ULL, start2 = 0ULL;
};
auto find_matching_regions(const MemoryRange& rangeA, const MemoryRange& rangeB)
{
    std::vector<MatchInfo> bestMatch;
    size_t largestMatch(0ULL);

    // Algorithm Overview
    // For each element in Range A, compare from element index -> end
    // against entire Range B
    std::for_each(
        rangeA.cbegin_t<size_t>(),
        rangeA.cend_t<size_t>(),
        [&](const size_t& elementA)
        {
            size_t matchCount(0ULL);
            size_t sumMatches(0ULL);
            std::vector<MatchInfo> matches;
            const size_t index_8byte = &elementA - reinterpret_cast<const size_t*>(rangeA.cbegin());
            const size_t index_byte = index_8byte * sizeof(size_t);
            const auto subRangeA = rangeA.subrange(index_byte, rangeA.size() - index_byte);
            const auto length = std::min<size_t>(subRangeA.size(), rangeB.size());

            // Iterate through {subRangeA and rangeB} 8 bytes at a time
            // Take note of matches >= 32 bytes in a row
            for (size_t ind = 0ULL; ind < length; ind += 8ULL) {
                const auto& a = *reinterpret_cast<const size_t*>(&subRangeA[ind]);
                const auto& b = *reinterpret_cast<const size_t*>(&rangeB[ind]);
                if (a == b)
                    ++matchCount;
                else {
                    if (matchCount >= 4ULL) {
                        const auto matchLength = matchCount * sizeof(size_t);
                        matches.push_back(
                            {
                                matchLength,
                                ind + index_byte - matchLength,
                                ind - matchLength
                            }
                        );
                        sumMatches += matchCount;
                    }
                    matchCount = 0ULL;
                }
            }

            if (sumMatches > largestMatch) {
                largestMatch = sumMatches;
                bestMatch = matches;
            }
        }
    );
    return bestMatch;
}

struct WindowInfo { size_t windowSize = 0ULL, indexA = 0ULL, indexB = 0ULL; };
auto split_and_match_ranges(const MemoryRange& rangeA, const MemoryRange& rangeB, size_t& indexA, size_t& indexB)
{
    Threader threader;
    const auto sizeA = rangeA.size();
    const auto sizeB = rangeB.size();
    std::mutex matchMutex;
    std::vector<std::pair<WindowInfo, std::vector<MatchInfo>>> matchingRegions;

    while (indexA < sizeA && indexB < sizeB) {
        const auto windowSize = std::min<size_t>(4096ULL, std::min<size_t>(sizeA - indexA, sizeB - indexB));

        threader.addJob(
            [&, windowSize, indexA, indexB]()
            {
                const auto windowA = rangeA.subrange(indexA, windowSize);
                const auto windowB = rangeB.subrange(indexB, windowSize);
                auto matches = find_matching_regions(windowA, windowB);
                for (auto& matchInfo : matches) {
                    matchInfo.start1 += indexA;
                    matchInfo.start2 += indexB;
                }

                std::unique_lock<std::mutex> writeGuard(matchMutex);
                matchingRegions.emplace_back(
                    WindowInfo{ windowSize, indexA, indexB }, matches);
            }
        );

        // increment
        indexA += windowSize;
        indexB += windowSize;
    }

    // Wait for jobs to finish
    while (!threader.isFinished())
        continue;

    return matchingRegions;
}

std::optional<Buffer> Buffer::diff(const MemoryRange& sourceMemory, const MemoryRange& targetMemory)
{
    // Ensure that at least ONE of the two source buffers exists
    if (sourceMemory.empty() && targetMemory.empty())
        return {}; // Failure

    // Convert matching regions into diff instructions
    size_t indexA(0ULL);
    size_t indexB(0ULL);
    const auto matchingRegions = split_and_match_ranges(sourceMemory, targetMemory, indexA, indexB);
    std::mutex instructionMutex;
    std::vector<std::unique_ptr<Differential_Instruction>> instructions;
    Threader threader;
    for (const auto& [windowInfo, matches] : matchingRegions) {
        const auto& [windowSize, windowIndexA, windowIndexB] = windowInfo;
        //threader.addJob(
        //    [&, matches, windowSize, windowIndexA, windowIndexB]()
        {
            // INSERT entire window when no matches are found
            if (matches.empty()) {
                auto inst = std::make_unique<Insert_Instruction>();
                inst->m_index = windowIndexB;
                inst->m_newData.resize(windowSize);
                const auto subRange = targetMemory.subrange(windowIndexB, windowSize);
                std::copy(subRange.cbegin(), subRange.cend(), inst->m_newData.begin());
                std::unique_lock<std::mutex> writeGuard(instructionMutex);
                instructions.emplace_back(std::move(inst));
            }
            else {
                size_t lastMatchEnd(windowIndexB);
                for (auto& matchInfo : matches) {
                    // INSERT data from end of the last match until the beginning of current match
                    const auto newDataLength = matchInfo.start2 - lastMatchEnd;
                    if (newDataLength > 0ULL) {
                        auto inst = std::make_unique<Insert_Instruction>();
                        inst->m_index = lastMatchEnd;
                        inst->m_newData.resize(newDataLength);
                        const auto subRange = targetMemory.subrange(lastMatchEnd, newDataLength);
                        std::copy(subRange.cbegin(), subRange.cend(), inst->m_newData.begin());
                        std::unique_lock<std::mutex> writeGuard(instructionMutex);
                        instructions.emplace_back(std::move(inst));
                    }

                    // COPY data in matching region
                    auto inst = std::make_unique<Copy_Instruction>();
                    inst->m_index = matchInfo.start2;
                    inst->m_beginRead = matchInfo.start1;
                    inst->m_endRead = matchInfo.start1 + matchInfo.length;
                    lastMatchEnd = matchInfo.start2 + matchInfo.length;
                    std::unique_lock<std::mutex> writeGuard(instructionMutex);
                    instructions.emplace_back(std::move(inst));
                }

                // INSERT data from end of the last match until the end of the current window
                const auto newDataLength = (windowIndexB + windowSize) - lastMatchEnd;
                if (newDataLength > 0ULL) {
                    auto inst = std::make_unique<Insert_Instruction>();
                    inst->m_index = lastMatchEnd;
                    inst->m_newData.resize(newDataLength);
                    const auto subRange =
                        targetMemory.subrange(lastMatchEnd, newDataLength);
                    std::copy(subRange.cbegin(), subRange.cend(),
                        inst->m_newData.begin());
                    std::unique_lock<std::mutex> writeGuard(instructionMutex);
                    instructions.emplace_back(std::move(inst));
                }
            }
        }
        //);
    }

    // INSERT data from end of the last window until the end of the buffer range
    const auto sizeB = targetMemory.size();
    if (indexB < sizeB) {
        auto inst = std::make_unique<Insert_Instruction>();
        inst->m_index = indexB;
        inst->m_newData.resize(sizeB - indexB);
        const auto subRange = targetMemory.subrange(indexB, sizeB - indexB);
        std::copy(subRange.cbegin(), subRange.cend(), inst->m_newData.begin());
        std::unique_lock<std::mutex> writeGuard(instructionMutex);
        instructions.emplace_back(std::move(inst));
    }

    // Wait for jobs to finish
    while (!threader.isFinished())
        continue;

    // Replace insertions with some repeat instructions
    const size_t startInstCount = instructions.size();
    for (size_t i = 0; i < startInstCount; ++i) {
        if (auto inst = dynamic_cast<Insert_Instruction*>(instructions[i].get())) {
            threader.addJob([inst, &instructions, &instructionMutex]() {
                // We only care about repeats larger than 36 bytes.
                if (inst->m_newData.size() > 36ull) {
                    // Upper limit (mx and my) reduced by 36, since we only care about matches that exceed 36 bytes
                    size_t max = std::min<size_t>(inst->m_newData.size(), inst->m_newData.size() - 37ull);
                    for (size_t x = 0ull; x < max; ++x) {
                        const auto& value_at_x = inst->m_newData[x];
                        if (inst->m_newData[x + 36ull] != value_at_x)
                            continue; // quit early if the value 36 units away isn't the same as this index

                        size_t y = x + 1;
                        while (y < max) {
                            if (value_at_x == inst->m_newData[y])
                                y++;
                            else
                                break;
                        }

                        const auto length = y - x;
                        if (length > 36ull) {
                            // Worthwhile to insert a new instruction
                            // Keep data up until region where repeats occur
                            auto instBefore = std::make_unique<Insert_Instruction>();
                            instBefore->m_index = inst->m_index;
                            instBefore->m_newData.resize(x);
                            std::copy(inst->m_newData.data(), inst->m_newData.data() + x, instBefore->m_newData.data());

                            // Generate new Repeat Instruction
                            auto instRepeat = std::make_unique<Repeat_Instruction>();
                            instRepeat->m_index = inst->m_index + x;
                            instRepeat->m_value = value_at_x;
                            instRepeat->m_amount = length;

                            // Modifying instructions vector
                            std::unique_lock<std::mutex> writeGuard(instructionMutex);
                            instructions.emplace_back(std::move(instBefore));
                            instructions.emplace_back(std::move(instRepeat));

                            // Modify original insert-instruction to contain remainder of the data
                            inst->m_index = inst->m_index + x + length;
                            std::memmove(&inst->m_newData[0], &inst->m_newData[y], inst->m_newData.size() - y);
                            inst->m_newData.resize(inst->m_newData.size() - y);
                            writeGuard.unlock();
                            writeGuard.release();

                            x = ULLONG_MAX; // require overflow, because we want next iteration for x == 0
                            max = std::min<size_t>(inst->m_newData.size(), inst->m_newData.size() - 37ull);
                            break;
                        }
                        x = y - 1;
                        break;
                    }
                }
                });
        }
    }

    // Wait for jobs to finish
    while (!threader.isFinished())
        continue;
    threader.shutdown();

    // Create a buffer to contain all the diff instructions
    const auto size_patch = std::accumulate(
        instructions.cbegin(),
        instructions.cend(),
        0ULL,
        [](const size_t& currentSum, const std::unique_ptr<Differential_Instruction>& instruction) noexcept {
            return currentSum + instruction->size();
        }
    );
    Buffer patchBuffer(size_patch);

    // Write the instruction data to a buffer
    size_t m_index(0ULL);
    for (const auto& instruction : instructions)
        instruction->write(patchBuffer, m_index);

    // Free up memory
    instructions.clear();
    instructions.shrink_to_fit();

    // Try to compress the patch buffer
    if (auto compressionResult = patchBuffer.compress())
        std::swap(patchBuffer, *compressionResult);
    else
        return {}; // Failure

    // Prepend header information
    constexpr size_t headerSize = sizeof(DifferentialHeader);
    Buffer bufferWithHeader(patchBuffer.size() + headerSize);
    DifferentialHeader diffHeader{ "yatta diff", sizeB };

    // Copy header data into new buffer at the beginning
    bufferWithHeader.in_type(diffHeader);

    // Copy remaining data
    bufferWithHeader.in_raw(patchBuffer.bytes(), patchBuffer.size(), headerSize);

    return bufferWithHeader; // Success
}

std::optional<Buffer> Buffer::patch(const Buffer& diffBuffer) const
{
    return Buffer::patch(*this, diffBuffer);
}

std::optional<Buffer> Buffer::patch(const Buffer& sourceBuffer, const Buffer& diffBuffer)
{
    return Buffer::patch(Buffer::MemoryRange(sourceBuffer), Buffer::MemoryRange(diffBuffer));
}

std::optional<Buffer> Buffer::patch(const MemoryRange& sourceMemory, const MemoryRange& diffMemory)
{
    // Ensure diff buffer at least *exists* (ignore source buffer, when empty we treat instruction as a brand new file)
    if (diffMemory.empty())
        return {}; // Failure

    // Read in header
    DifferentialHeader header;
    diffMemory.out_type(header);

    // Ensure header title matches
    if (std::strcmp(header.m_title, "yatta diff") != 0)
        return {}; // Failure

    // Try to decompress the diff buffer
    constexpr size_t diffHeaderSize = sizeof(DifferentialHeader);
    const auto dataSize = diffMemory.size() - diffHeaderSize;
    Buffer patchBuffer;
    if (auto decompressionResult = decompress(diffMemory.subrange(diffHeaderSize, dataSize)))
        std::swap(patchBuffer, *decompressionResult);
    else
        return {}; // Failure
    const auto patchBufferSize = patchBuffer.size();

    // Convert buffer into instructions
    Buffer bufferNew(header.m_targetSize);
    size_t byteIndex(0ULL);
    while (byteIndex < patchBufferSize) {
        // Deduce the instruction type
        char type(0);
        patchBuffer.out_type(type, byteIndex);
        byteIndex += static_cast<size_t>(sizeof(char));

        const auto executeInstruction = [&](auto instruction) {
            // Read the instruction
            instruction.read(patchBuffer, byteIndex);

            // Execute the instruction
            instruction.execute(bufferNew, sourceMemory);
        };

        // Make and execute the instruction from the diff buffer memory
        if (type == 'R')
            executeInstruction(Repeat_Instruction());
        else if (type == 'I')
            executeInstruction(Insert_Instruction());
        else if (type == 'C')
            executeInstruction(Copy_Instruction());
    }

    // Success
    return bufferNew;
}