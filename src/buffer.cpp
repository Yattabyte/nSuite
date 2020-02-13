#include "buffer.hpp"
#include "lz4/lz4.h"
#include "threader.hpp"
#include <algorithm>
#include <climits>
#include <cstring>
#include <numeric>
#include <vector>

// Convenience Definitions
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
    // Public Default Members
    virtual ~Differential_Instruction() = default;
    Differential_Instruction() = default;
    Differential_Instruction(const Differential_Instruction& other) = default;
    Differential_Instruction(Differential_Instruction&& other) = default;
    Differential_Instruction&
    operator=(const Differential_Instruction& other) = default;
    Differential_Instruction&
    operator=(Differential_Instruction&& other) = default;
    // Interface Declaration
    /** Retrieve the byte-size of this instruction. */
    [[nodiscard]] virtual size_t size() const noexcept = 0;
    /** Execute this instruction. */
    virtual void
    execute(Buffer& bufferNew, const MemoryRange& bufferOld) const = 0;
    /** Write-out this instruction to a buffer. */
    virtual void write(Buffer& outputBuffer) const = 0;
    /** Read-in this instruction from a buffer. */
    virtual void read(const Buffer& inputBuffer, size_t& byteIndex) = 0;

    // Attributes
    size_t m_index = 0ULL;
};
/** Diff instruction to copy an existing segment. */
struct Copy_Instruction final : public Differential_Instruction {
    // Interface Implementation
    [[nodiscard]] size_t size() const noexcept final {
        return static_cast<size_t>(sizeof(char) + (sizeof(size_t) * 3ULL));
    }
    void execute(Buffer& bufferNew, const MemoryRange& bufferOld) const final {
        const auto old_subRange =
            bufferOld.subrange(m_beginRead, m_endRead - m_beginRead);
        std::copy(
            old_subRange.cbegin(), old_subRange.cend(), &bufferNew[m_index]);
    }
    void write(Buffer& outputBuffer) const final {
        // Write Attributes
        outputBuffer.push_type('C');
        outputBuffer.push_type(m_index);
        outputBuffer.push_type(m_beginRead);
        outputBuffer.push_type(m_endRead);
    }
    void read(const Buffer& inputBuffer, size_t& byteIndex) final {
        // Read Attributes
        // Type already read
        inputBuffer.out_type(m_index, byteIndex);
        byteIndex += sizeof(size_t);
        inputBuffer.out_type(m_beginRead, byteIndex);
        byteIndex += sizeof(size_t);
        inputBuffer.out_type(m_endRead, byteIndex);
        byteIndex += sizeof(size_t);
    }

    // Public Attributes
    size_t m_beginRead = 0ULL, m_endRead = 0ULL;
};
/** Diff instruction for inserting an entirely new data segment. */
struct Insert_Instruction final : public Differential_Instruction {
    // Interface Implementation
    [[nodiscard]] size_t size() const noexcept final {
        return static_cast<size_t>(
            sizeof(char) + (sizeof(size_t) * 2ULL) +
            (sizeof(char) * m_newData.size()));
    }
    void execute(Buffer& bufferNew, const MemoryRange& /*unused*/) const final {
        std::copy(m_newData.cbegin(), m_newData.cend(), &bufferNew[m_index]);
    }
    void write(Buffer& outputBuffer) const final {
        // Write Attributes
        outputBuffer.push_type('I');
        outputBuffer.push_type(m_index);
        const auto length = m_newData.size();
        outputBuffer.push_type(length);
        if (length != 0U)
            outputBuffer.push_raw(m_newData.data(), length);
    }
    void read(const Buffer& inputBuffer, size_t& byteIndex) final {
        // Read Attributes
        // Type already read
        inputBuffer.out_type(m_index, byteIndex);
        byteIndex += sizeof(size_t);
        size_t length(0ULL);
        inputBuffer.out_type(length, byteIndex);
        byteIndex += sizeof(size_t);
        if (length != 0ULL) {
            m_newData.resize(length);
            inputBuffer.out_raw(m_newData.data(), length, byteIndex);
            byteIndex += sizeof(char) * length;
        }
    }

    // Attributes
    std::vector<std::byte> m_newData;
};
/** Diff instruction for a repeating value. */
struct Repeat_Instruction final : public Differential_Instruction {
    // Interface Implementation
    [[nodiscard]] size_t size() const noexcept final {
        return static_cast<size_t>(
            sizeof(char) + (sizeof(size_t) * 2ULL) + sizeof(char));
    }
    void execute(Buffer& bufferNew, const MemoryRange& /*unused*/) const final {
        std::fill(
            &bufferNew[m_index],
            &bufferNew[std::min(m_index + m_amount, bufferNew.size())],
            m_value);
    }
    void write(Buffer& outputBuffer) const final {
        // Write Attributes
        outputBuffer.push_type('R');
        // Write Index
        outputBuffer.push_type(m_index);
        // Write Amount
        outputBuffer.push_type(m_amount);
        // Write Value
        outputBuffer.push_type(m_value);
    }
    void read(const Buffer& inputBuffer, size_t& byteIndex) final {
        // Read Attributes
        // Type already read
        inputBuffer.out_type(m_index, byteIndex);
        byteIndex += sizeof(size_t);
        inputBuffer.out_type(m_amount, byteIndex);
        byteIndex += sizeof(size_t);
        inputBuffer.out_type(m_value, byteIndex);
        byteIndex += sizeof(char);
    }

    // Attributes
    size_t m_amount = 0ULL;
    std::byte m_value = static_cast<std::byte>(0);
};
/** Defines a matching region. */
struct MatchInfo {
    size_t length = 0ULL, start1 = 0ULL, start2 = 0ULL;
};
/** Defines a window. */
struct WindowInfo {
    size_t windowSize = 0ULL, indexA = 0ULL, indexB = 0ULL;
};

// Private Static Methods

/** Find matching regions for 2 given ranges. */
auto find_matching_regions(
    const MemoryRange& rangeA, const MemoryRange& rangeB) {
    std::vector<MatchInfo> bestMatch;
    size_t largestMatch(0ULL);

    // Algorithm Overview
    // For each element in Range A, compare from element index -> end
    // against entire Range B
    std::for_each(
        rangeA.cbegin_t<size_t>(), rangeA.cend_t<size_t>(),
        [&](const size_t& elementA) {
            size_t matchCount(0ULL);
            size_t sumMatches(0ULL);
            std::vector<MatchInfo> matches;
            const size_t index_8byte =
                &elementA - reinterpret_cast<const size_t*>(rangeA.cbegin());
            const size_t index_byte = index_8byte * sizeof(size_t);
            const auto subRangeA =
                rangeA.subrange(index_byte, rangeA.size() - index_byte);
            const auto length = std::min(subRangeA.size(), rangeB.size());

            // Iterate through {subRangeA and rangeB} 8 bytes at a time
            // Take note of matches >= 32 bytes in a row
            for (size_t ind = 0ULL; ind < length; ind += 8ULL) {
                const auto& byteA =
                    *reinterpret_cast<const size_t*>(&subRangeA[ind]);
                const auto& byteB =
                    *reinterpret_cast<const size_t*>(&rangeB[ind]);
                if (byteA == byteB)
                    ++matchCount;
                else {
                    if (matchCount >= 4ULL) {
                        const auto matchLength = matchCount * sizeof(size_t);
                        matches.emplace_back(MatchInfo{
                            matchLength, ind + index_byte - matchLength,
                            ind - matchLength });
                        sumMatches += matchCount;
                    }
                    matchCount = 0ULL;
                }
            }

            if (sumMatches > largestMatch) {
                largestMatch = sumMatches;
                bestMatch = matches;
            }
        });
    return bestMatch;
}

/** Split 2 ranges and find their matching ranges. */
auto split_and_match_ranges(
    const MemoryRange& rangeA, const MemoryRange& rangeB, size_t& indexA,
    size_t& indexB) {
    const auto sizeA = rangeA.size();
    const auto sizeB = rangeB.size();
    std::mutex matchMutex;
    std::vector<std::pair<WindowInfo, std::vector<MatchInfo>>> matchingRegions;

    Threader threader;
    while (indexA < sizeA && indexB < sizeB) {
        const auto windowSize = std::min(
            static_cast<size_t>(4096ULL),
            std::min(sizeA - indexA, sizeB - indexB));

        threader.addJob([&, windowSize, indexA, indexB]() {
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
        });

        // increment
        indexA += windowSize;
        indexB += windowSize;
    }

    // Wait for jobs to finish
    while (!threader.isFinished())
        continue;

    return matchingRegions;
}

/** Generate and emplace a new insertion instruction. */
void emplace_insertion(
    const size_t& index, const MemoryRange& range, std::mutex& mutex,
    std::vector<std::unique_ptr<Differential_Instruction>>& instructions) {
    // Make an instruction from input arguments
    auto inst = std::make_unique<Insert_Instruction>();
    inst->m_index = index;
    inst->m_newData.resize(range.size());

    // Copy buffer data
    std::copy(range.cbegin(), range.cend(), inst->m_newData.begin());

    // Emplace instruction back in vector
    std::unique_lock<std::mutex> writeGuard(mutex);
    instructions.emplace_back(std::move(inst));
}

/** Generate and emplace a new copy instruction. */
void emplace_copy(
    const size_t& index, const size_t& beginRead, const size_t& endRead,
    std::mutex& mutex,
    std::vector<std::unique_ptr<Differential_Instruction>>& instructions) {
    // Make an instruction from input arguments
    auto inst = std::make_unique<Copy_Instruction>();
    inst->m_index = index;
    inst->m_beginRead = beginRead;
    inst->m_endRead = endRead;

    // Emplace instruction back in vector
    std::unique_lock<std::mutex> writeGuard(mutex);
    instructions.emplace_back(std::move(inst));
}

/** Generate a diff instruction set from 2 ranges. */
auto generate_instructions(
    const MemoryRange& rangeA, const MemoryRange& rangeB) {
    size_t indexA(0ULL);
    size_t indexB(0ULL);
    const auto matchingRegions =
        split_and_match_ranges(rangeA, rangeB, indexA, indexB);
    Threader threader;
    std::mutex instructionMutex;
    std::vector<std::unique_ptr<Differential_Instruction>> instructions;
    for (const auto& matchRegion : matchingRegions) {
        threader.addJob([&, matchRegion]() {
            const auto& [windowInfo, matches] = matchRegion;
            // INSERT entire window when no matches are found
            if (matches.empty())
                emplace_insertion(
                    windowInfo.indexB,
                    rangeB.subrange(windowInfo.indexB, windowInfo.windowSize),
                    instructionMutex, instructions);
            else {
                size_t lastMatchEnd(windowInfo.indexB);
                for (auto& matchInfo : matches) {
                    // INSERT data from end of the last match until now
                    const auto newDataLength = matchInfo.start2 - lastMatchEnd;
                    if (newDataLength > 0ULL)
                        emplace_insertion(
                            lastMatchEnd,
                            rangeB.subrange(lastMatchEnd, newDataLength),
                            instructionMutex, instructions);

                    // COPY data in matching region
                    emplace_copy(
                        matchInfo.start2, matchInfo.start1,
                        matchInfo.start1 + matchInfo.length, instructionMutex,
                        instructions);
                    lastMatchEnd = matchInfo.start2 + matchInfo.length;
                }

                // INSERT data from end of the last match until window end
                const auto newDataLength =
                    (windowInfo.indexB + windowInfo.windowSize) - lastMatchEnd;
                if (newDataLength > 0ULL)
                    emplace_insertion(
                        lastMatchEnd,
                        rangeB.subrange(lastMatchEnd, newDataLength),
                        instructionMutex, instructions);
            }
        });
    }

    // INSERT data from end of the last window until the end of the buffer range
    if (const auto sizeB = rangeB.size(); indexB < sizeB)
        emplace_insertion(
            indexB, rangeB.subrange(indexB, sizeB - indexB), instructionMutex,
            instructions);

    // Wait for jobs to finish
    while (!threader.isFinished())
        continue;

    return instructions;
}

/** Retrieve insertion instructions larger than 36 bytes. */
std::vector<Insert_Instruction*> get_large_insertions(
    std::vector<std::unique_ptr<Differential_Instruction>>& instructions) {
    // Create a vector large enough for worst-case-scenario
    std::vector<Insert_Instruction*> largeInsertions;
    largeInsertions.reserve(instructions.size());

    // Find all valid insertion instructions
    for (const auto& instruction : instructions) {
        // We only care about repeats larger than 36 bytes.
        auto inst = dynamic_cast<Insert_Instruction*>(instruction.get());
        if (inst == nullptr || inst->m_newData.size() <= 36ULL)
            continue;

        // Use this instruction
        largeInsertions.emplace_back(inst);
    }

    return largeInsertions;
}

/** Find the last common value in a repeating series. */
size_t
find_last_in_series(const std::byte& value, const MemoryRange& range) noexcept {
    // Compare 8-byte-wise
    size_t index(0ULL);
    const std::byte value_array[8] = { value, value, value, value,
                                       value, value, value, value };
    const auto& value_8 = *reinterpret_cast<const size_t*>(&value_array[0]);
    const auto* const range_8 = reinterpret_cast<const size_t*>(range.bytes());
    const size_t size(range.size());
    const size_t max(size / 8ULL);
    for (; index < max; ++index)
        if (range_8[index] != value_8)
            break;

    // Compare 1-byte-wise
    const auto range_1 = range.bytes();
    index *= 8ULL;
    for (; index < size; ++index)
        if (range_1[index] != value)
            break;

    return index;
}

/** Split an insertion instruction into two insertions and a repeat. */
void split_insertion(
    Insert_Instruction* const& inst, const size_t& startIndex,
    const size_t& endIndex, const std::byte& value_at_x,
    std::vector<std::unique_ptr<Differential_Instruction>>& instructions,
    std::mutex& mutex) {
    // Keep data up until region where repeats occur
    Insert_Instruction instBefore;
    instBefore.m_index = inst->m_index;
    instBefore.m_newData.resize(startIndex);
    std::copy(
        inst->m_newData.data(), inst->m_newData.data() + startIndex,
        instBefore.m_newData.data());

    // Generate new Repeat Instruction
    Repeat_Instruction instRepeat;
    const auto length = endIndex - startIndex;
    instRepeat.m_index = inst->m_index + startIndex;
    instRepeat.m_value = value_at_x;
    instRepeat.m_amount = length;

    // Retain remainder of insertion data
    const auto size = inst->m_newData.size();
    inst->m_index = inst->m_index + startIndex + length;
    std::memmove(
        &inst->m_newData[0], &inst->m_newData[endIndex], size - endIndex);
    inst->m_newData.resize(size - endIndex);

    std::unique_lock<std::mutex> writeGuard(mutex);
    instructions.emplace_back(
        std::make_unique<Insert_Instruction>(std::move(instBefore)));
    instructions.emplace_back(
        std::make_unique<Repeat_Instruction>(std::move(instRepeat)));
}

/** Replace repeating segments in insertion instructions with repeats. */
void insertions_to_repeats(
    std::vector<std::unique_ptr<Differential_Instruction>>& baseInstructions) {
    // Analyze segments larger than 36 bytes in a separate thread
    Threader threader;
    std::mutex instructionMutex;
    std::vector<std::unique_ptr<Differential_Instruction>> newInstructions;
    for (const auto& inst : get_large_insertions(baseInstructions)) {
        threader.addJob([inst, &newInstructions, &instructionMutex]() {
            size_t startIndex(0ULL);
            size_t max(inst->m_newData.size());
            while (startIndex + 36ULL < max) {
                // Find how far this value is repeated for
                const auto& value_at_x = inst->m_newData[startIndex];
                const auto endIndex =
                    startIndex +
                    find_last_in_series(
                        value_at_x,
                        MemoryRange{ max - startIndex,
                                     &inst->m_newData[startIndex] });

                // Quit early if repeats less than 36 bytes
                if ((endIndex - startIndex) <= 36ULL) {
                    startIndex = endIndex;
                    continue;
                }

                // Split the insertion instruction into two, plus a repeat
                split_insertion(
                    inst, startIndex, endIndex, value_at_x, newInstructions,
                    instructionMutex);

                // Start at beginning of remaining segment
                startIndex = 0ULL;
                max = inst->m_newData.size();
            }
        });
    }

    // Wait for jobs to finish
    while (!threader.isFinished())
        continue;
    threader.shutdown();

    // Join instruction sets together
    baseInstructions.reserve(baseInstructions.size() + newInstructions.size());
    baseInstructions.insert(
        baseInstructions.end(),
        std::make_move_iterator(newInstructions.begin()),
        std::make_move_iterator(newInstructions.end()));
}

// Public (de)Constructors

Buffer::Buffer(const size_t& size)
    : MemoryRange(size, nullptr), m_capacity(size * 2ULL),
      m_data(std::make_unique<std::byte[]>(m_capacity)) {
    m_dataPtr = m_data.get();
}

Buffer::Buffer(const Buffer& other)
    : MemoryRange(other), m_capacity(other.m_capacity),
      m_data(std::make_unique<std::byte[]>(other.m_capacity)) {
    m_dataPtr = m_data.get();
    std::copy(
        other.m_data.get(), other.m_data.get() + other.m_range, m_data.get());
}

Buffer::Buffer(Buffer&& other) noexcept
    : MemoryRange(std::move(other)), m_capacity(other.m_capacity),
      m_data(std::move(other.m_data)) {
    other.m_capacity = 0ULL;
    other.m_data = nullptr;
}

// Public Assignment Operators

Buffer& Buffer::operator=(const Buffer& other) {
    if (this != &other) {
        m_range = other.m_range;
        m_capacity = other.m_capacity;
        m_data = std::make_unique<std::byte[]>(other.m_capacity);
        m_dataPtr = m_data.get();
        std::copy(
            other.m_data.get(), other.m_data.get() + other.m_range,
            m_data.get());
    }
    return *this;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
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

bool Buffer::empty() const noexcept {
    return m_data == nullptr || m_range == 0ULL || m_capacity == 0ULL;
}

size_t Buffer::capacity() const noexcept { return m_capacity; }

// Public Manipulation Methods

void Buffer::resize(const size_t& size) {
    // Create the data container if it is missing
    if (m_data == nullptr) {
        m_capacity = size * 2ULL;
        m_data = std::make_unique<std::byte[]>(m_capacity);
        m_dataPtr = m_data.get();
    }

    // Check if our previous buffer is too small
    else if (size > m_capacity) {
        // Allocate new container
        m_capacity = size * 2ULL;
        auto newData = std::make_unique<std::byte[]>(m_capacity);

        // Copy over old data
        std::copy(m_data.get(), m_data.get() + m_range, newData.get());

        // Swap data containers
        m_data.swap(newData);
        m_dataPtr = m_data.get();
    }

    m_range = size;
}

void Buffer::reserve(const size_t& capacity) {
    if (capacity > m_capacity) {
        m_capacity = capacity;
        auto newData = std::make_unique<std::byte[]>(m_capacity);

        // Copy previous data if present
        if (m_data != nullptr)
            std::copy(m_data.get(), m_data.get() + m_range, newData.get());

        // Swap data containers
        m_data.swap(newData);
        m_dataPtr = m_data.get();
    }
}

void Buffer::shrink() {
    // Ensure there is data to shrink
    if (m_data == nullptr)
        return;

    // Allocate new container
    auto newData = std::make_unique<std::byte[]>(m_range);

    // Copy over old data
    std::copy(m_data.get(), m_data.get() + m_range, newData.get());

    // Swap data containers
    m_data.swap(newData);
    m_dataPtr = m_data.get();
    m_capacity = m_range;
}

void Buffer::clear() noexcept {
    m_data.reset();
    m_range = 0ULL;
    m_capacity = 0ULL;
    m_data = nullptr;
    m_dataPtr = nullptr;
}

// Public IO Methods

void Buffer::push_raw(const void* const dataPtr, const size_t& size) {
    // Find the starting index to write at
    const auto byteIndex = m_range;
    // Grow the container to hold the new data
    resize(m_range + size);
    // Copy the data into the container
    in_raw(dataPtr, size, byteIndex);
}

void Buffer::pop_raw(void* const dataPtr, const size_t& size) {
    // Find the starting index to read at
    const auto byteIndex = m_range - size;
    // Copy the data out of the container
    out_raw(dataPtr, size, byteIndex);
    // Shrink the container by the size of the data
    resize(m_range - size);
}

// Public Derivation Methods

std::optional<Buffer> Buffer::compress() const {
    return Buffer::compress(*this);
}

std::optional<Buffer> Buffer::compress(const Buffer& buffer) {
    const MemoryRange& range = buffer;
    return Buffer::compress(range);
}

std::optional<Buffer> Buffer::compress(const MemoryRange& memoryRange) {
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
        static_cast<int>(sourceSize), static_cast<int>(destinationSize));

    // Ensure we have a non-zero sized buffer
    if (compressedSize == 0ULL)
        return {}; // Failure

    // We now know the actual compressed size, downsize our oversized buffer to
    // the compressed size
    compressedBuffer.resize(headerSize + compressedSize);
    compressedBuffer.shrink();

    // Success
    return compressedBuffer;
}

std::optional<Buffer> Buffer::decompress() const {
    return Buffer::decompress(*this);
}

std::optional<Buffer> Buffer::decompress(const Buffer& buffer) {
    const MemoryRange& range = buffer;
    return Buffer::decompress(range);
}

std::optional<Buffer> Buffer::decompress(const MemoryRange& memoryRange) {
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
        &memoryRange.charArray()[headerSize], uncompressedBuffer.charArray(),
        static_cast<int>(memoryRange.size() - headerSize),
        static_cast<int>(uncompressedBuffer.size()));

    // Ensure we have a non-zero sized decompressed buffer
    if (decompressionResult <= 0)
        return {}; // Failure

    // Success
    return uncompressedBuffer;
}

std::optional<Buffer> Buffer::diff(const Buffer& target) const {
    return Buffer::diff(*this, target);
}

std::optional<Buffer>
Buffer::diff(const Buffer& sourceBuffer, const Buffer& targetBuffer) {
    const MemoryRange& sourcetRange = sourceBuffer;
    const MemoryRange& targetRange = targetBuffer;
    return Buffer::diff(sourcetRange, targetRange);
}

std::optional<Buffer>
Buffer::diff(const MemoryRange& sourceMemory, const MemoryRange& targetMemory) {
    // Ensure that at least ONE of the two source buffers exists
    if (sourceMemory.empty() && targetMemory.empty())
        return {}; // Failure

    // Convert matching regions into diff instructions
    auto instructions = generate_instructions(sourceMemory, targetMemory);

    // Replace insertions with some repeat instructions
    insertions_to_repeats(instructions);

    // Create a buffer to contain all the diff instructions
    const auto size_patch = std::accumulate(
        instructions.cbegin(), instructions.cend(), 0ULL,
        [](const auto& currentSum, const auto& instruction) noexcept {
            return currentSum + instruction->size();
        });
    Buffer patchBuffer(size_patch);

    // Write the instruction data to a buffer
    for (const auto& instruction : instructions)
        instruction->write(patchBuffer);

    // Free up memory
    instructions.clear();
    instructions.shrink_to_fit();

    // Try to compress the patch buffer
    if (auto result = patchBuffer.compress())
        std::swap(patchBuffer, *result);
    else
        return {}; // Failure

    // Prepend header information
    constexpr size_t headerSize = sizeof(DifferentialHeader);
    DifferentialHeader diffHeader{ "yatta diff", targetMemory.size() };
    Buffer bufferWithHeader;
    bufferWithHeader.reserve(patchBuffer.size() + headerSize);

    // Copy header data into new buffer at the beginning
    bufferWithHeader.push_type(diffHeader);
    bufferWithHeader.push_raw(patchBuffer.bytes(), patchBuffer.size());

    return bufferWithHeader; // Success
}

std::optional<Buffer> Buffer::patch(const Buffer& diffBuffer) const {
    return Buffer::patch(*this, diffBuffer);
}

std::optional<Buffer>
Buffer::patch(const Buffer& sourceBuffer, const Buffer& diffBuffer) {
    const MemoryRange& sourcetRange = sourceBuffer;
    const MemoryRange& diffRange = diffBuffer;
    return Buffer::patch(sourcetRange, diffRange);
}

std::optional<Buffer>
Buffer::patch(const MemoryRange& sourceMemory, const MemoryRange& diffMemory) {
    // Ensure diff buffer at least *exists*, empty source = new file
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
    auto patchBuffer =
        decompress(diffMemory.subrange(diffHeaderSize, dataSize));
    if (!patchBuffer.has_value())
        return {}; // Failure
    const auto patchBufferSize = patchBuffer->size();

    // Convert buffer into instructions
    Buffer bufferNew(header.m_targetSize);
    size_t byteIndex(0ULL);
    while (byteIndex < patchBufferSize) {
        // Deduce the instruction type
        char type(0);
        patchBuffer->out_type(type, byteIndex);
        byteIndex += sizeof(char);

        // Make and execute the instruction from the diff buffer memory
        const auto executeInstruction = [&](auto instruction) {
            // Read the instruction
            instruction.read(*patchBuffer, byteIndex);

            // Execute the instruction
            instruction.execute(bufferNew, sourceMemory);
        };
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