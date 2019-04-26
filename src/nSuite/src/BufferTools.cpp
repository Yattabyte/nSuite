#include "BufferTools.h"
#include "Instructions.h"
#include "Threader.h"
#include "lz4.h"
#include <algorithm>
#define BUFFER_HEADER_TEXT "nSuite cbuffer"


bool BFT::CompressBuffer(char * sourceBuffer, const size_t & sourceSize, char ** destinationBuffer, size_t & destinationSize)
{
	// Pre-allocate a huge buffer to allow for compression OPS
	char * compressedBuffer = new char[sourceSize * 2ull];

	// Try to compress the source buffer
	if (auto compressedSize = LZ4_compress_default(
		sourceBuffer,
		compressedBuffer,
		int(sourceSize),
		int(sourceSize * 2ull)
	)) {
		// We now know the actual compressed buffer size
		// Calculate the size of the header, then generate a DECORATED compressed buffer
		constexpr const char HEADER_TITLE[] = BUFFER_HEADER_TEXT;
		constexpr const size_t HEADER_TITLE_SIZE = (sizeof(BUFFER_HEADER_TEXT) / sizeof(*BUFFER_HEADER_TEXT));
		constexpr const size_t HEADER_SIZE = size_t(HEADER_TITLE_SIZE + sizeof(size_t));
		destinationSize = HEADER_SIZE + size_t(compressedSize);
		*destinationBuffer = new char[destinationSize];
		char * HEADER_ADDRESS = *destinationBuffer;
		char * DATA_ADDRESS = HEADER_ADDRESS + HEADER_SIZE;

		// Write header information
		std::memcpy(HEADER_ADDRESS, &HEADER_TITLE[0], HEADER_TITLE_SIZE);
		HEADER_ADDRESS = reinterpret_cast<char*>(PTR_ADD(HEADER_ADDRESS, HEADER_TITLE_SIZE));
		std::memcpy(HEADER_ADDRESS, &sourceSize, size_t(sizeof(size_t)));

		// Copy compressed data over
		std::memcpy(DATA_ADDRESS, compressedBuffer, size_t(compressedSize));
		delete[] compressedBuffer;

		return true;
	}
	return false;
}

bool BFT::DecompressBuffer(char * sourceBuffer, const size_t & sourceSize, char ** destinationBuffer, size_t & destinationSize)
{
	// Ensure buffer at least *exists*
	if (sourceSize <= size_t(sizeof(size_t)) || sourceBuffer == nullptr)
		return false;

	// Expected header information
	constexpr const char HEADER_TITLE[] = BUFFER_HEADER_TEXT;
	constexpr const size_t HEADER_TITLE_SIZE = (sizeof(BUFFER_HEADER_TEXT) / sizeof(*BUFFER_HEADER_TEXT));
	constexpr const size_t HEADER_SIZE = size_t(HEADER_TITLE_SIZE + sizeof(size_t));
	char headerTitle_In[HEADER_TITLE_SIZE];

	// Read in the header title of this buffer, ensure it matches
	char * HEADER_ADDRESS = sourceBuffer;
	std::memcpy(headerTitle_In, HEADER_ADDRESS, HEADER_TITLE_SIZE);
	if (std::strcmp(headerTitle_In, HEADER_TITLE) == 0) {
		// Get data size
		HEADER_ADDRESS = reinterpret_cast<char*>(PTR_ADD(HEADER_ADDRESS, HEADER_TITLE_SIZE));
		destinationSize = *reinterpret_cast<size_t*>(HEADER_ADDRESS);
		*destinationBuffer = new char[destinationSize];
		
		// Get data
		const char * DATA_ADDRESS = reinterpret_cast<char*>(PTR_ADD(HEADER_ADDRESS, size_t(sizeof(size_t))));
		return (LZ4_decompress_safe(
			DATA_ADDRESS,
			reinterpret_cast<char*>(*destinationBuffer),
			int(sourceSize - HEADER_SIZE),
			int(destinationSize)
		) > 0);
	}

	// Unexpected buffer header
	return false;
}

bool BFT::DiffBuffers(char * buffer_old, const size_t & size_old, char * buffer_new, const size_t & size_new, char ** buffer_diff, size_t & size_diff, size_t * instructionCount)
{
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
		threader.addJob([&instructions, &instructionMutex, &buffer_old, &buffer_new, windowSize, &size_old, &size_new, bytesUsed_old, bytesUsed_new]() {
			// Step 1: Find all regions that match
			struct MatchInfo {
				size_t length = 0ull, start1 = 0ull, start2 = 0ull;
			};
			size_t bestContinuous(0ull), bestMatchCount(windowSize);
			std::vector<MatchInfo> bestSeries;
			auto buffer_slice_old = reinterpret_cast<std::byte*>(BFT::PTR_ADD(buffer_old, bytesUsed_old));
			auto buffer_slice_new = reinterpret_cast<std::byte*>(BFT::PTR_ADD(buffer_new, bytesUsed_new));
			for (size_t index1 = 0ull; index1 + 8ull < windowSize; index1 += 8ull) {
				const size_t OLD_FIRST8 = *reinterpret_cast<size_t*>(&buffer_slice_old[index1]);
				std::vector<MatchInfo> matches;
				size_t largestContinuous(0ull);
				for (size_t index2 = 0ull; index2 + 8ull < windowSize; index2+= 8ull) {
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
						std::memcpy(inst.newData.data(), &buffer_new[lastMatchEnd], newDataLength);
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
					std::memcpy(inst.newData.data(), &buffer_new[lastMatchEnd], newDataLength);
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
		std::memcpy(inst.newData.data(), &buffer_new[bytesUsed_new], size_new - bytesUsed_new);
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
	if (instructionCount != nullptr)
		*instructionCount += instructions.size();

	// Create buffer for the patch file, writing all instruction data into it
	size_t size_patch(0ull);
	constexpr auto CallSize = [](const auto & obj) { return obj.SIZE(); };
	for each (const auto & instruction in instructions)
		size_patch += std::visit(CallSize, instruction);
	size_patch += sizeof(size_t); // size increased b/c first bytes == source file size
	char * buffer_patch = new char[size_patch];
	void * writingPtr = buffer_patch;
	// Write-out source file size
	std::memcpy(writingPtr, &size_new, sizeof(size_t));
	writingPtr = BFT::PTR_ADD(writingPtr, sizeof(size_t));
	// Write-out instructions
	const auto CallWrite = [&writingPtr](const auto & obj) { obj.WRITE(&writingPtr); };
	for (auto & instruction : instructions) 
		std::visit(CallWrite, instruction);		

	// Attempt to compress the buffer
	bool CompressResult = BFT::CompressBuffer(buffer_patch, size_patch, buffer_diff, size_diff);
	delete[] buffer_patch;
	return CompressResult;
}

bool BFT::PatchBuffer(char * buffer_old, const size_t & size_old, char ** buffer_new, size_t & size_new, char * buffer_diff, const size_t & size_diff, size_t * instructionCount)
{
	// Attempt to decompress the buffer
	Threader threader;
	char * buffer_diff_full(nullptr);
	size_t size_diff_full(0ull);
	if (BFT::DecompressBuffer(buffer_diff, size_diff, &buffer_diff_full, size_diff_full)) {
		// Convert buffer into instructions
		size_t bytesRead(0ull);
		void * readingPtr = buffer_diff_full;
		size_new = *reinterpret_cast<size_t*>(readingPtr);
		readingPtr = BFT::PTR_ADD(readingPtr, size_t(sizeof(size_t)));
		bytesRead += sizeof(size_t);
		*buffer_new = new char[size_new];
		constexpr auto CallSize = [](const auto & obj) { return obj.SIZE(); };
		const auto CallDo = [&buffer_new, &size_new, &buffer_old, size_old](const auto & obj) { return obj.DO(*buffer_new, size_new, buffer_old, size_old); };
		size_t count(0ull);
		while (bytesRead < size_diff_full) {			
			// Make the instruction, reading it in from memory
			const auto & instruction = Instruction_Maker::Make(&readingPtr);
			// Read the instruction size
			bytesRead += std::visit(CallSize, instruction);
			threader.addJob([instruction, &CallDo]() {
				// Execute the instruction on a separate thread
				std::visit(CallDo, instruction);
			});
			count++;
		}

		// Wait for jobs to finish, then prepare to end
		threader.prepareForShutdown();
		while (!threader.isFinished())
			continue;
		threader.shutdown();
		delete[] buffer_diff_full;

		if (instructionCount != nullptr)
			*instructionCount += count;
		return true;
	}
	return false;
}

size_t BFT::HashBuffer(char * buffer, const size_t & size)
{
	size_t value(1234567890ull);
	auto pointer = reinterpret_cast<size_t*>(buffer);

	size_t x = 0ull, max = size / 8ull;
	for (; x < max; ++x)
		value = ((value << 5) + value) + pointer[x];

	// in case size isn't a multiple of 8, data will be leftover
	x *= 8ull; // modify index so we can compare byte-wise
	for (; x < size; ++x)
		value = ((value << 5) + value) + buffer[x]; // compare remaining bytes

	return value;
}

void * BFT::PTR_ADD(void * const ptr, const size_t & offset)
{
	return static_cast<std::byte*>(ptr) + offset;
}
