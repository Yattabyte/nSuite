#include "Buffer.hpp"
#include "BufferView.hpp"
#include "Threader.hpp"
#include "lz4/lz4.h"
#include <algorithm>
#include <numeric>
#include <vector>

using namespace yatta;


// Public (de)Constructors

Buffer::Buffer(const size_t& size) :
    m_size(size),
    m_capacity(size * 2ULL),
    m_data(std::make_unique<std::byte[]>(m_capacity))
{
}

Buffer::Buffer(const Buffer& other) :
	m_size(other.m_size),
	m_capacity(other.m_capacity),
	m_data(std::make_unique<std::byte[]>(other.m_capacity))
{
	std::copy(other.m_data.get(), other.m_data.get() + other.m_size, m_data.get());
}

Buffer::Buffer(Buffer&& other) noexcept :
	m_size(other.m_size),
	m_capacity(other.m_capacity),
	m_data(std::move(other.m_data))
{
	other.m_size = 0ULL;
	other.m_capacity = 0ULL;
	other.m_data = nullptr;
}


// Public Assignment Operators

Buffer& Buffer::operator=(const Buffer& other)
{
	if (this != &other) {
		m_size = other.m_size;
		m_capacity = other.m_capacity;
		m_data = std::make_unique<std::byte[]>(other.m_capacity);
		std::copy(other.m_data.get(), other.m_data.get() + other.m_size, m_data.get());
	}
	return *this;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept
{
	if (this != &other) {
		m_size = other.m_size;
		m_capacity = other.m_capacity;
		m_data = std::move(other.m_data);

		other.m_size = 0ULL;
		other.m_capacity = 0ULL;
		other.m_data = nullptr;
	}
	return *this;
}


// Public Inquiry Methods

bool Buffer::empty() const noexcept
{
	return m_size == 0ULL && m_capacity == 0ULL;
}

bool Buffer::hasData() const noexcept
{
	return m_size > 0ULL;
}

size_t Buffer::size() const noexcept
{
	return m_size;
}

size_t Buffer::capacity() const noexcept
{
	return m_capacity;
}

size_t Buffer::hash() const noexcept
{
	return MemoryRange{ m_size, bytes() }.hash();
}


// Public Manipulation Methods

std::byte& Buffer::operator[](const size_t& byteIndex) const noexcept
{
	return m_data[byteIndex];
}

char* Buffer::charArray() const noexcept
{
	return reinterpret_cast<char*>(&m_data[0]);
}

std::byte* Buffer::bytes() const noexcept
{
	return m_data.get();
}

void Buffer::resize(const size_t& size)
{
	// Create the data container if it is missing
	if (m_data == nullptr) {
		m_capacity = size * 2ULL;
		m_data = std::make_unique<std::byte[]>(size * 2ULL);
	}

	// Check if our previous buffer is too small
	else if (size > m_capacity) {
		// Allocate new container
		auto newData = std::make_unique<std::byte[]>(size * 2ULL);
		// Copy over old data
		std::copy(m_data.get(), m_data.get() + m_size, newData.get());
		// Swap data containers
		m_capacity = size * 2ULL;
		m_data = std::move(newData);
	}

	m_size = size;
}

void Buffer::shrink()
{
	if (m_data == nullptr)
		return; // Failure

	// Allocate new container
	auto newData = std::make_unique<std::byte[]>(m_size);

	// Copy over old data
	std::copy(m_data.get(), m_data.get() + m_size, newData.get());

	// Swap data containers
	m_capacity = m_size;
	m_data = std::move(newData);
}

void Buffer::clear() noexcept
{
	m_data.release();
	m_size = 0ULL;
	m_capacity = 0ULL;
	m_data = nullptr;
}

void Buffer::in_raw(const void* const dataPtr, const size_t& size, const size_t byteIndex) noexcept
{
	// Ensure pointers are valid
	if (m_data == nullptr || dataPtr == nullptr)
		return; // Failure

	// Ensure data won't exceed range
	if ((size + byteIndex) > m_size)
		return; // Failure

	// Copy Data
	std::memcpy(&bytes()[byteIndex], dataPtr, size);
}

void Buffer::out_raw(void* const dataPtr, const size_t& size, const size_t byteIndex) const noexcept
{
	// Ensure pointers are valid
	if (m_data == nullptr || dataPtr == nullptr)
		return; // Failure

	// Ensure data won't exceed range
	if ((size + byteIndex) > m_size)
		return; // Failure

	// Copy Data
	std::memcpy(dataPtr, &bytes()[byteIndex], size);
}


// Public Derivation Methods

/** Data structure defining a compressed buffer. */
struct CompressionHeader {
	char m_title[16ULL] = { '\0' };
	size_t m_uncompressedSize = 0ULL;
};

std::optional<Buffer> Buffer::compress() const
{
	return Buffer::compress(*this);
}

std::optional<Buffer> Buffer::compress(const Buffer& buffer)
{
	// Ensure this buffer has some data to compress
	if (buffer.empty())
		return {}; // Failure

	MemoryRange memoryRange{ buffer.size(), buffer.bytes() };
	return Buffer::compress(memoryRange);
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
		reinterpret_cast<char*>(memoryRange.m_dataPtr),
		&compressedBuffer.charArray()[headerSize], // Offset by header's amount
		int(sourceSize),
		int(destinationSize)
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

std::optional<Buffer> yatta::Buffer::decompress(const Buffer& buffer)
{
	// Ensure this buffer has some data to compress
	constexpr auto headerSize = sizeof(CompressionHeader);
	if (buffer.empty() || buffer.size() < headerSize)
		return {}; // Failure

	MemoryRange memoryRange{ buffer.size(), buffer.bytes() };
	return Buffer::decompress(memoryRange);
}

std::optional<Buffer> yatta::Buffer::decompress(const MemoryRange& memoryRange)
{
	// Ensure this buffer has some data to decompress
	constexpr auto headerSize = sizeof(CompressionHeader);
	if (memoryRange.m_range == 0ULL || memoryRange.m_range < headerSize)
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
		&reinterpret_cast<char*>(memoryRange.m_dataPtr)[headerSize],
		uncompressedBuffer.charArray(),
		int(memoryRange.size() - headerSize),
		int(uncompressedBuffer.size())
	);

	// Ensure we have a non-zero sized decompressed buffer
	if (decompressionResult <= 0)
		return {}; // Failure

	// Success
	return uncompressedBuffer;
}

/** Data structure defining a differential buffer. */
struct DifferentialHeader {
	char m_title[16ULL] = { '\0' };
	size_t m_targetSize = 0ULL;
};
/** Super-class for buffer diff instructions. */
struct Differential_Instruction {
	// (de)Constructors
	inline virtual ~Differential_Instruction() = default;
	inline Differential_Instruction(const char& t) noexcept : m_type(t) {}
	inline Differential_Instruction(const Differential_Instruction& other) = delete;
	inline Differential_Instruction(Differential_Instruction&& other) = delete;
	inline Differential_Instruction& operator=(const Differential_Instruction& other) = delete;
	inline Differential_Instruction& operator=(Differential_Instruction&& other) = delete;


	// Interface Declaration
	/** Retrieve the byte-size of this instruction. */
	[[nodiscard]] virtual size_t size() const noexcept = 0;
	/** Execute this instruction. */
	virtual void execute(Buffer& bufferNew, const MemoryRange& bufferOld) const noexcept = 0;
	/** Write-out this instruction to a buffer. */
	virtual void write(Buffer& outputBuffer, size_t& byteIndex) const noexcept = 0;
	/** Read-in this instruction from a buffer. */
	virtual void read(const Buffer& inputBuffer, size_t& byteIndex) = 0;


	// Attributes
	const char m_type = 0;
	size_t m_index = 0ULL;
};
/** Specifies a region in the 'old' file to read from, and where to put it in the 'new' file. */
struct Copy_Instruction final : public Differential_Instruction {
	// Constructor
	inline Copy_Instruction() noexcept : Differential_Instruction('C') {}


	// Interface Implementation
	inline size_t size() const noexcept final {
		return sizeof(char) + (sizeof(size_t) * 3ULL);
	}
	inline void execute(Buffer& bufferNew, const MemoryRange& bufferOld) const noexcept final {
		for (auto i = m_index, x = m_beginRead; i < bufferNew.size() && x < m_endRead && x < bufferOld.size(); ++i, ++x)
			bufferNew[i] = bufferOld[x];
	}
	inline void write(Buffer& outputBuffer, size_t& byteIndex) const noexcept final {
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
	inline void read(const Buffer& inputBuffer, size_t& byteIndex) noexcept final {
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
	inline Insert_Instruction() noexcept : Differential_Instruction('I') {}


	// Interface Implementation
	inline size_t size() const noexcept final {
		return sizeof(char) + (sizeof(size_t) * 2) + (sizeof(char) * m_newData.size());
	}
	inline void execute(Buffer& bufferNew, const MemoryRange& bufferOld) const noexcept final {
		const auto length = m_newData.size();
		for (auto i = m_index, x = static_cast<size_t>(0ULL); i < bufferNew.size() && x < length; ++i, ++x)
			bufferNew[i] = m_newData[x];
	}
	inline void write(Buffer& outputBuffer, size_t& byteIndex) const noexcept final {
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
			byteIndex += length;
		}
	}
	inline void read(const Buffer& inputBuffer, size_t& byteIndex) final {
		// Type already read
		// Read Index
		inputBuffer.out_type(m_index, byteIndex);
		byteIndex += static_cast<size_t>(sizeof(size_t));
		// Read Length
		size_t length;
		inputBuffer.out_type(length, byteIndex);
		byteIndex += static_cast<size_t>(sizeof(size_t));
		if (length != 0U) {
			// Read Data
			m_newData.resize(length);
			inputBuffer.out_raw(m_newData.data(), length, byteIndex);
			byteIndex += length;
		}
	}


	// Attributes
	std::vector<std::byte> m_newData;
};
/** Contains a single value a to insert into the 'new' file, at a given point, repeating multiple times. */
struct Repeat_Instruction final : public Differential_Instruction {
	// Constructor
	inline Repeat_Instruction() noexcept : Differential_Instruction('R') {}


	// Interface Implementation
	inline size_t size() const noexcept final {
		return sizeof(char) + (sizeof(size_t) * 2ULL) + sizeof(char);
	}
	inline void execute(Buffer& bufferNew, const MemoryRange& bufferOld) const noexcept final {
		for (auto i = m_index, x = static_cast<size_t>(0ULL); i < bufferNew.size() && x < m_amount; ++i, ++x)
			bufferNew[i] = m_value;
	}
	inline void write(Buffer& outputBuffer, size_t& byteIndex) const noexcept final {
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
	inline void read(const Buffer& inputBuffer, size_t& byteIndex) noexcept final {
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

std::optional<Buffer> Buffer::diff(const Buffer& target, const size_t& maxThreads) const
{
	return Buffer::diff(*this, target, maxThreads);
}

std::optional<Buffer> Buffer::diff(const Buffer& source, const Buffer& target, const size_t& maxThreads)
{
	// Ensure that at least ONE of the two source buffers exists
	if (source.empty() && target.empty())
		return {}; // Failure

	MemoryRange sourceRange{ source.size(), source.bytes() };
	MemoryRange destinationRange{ target.size(), target.bytes() };
	return Buffer::diff(sourceRange, destinationRange, maxThreads);
}

std::optional<Buffer> Buffer::diff(const MemoryRange& sourceMemory, const MemoryRange& destinationMemory, const size_t& maxThreads)
{
	// Ensure that at least ONE of the two source buffers exists
	if (sourceMemory.empty() && destinationMemory.empty())
		return {}; // Failure

	const auto size_old = sourceMemory.size();
	const auto size_new = destinationMemory.size();
	std::vector<std::unique_ptr<Differential_Instruction>> instructions;
	instructions.reserve(std::max<size_t>(size_old, size_new) / 8ULL);
	std::mutex instructionMutex;
	Threader threader(maxThreads);
	constexpr size_t m_amount(4096);
	size_t bytesUsed_old(0ULL);
	size_t bytesUsed_new(0ULL);
	while (bytesUsed_old < size_old && bytesUsed_new < size_new) {
		// Variables for this current chunk
		auto windowSize = std::min<size_t>(m_amount, std::min<size_t>(size_old - bytesUsed_old, size_new - bytesUsed_new));

		// Find best matches for this chunk
		threader.addJob([&, windowSize, bytesUsed_old, bytesUsed_new]() {
			// Step 1: Find all regions that match
			struct MatchInfo { size_t length = 0ull, start1 = 0ull, start2 = 0ull; };
			size_t bestContinuous(0ull);
			size_t bestMatchCount(windowSize);
			std::vector<MatchInfo> bestSeries;
			const auto buffer_slice_old = &sourceMemory[bytesUsed_old];
			auto buffer_slice_new = &destinationMemory[bytesUsed_new];
			for (size_t index1 = 0ull; index1 + 8ull < windowSize; index1 += 8ull) {
				const size_t OLD_FIRST8 = *reinterpret_cast<size_t*>(&buffer_slice_old[index1]);
				std::vector<MatchInfo> matches;
				size_t largestContinuous(0ull);
				for (size_t index2 = 0ull; index2 + 8ull < windowSize; index2 += 8ull) {
					const size_t NEW_FIRST8 = *reinterpret_cast<size_t*>(&buffer_slice_new[index2]);
					// Check if values match
					if (OLD_FIRST8 == NEW_FIRST8) {
						size_t offset = 8ull;
						// Restrict matches to be at least 32 bytes (minimum byte size of 1 instruction)
						if (buffer_slice_old[index1 + 32ull] == buffer_slice_new[index2 + 32ull]) {
							// Check how long this series of matches continues for
							for (; ((index1 + offset) < windowSize) && ((index2 + offset) < windowSize); ++offset)
								if (buffer_slice_old[index1 + offset] != buffer_slice_new[index2 + offset])
									break;

							// Save series
							if (offset >= 32ull) {
								matches.emplace_back(MatchInfo{ offset, bytesUsed_old + index1, bytesUsed_new + index2 });
								if (offset > largestContinuous)
									largestContinuous = offset;
							}
						}

						index2 += offset;
					}
				}
				// Check if the recently completed series saved the most continuous data
				if (largestContinuous > bestContinuous&& matches.size() <= bestMatchCount) {
					// Save the series for later
					bestContinuous = largestContinuous;
					bestMatchCount = matches.size();
					bestSeries = matches;
				}

				// Break if the largest series is as big as the window
				if (bestContinuous >= windowSize)
					break;
			}

			// Step 2: Generate instructions based on the matching-regions found
			// Note:
			//			data from [start1 -> +length] equals [start2 -> + length]
			//			data before and after these ranges isn't equal
			if (bestSeries.empty()) {
				// NO MATCHES
				// NEW INSERT_INSTRUCTION: use entire window
				auto inst = std::make_unique<Insert_Instruction>();
				inst->m_index = bytesUsed_new;
				inst->m_newData.resize(windowSize);
				std::copy(buffer_slice_new, buffer_slice_new + windowSize, inst->m_newData.data());
				std::unique_lock<std::mutex> writeGuard(instructionMutex);
				instructions.emplace_back(std::move(inst));
			}
			else {
				size_t lastMatchEnd(bytesUsed_new);
				for (const auto& match : bestSeries) {
					const auto newDataLength = match.start2 - lastMatchEnd;
					if (newDataLength > 0ull) {
						// NEW INSERT_INSTRUCTION: Use data from end of last match until beginning of current match
						auto inst = std::make_unique<Insert_Instruction>();
						inst->m_index = lastMatchEnd;
						inst->m_newData.resize(newDataLength);
						std::copy(&destinationMemory[lastMatchEnd], &destinationMemory[lastMatchEnd + newDataLength], inst->m_newData.data());
						std::unique_lock<std::mutex> writeGuard(instructionMutex);
						instructions.emplace_back(std::move(inst));
					}

					// NEW COPY_INSTRUCTION: Use data from beginning of match until end of match
					auto inst = std::make_unique<Copy_Instruction>();
					inst->m_index = match.start2;
					inst->m_beginRead = match.start1;
					inst->m_endRead = match.start1 + match.length;
					lastMatchEnd = match.start2 + match.length;
					std::unique_lock<std::mutex> writeGuard(instructionMutex);
					instructions.emplace_back(std::move(inst));
				}

				const auto newDataLength = (bytesUsed_new + windowSize) - lastMatchEnd;
				if (newDataLength > 0ull) {
					// NEW INSERT_INSTRUCTION: Use data from end of last match until end of window
					auto inst = std::make_unique<Insert_Instruction>();
					inst->m_index = lastMatchEnd;
					inst->m_newData.resize(newDataLength);
					std::copy(&destinationMemory[lastMatchEnd], &destinationMemory[lastMatchEnd + newDataLength], inst->m_newData.data());
					std::unique_lock<std::mutex> writeGuard(instructionMutex);
					instructions.emplace_back(std::move(inst));
				}
			}
			});
		// increment
		bytesUsed_old += windowSize;
		bytesUsed_new += windowSize;
	}

	if (bytesUsed_new < size_new) {
		// NEW INSERT_INSTRUCTION: Use data from end of last block until end-of-file
		auto inst = std::make_unique<Insert_Instruction>();
		inst->m_index = bytesUsed_new;
		inst->m_newData.resize(size_new - bytesUsed_new);
		std::copy(&destinationMemory[bytesUsed_new], &destinationMemory[bytesUsed_new + (size_new - bytesUsed_new)], inst->m_newData.data());
		std::unique_lock<std::mutex> writeGuard(instructionMutex);
		instructions.emplace_back(std::move(inst));
	}

	// Wait for jobs to finish
	while (!threader.isFinished())
		continue;

	// Replace insertions with some repeat instructions
	const auto& originalInstructionCount = instructions.size();
	for (size_t i = 0; i < originalInstructionCount; ++i) {
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
	threader.prepareForShutdown();
	while (!threader.isFinished())
		continue;
	threader.shutdown();

	// Create a buffer to contain all the diff instructions
	const auto size_patch = std::accumulate(
		instructions.cbegin(),
		instructions.cend(), 0ULL,
		[](const size_t& currentSum, const std::unique_ptr<Differential_Instruction>& instruction) noexcept {
			return currentSum + instruction->size();
		}
	);
	Buffer patchBuffer(size_patch);

	// Write instruction data to the buffer
	size_t m_index(0ULL);
	for (const auto& instruction : instructions)
		instruction->write(patchBuffer, m_index);

	// Free up memory
	instructions.clear();
	instructions.shrink_to_fit();

	// Try to compress the diff buffer
	if (auto compressedPatchBuffer = patchBuffer.compress()) {
		patchBuffer.clear();
		// Prepend header information
		constexpr auto headerSize = sizeof(DifferentialHeader);
		Buffer compressedPatchBufferWithHeader(compressedPatchBuffer->size() + headerSize);
		DifferentialHeader diffHeader{ "yatta diff", size_new };

		// Copy header data into new buffer at the beginning
		compressedPatchBufferWithHeader.in_type(diffHeader);

		// Copy remaining data
		compressedPatchBufferWithHeader.in_raw(compressedPatchBuffer->bytes(), compressedPatchBuffer->size(), headerSize);

		return compressedPatchBufferWithHeader; // Success
	}

	return {}; // Failure
}

std::optional<Buffer> Buffer::patch(const Buffer& diffBuffer) const
{
	return Buffer::patch(*this, diffBuffer);
}

std::optional<Buffer> Buffer::patch(const Buffer& source, const Buffer& diffBuffer)
{
	// Ensure diff buffer at least *exists* (ignore source buffer, when empty we treat instruction as a brand new file)
	if (diffBuffer.empty())
		return {}; // Failure

	MemoryRange sourceRange{ source.size(), source.bytes() };
	MemoryRange diffRange{ diffBuffer.size(), diffBuffer.bytes() };
	return Buffer::patch(sourceRange, diffRange);
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
	const auto dataSize = diffMemory.size() - sizeof(DifferentialHeader);
	if (auto diffInstructionBuffer = BufferView{ dataSize, &diffMemory.bytes()[sizeof(DifferentialHeader)] }.decompress()) {
		// Convert buffer into instructions
		Buffer bufferNew(header.m_targetSize);
		size_t bytesRead(0ULL);
		size_t byteIndex(0ULL);
		while (bytesRead < diffInstructionBuffer->size()) {
			// Deduce the instruction type
			char type(0);
			diffInstructionBuffer->out_type(type);
			byteIndex += static_cast<size_t>(sizeof(char));

			const auto executeInstruction = [&](auto instruction) {
				instruction.read(*diffInstructionBuffer, byteIndex);

				// Read the instruction size
				bytesRead += instruction.size();

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

	return {}; // Failure
}