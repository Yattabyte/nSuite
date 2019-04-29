#include "Buffer.h"
#include "Instructions.h"
#include "Threader.h"
#include "lz4.h"
#include <algorithm>

using namespace NST;


// Public (de)Constructors

Buffer::~Buffer()
{
	release();
}

Buffer::Buffer() : m_data(0) {}

Buffer::Buffer(const size_t & size) : m_data(size) {}

Buffer::Buffer(const void * pointer, const size_t & range)
	: m_data(range)
{
	std::memcpy(&m_data[0], pointer, range);
}


// Public Derivators

std::optional<Buffer> Buffer::compress() const
{
	// Ensure this buffer has some data to compress
	if (hasData()) {
		// Pre-allocate a huge buffer to allow for compression operations
		const auto sourceSize = size();
		const auto workingSize = sourceSize * 2ull;
		Buffer workingBuffer(workingSize);

		// Try to compress the source buffer
		const auto compressedSize = LZ4_compress_default(
			cArray(),
			workingBuffer.cArray(),
			int(sourceSize),
			int(workingSize)
		);

		// Ensure we have a non-zero sized buffer
		if (compressedSize > 0) {
			// We now know the actual compressed buffer size
			// Calculate the size of the header, then generate a DECORATED compressed buffer
			Buffer compressedBuffer(CBUFFER_H_SIZE + size_t(compressedSize));

			// Write Header Information
			auto index = compressedBuffer.writeData(&CBUFFER_H_TITLE[0], CBUFFER_H_TITLE_SIZE);
			index = compressedBuffer.writeData(&sourceSize, size_t(sizeof(size_t)), index);

			// Write compressed data over
			index = compressedBuffer.writeData(workingBuffer.data(), size_t(compressedSize), index);
			workingBuffer.release();

			// Success
			return compressedBuffer;
		}
	}

	// Failure
	return {};
}

std::optional<Buffer> Buffer::decompress() const
{
	// Ensure buffer at least *exists*
	if (hasData()) {
		// Read in header title, ensure header matches
		char headerTitle_In[CBUFFER_H_TITLE_SIZE];
		auto index = readData(&headerTitle_In, CBUFFER_H_TITLE_SIZE);
		if (std::strcmp(headerTitle_In, CBUFFER_H_TITLE) == 0) {
			// Get uncompressed size
			size_t uncompressedSize(0ull);
			index = readData(&uncompressedSize, size_t(sizeof(size_t)), index);

			// Uncompress the remaining data
			/////////////////////////////////////////////////
			const size_t DATA_SIZE = size() - CBUFFER_H_SIZE;
			Buffer DELETE_ME(&(operator[](index)), DATA_SIZE);
			/////////////////////////////////////////////////
			Buffer uncompressedBuffer(uncompressedSize);
			const auto decompressionResult = LZ4_decompress_safe(
				DELETE_ME.cArray(),
				uncompressedBuffer.cArray(),
				int(DELETE_ME.size()),
				int(uncompressedBuffer.size())
			);
			DELETE_ME.release();

			// Ensure we have a non-zero sized buffer
			if (decompressionResult > 0) {
				// Success
				return uncompressedBuffer;
			}
		}
	}

	// Failure
	return {};
}

std::optional<Buffer> Buffer::diff(const Buffer & target) const
{
	// Ensure that at least ONE of the two source buffers exists
	if (hasData() || target.hasData()) {
		const auto size_old = size();
		const auto size_new = target.size();
		std::vector<InstructionTypes> instructions;
		instructions.reserve(std::max<size_t>(size_old, size_new) / 8ull);
		std::mutex instructionMutex;
		Threader threader;
		constexpr size_t amount(4096);
		size_t bytesUsed_old(0ull), bytesUsed_new(0ull);
		while (bytesUsed_old < size_old && bytesUsed_new < size_new) {
			// Variables for this current chunk
			auto windowSize = std::min<size_t>(amount, std::min<size_t>(size_old - bytesUsed_old, size_new - bytesUsed_new));

			// Find best matches for this chunk
			threader.addJob([&, windowSize, bytesUsed_old, bytesUsed_new]() {
				// Step 1: Find all regions that match
				struct MatchInfo { size_t length = 0ull, start1 = 0ull, start2 = 0ull; };
				size_t bestContinuous(0ull), bestMatchCount(windowSize);
				std::vector<MatchInfo> bestSeries;
				auto buffer_slice_old = &(operator[](bytesUsed_old));
				auto buffer_slice_new = &target[bytesUsed_new];
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
					if (largestContinuous > bestContinuous && matches.size() <= bestMatchCount) {
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
				if (!bestSeries.size()) {
					// NO MATCHES
					// NEW INSERT_INSTRUCTION: use entire window
					Insert_Instruction inst;
					inst.index = bytesUsed_new;
					inst.newData.resize(windowSize);
					std::memcpy(inst.newData.data(), buffer_slice_new, windowSize);
					std::unique_lock<std::mutex> writeGuard(instructionMutex);
					instructions.push_back(inst);
				}
				else {
					size_t lastMatchEnd(bytesUsed_new);
					for (size_t mx = 0, ms = bestSeries.size(); mx < ms; ++mx) {
						const auto & match = bestSeries[mx];

						const auto newDataLength = match.start2 - lastMatchEnd;
						if (newDataLength > 0ull) {
							// NEW INSERT_INSTRUCTION: Use data from end of last match until beginning of current match
							Insert_Instruction inst;
							inst.index = lastMatchEnd;
							inst.newData.resize(newDataLength);
							std::memcpy(inst.newData.data(), &target[lastMatchEnd], newDataLength);
							std::unique_lock<std::mutex> writeGuard(instructionMutex);
							instructions.push_back(inst);
						}

						// NEW COPY_INSTRUCTION: Use data from begining of match until end of match
						Copy_Instruction inst;
						inst.index = match.start2;
						inst.beginRead = match.start1;
						inst.endRead = match.start1 + match.length;
						lastMatchEnd = match.start2 + match.length;
						std::unique_lock<std::mutex> writeGuard(instructionMutex);
						instructions.push_back(inst);
					}

					const auto newDataLength = (bytesUsed_new + windowSize) - lastMatchEnd;
					if (newDataLength > 0ull) {
						// NEW INSERT_INSTRUCTION: Use data from end of last match until end of window
						Insert_Instruction inst;
						inst.index = lastMatchEnd;
						inst.newData.resize(newDataLength);
						std::memcpy(inst.newData.data(), &target[lastMatchEnd], newDataLength);
						std::unique_lock<std::mutex> writeGuard(instructionMutex);
						instructions.push_back(inst);
					}
				}
			});
			// increment
			bytesUsed_old += windowSize;
			bytesUsed_new += windowSize;
		}

		if (bytesUsed_new < size_new) {
			// NEW INSERT_INSTRUCTION: Use data from end of last block until end-of-file
			Insert_Instruction inst;
			inst.index = bytesUsed_new;
			inst.newData.resize(size_new - bytesUsed_new);
			std::memcpy(inst.newData.data(), &target[bytesUsed_new], size_new - bytesUsed_new);
			std::unique_lock<std::mutex> writeGuard(instructionMutex);
			instructions.push_back(inst);
		}

		// Wait for jobs to finish
		while (!threader.isFinished())
			continue;

		// Replace insertions with some repeat instructions
		for (size_t i = 0, mi = instructions.size(); i < mi; ++i) {
			threader.addJob([i, &instructions, &instructionMutex]() {
				auto & instruction = instructions[i];
				if (const auto inst(std::get_if<Insert_Instruction>(&instruction)); inst) {
					// We only care about repeats larger than 36 bytes.
					if (inst->newData.size() > 36ull) {
						// Upper limit (mx and my) reduced by 36, since we only care about matches that exceed 36 bytes
						size_t max = std::min<size_t>(inst->newData.size(), inst->newData.size() - 37ull);
						for (size_t x = 0ull; x < max; ++x) {
							const auto & value_at_x = inst->newData[x];
							if (inst->newData[x + 36ull] != value_at_x)
								continue; // quit early if the value 36 units away isn't the same as this index

							size_t y = x + 1;
							while (y < max) {
								if (value_at_x == inst->newData[y])
									y++;
								else
									break;
							}

							const auto length = y - x;
							if (length > 36ull) {
								// Worthwhile to insert a new instruction
								// Keep data up-until region where repeats occur
								Insert_Instruction instBefore;
								instBefore.index = inst->index;
								instBefore.newData.resize(x);
								std::memcpy(instBefore.newData.data(), inst->newData.data(), x);

								// Generate new Repeat Instruction
								Repeat_Instruction instRepeat;
								instRepeat.index = inst->index + x;
								instRepeat.value = value_at_x;
								instRepeat.amount = length;

								// Modifying instructions vector
								std::unique_lock<std::mutex> writeGuard(instructionMutex);
								instructions.push_back(instBefore);
								instructions.push_back(instRepeat);
								// Modify original insert-instruction to contain remainder of the data
								inst->index = inst->index + x + length;
								std::memmove(&inst->newData[0], &inst->newData[y], inst->newData.size() - y);
								inst->newData.resize(inst->newData.size() - y);
								writeGuard.unlock();
								writeGuard.release();

								x = ULLONG_MAX; // require overflow, because we want next itteration for x == 0
								max = std::min<size_t>(inst->newData.size(), inst->newData.size() - 37ull);
								break;
							}
							x = y - 1;
							break;
						}
					}
				}
			});
		}

		// Wait for jobs to finish
		threader.prepareForShutdown();
		while (!threader.isFinished())
			continue;
		threader.shutdown();

		// Create a buffer to contain all the diff instructions
		size_t size_patch(0ull);
		constexpr auto CallSize = [](const auto & obj) { return obj.SIZE(); };
		for each (const auto & instruction in instructions)
			size_patch += std::visit(CallSize, instruction);
		Buffer patchBuffer(size_patch);

		// Write instruction data to the buffer
		size_t index(0ull);
		const auto CallWrite = [&patchBuffer, &index](const auto & obj) { obj.WRITE(patchBuffer, index); };
		for (auto & instruction : instructions)
			std::visit(CallWrite, instruction);

		// Free up memory
		instructions.clear();
		instructions.shrink_to_fit();

		// Try to compress the diff buffer
		auto compressedPatchBuffer = patchBuffer.compress();
		patchBuffer.release();
		if (compressedPatchBuffer) {
			// We now want to prepend a header
			// Calculate the size of the header, then generate a DECORATED compressed diff buffer
			Buffer diffBuffer(DBUFFER_H_SIZE + compressedPatchBuffer->size());

			// Write Header Information
			auto byteIndex = diffBuffer.writeData(&DBUFFER_H_TITLE[0], DBUFFER_H_TITLE_SIZE);
			byteIndex = diffBuffer.writeData(&size_new, size_t(sizeof(size_t)), byteIndex);

			// Write compressed data over
			byteIndex = diffBuffer.writeData(compressedPatchBuffer->data(), compressedPatchBuffer->size(), byteIndex);
			compressedPatchBuffer->release();

			// Success
			return diffBuffer;
		}
	}

	// Failure
	return {};
}

std::optional<Buffer> Buffer::patch(const Buffer & diffBuffer) const
{
	// Ensure diff buffer at least *exists* (ignore old buffer, when empty we treat instruction as a brand new file)
	if (diffBuffer.hasData()) {
		// Read in diffBuffer's header title, ensure header matches
		char headerTitle_In[DBUFFER_H_TITLE_SIZE];
		auto index = diffBuffer.readData(&headerTitle_In, DBUFFER_H_TITLE_SIZE);
		if (std::strcmp(headerTitle_In, DBUFFER_H_TITLE) == 0) {
			// Get uncompressed size
			size_t newSize(0ull);
			index = diffBuffer.readData(&newSize, size_t(sizeof(size_t)), index);

			// Try to decompress the diff buffer
			/////////////////////////////////////////////////
			const size_t DATA_SIZE = diffBuffer.size() - DBUFFER_H_SIZE;
			Buffer DELETE_ME(&diffBuffer[index], DATA_SIZE);
			/////////////////////////////////////////////////
			auto diffInstructionBuffer = DELETE_ME.decompress();
			DELETE_ME.release();
			if (diffInstructionBuffer) {
				// Convert buffer into instructions
				Threader threader;
				Buffer bufferNew(newSize);
				size_t bytesRead(0ull), byteIndex(0ull);
				constexpr auto CallSize = [](const auto & obj) { return obj.SIZE(); };
				const auto CallDo = [&](const auto & obj) { return obj.DO(bufferNew, *this); };
				while (bytesRead < diffInstructionBuffer->size()) {
					// Make the instruction, reading it in from memory
					const auto & instruction = Instruction_Maker::Make(*diffInstructionBuffer, byteIndex);
					// Read the instruction size
					bytesRead += std::visit(CallSize, instruction);
					threader.addJob([instruction, &CallDo]() {
						// Execute the instruction on a separate thread
						std::visit(CallDo, instruction);
					});
				}

				// Wait for jobs to finish, then prepare to end
				threader.prepareForShutdown();
				while (!threader.isFinished())
					continue;
				threader.shutdown();
				diffInstructionBuffer->release();

				// Success
				return bufferNew;
			}
		}
	}

	// Failure
	return {};
}


// Public Accessors

bool Buffer::hasData() const
{
	return !m_data.empty();
}

size_t Buffer::size() const
{
	return m_data.size();
}

const size_t Buffer::hash() const
{
	// Use data 8-bytes at a time, until end of data or less than 8 bytes remains
	size_t value(1234567890ull);
	auto pointer = reinterpret_cast<size_t*>(const_cast<std::byte*>(&m_data[0]));
	size_t x(0ull), full(m_data.size()), max(full / 8ull);
	for (; x < max; ++x)
		value = ((value << 5) + value) + pointer[x]; // use 8 bytes

	// If any bytes remain, switch technique to work byte-wise instead of 8-byte-wise
	x *= 8ull;
	auto remainderPtr = reinterpret_cast<char*>(const_cast<std::byte*>(&m_data[0]));
	for (; x < full; ++x)		
		value = ((value << 5) + value) + remainderPtr[x]; // use remaining bytes

	return value;
}

size_t Buffer::readData(void * outputPtr, const size_t & size, const size_t byteOffset) const
{
	std::memcpy(outputPtr, &m_data[byteOffset], size);
	return byteOffset + size;
}


// Public Modifiers

size_t Buffer::writeData(const void * inputPtr, const size_t & size, const size_t byteOffset)
{
	std::memcpy(&m_data[byteOffset], inputPtr, size);
	return byteOffset + size;
}

char * Buffer::cArray() const
{
	return reinterpret_cast<char*>(const_cast<std::byte*>(m_data.data()));
}

std::byte * Buffer::data() const
{
	return const_cast<std::byte*>(m_data.data());
}

std::byte & Buffer::operator[](const size_t & byteOffset) const
{
	return const_cast<std::byte&>(m_data[byteOffset]);
}

void Buffer::resize(const size_t & size)
{
	m_data.resize(size);
}

void Buffer::release()
{
	m_data.clear();
	m_data.shrink_to_fit();
}
