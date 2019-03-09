#include "buffer_tools.h"
#include "threader.h"
#include "lz4.h"
#include <algorithm>
#include <vector>


/** Increment a pointer's address by the offset provided.
@param	ptr			the pointer to increment by the offset amount.
@param	offset		the offset amount to apply to the pointer's address.
@return				the modified pointer address. */
static void * INCR_PTR(void *const ptr, const size_t & offset)
{
	return (void*)(reinterpret_cast<unsigned char*>(ptr) + offset);
};

bool BFT::CompressBuffer(char * sourceBuffer, const size_t & sourceSize, char ** destinationBuffer, size_t & destinationSize)
{
	// Allocate enough room for the compressed buffer
	*destinationBuffer = new char[sourceSize + sizeof(size_t)];

	// First chunk of data = the total uncompressed size
	*reinterpret_cast<size_t*>(*destinationBuffer) = sourceSize;

	// Increment pointer so that the compression works on the remaining part of the buffer
	*destinationBuffer = reinterpret_cast<char*>(*destinationBuffer) + size_t(sizeof(size_t));

	// Compress the buffer
	auto result = LZ4_compress_default(
		sourceBuffer,
		*destinationBuffer,
		int(sourceSize),
		int(sourceSize)
	);

	// Decrement pointer
	*destinationBuffer = reinterpret_cast<char*>(*destinationBuffer) - size_t(sizeof(size_t));
	destinationSize = size_t(result) + sizeof(size_t);
	return (result > 0);
}

bool BFT::DecompressBuffer(char * sourceBuffer, const size_t & sourceSize, char ** destinationBuffer, size_t & destinationSize)
{
	destinationSize = *reinterpret_cast<size_t*>(sourceBuffer);
	*destinationBuffer = new char[destinationSize];
	auto result = LZ4_decompress_safe(
		reinterpret_cast<char*>(reinterpret_cast<unsigned char*>(sourceBuffer) + size_t(sizeof(size_t))),
		reinterpret_cast<char*>(*destinationBuffer),
		int(sourceSize - size_t(sizeof(size_t))),
		int(destinationSize)
	);

	return (result > 0);
}

bool BFT::DiffBuffers(char * buffer_old, const size_t & size_old, char * buffer_new, const size_t & size_new, char ** buffer_diff, size_t & size_diff, size_t * instructionCount)
{
	struct Instruction {
		size_t beginKeep = 0ull, endKeep = 0ull;	// range to keep
		size_t beginNew = 0ull;						// spot to insert
		std::vector<char> newData;					// data to insert
	};
	std::vector<Instruction> instructions;
	std::mutex instructionMutex;
	Threader threader;
	std::atomic_size_t jobsStarted(0ull), jobsFinished(0ull);
	constexpr size_t amount(2048ull);
	size_t bytesUsed_old(0ull), bytesUsed_new(0ull);
	while (bytesUsed_old < size_old && bytesUsed_new < size_new) {
		// Variables for this current chunk
		auto buffer_slice_old = reinterpret_cast<char*>(INCR_PTR(buffer_old, bytesUsed_old));
		auto buffer_slice_new = reinterpret_cast<char*>(INCR_PTR(buffer_new, bytesUsed_new));
		auto windowSize = std::min<size_t>(amount, std::min<size_t>(size_old - bytesUsed_old, size_new - bytesUsed_new));

		// Find best matches for this chunk
		threader.addJob([&jobsFinished, &instructions, &instructionMutex, buffer_slice_old, buffer_slice_new, windowSize, bytesUsed_old, bytesUsed_new]() {
			// Step 1: Find all regions that match
			struct MatchInfo {
				size_t length = 0ull, start1 = 0ull, start2 = 0ull;
			};
			size_t bestTotal(0ull), bestContinuous(0ull), bestMatchCount(windowSize);
			std::vector<MatchInfo> bestSeries;
			for (size_t index1 = 0ull; index1 < windowSize; ++index1) {
				std::vector<MatchInfo> matches;
				size_t totalUnitsMatched(0ull), largestContinuous(0ull);
				for (size_t index2 = 0ull; index2 < windowSize; ++index2) {
					// Check if values match	
					if (buffer_slice_old[index1] == buffer_slice_new[index2]) {
						// Check how long this series of matches continues for
						size_t offset = 1ull;
						for (; ((index1 + offset) < windowSize) && ((index2 + offset) < windowSize); ++offset)
							if (buffer_slice_old[index1 + offset] != buffer_slice_new[index2 + offset])
								break;

						// Save series
						matches.emplace_back(MatchInfo{ offset, index1, index2 });

						if (offset > largestContinuous)
							largestContinuous = offset;
						totalUnitsMatched += offset;

						index2 += offset;
					}
				}
				// Check if the recently completed series saved the most data in the most linear fashion
				if (totalUnitsMatched > bestTotal && largestContinuous > bestContinuous && matches.size() <= bestMatchCount) {
					// Save the series for later
					bestTotal = totalUnitsMatched;
					bestContinuous = largestContinuous;
					bestMatchCount = matches.size();
					bestSeries = matches;
				}

				// Optimization: if the largest continous series found == window size, then break, because these 2 windows/chunks match
				if (largestContinuous >= windowSize)
					break;
			}

			// Step 2: Generate instructions based on the matching-regions found
			// Note: 
			//			data from [start1 -> +length] == [start2 -> + length]
			//			data before and after these ranges =/=
			if (!bestSeries.size()) {
				// No Matches, copy entire window
				Instruction inst;
				inst.beginNew = bytesUsed_new;
				inst.newData.resize(windowSize);
				for (size_t x = 0; x < windowSize; ++x)
					inst.newData[x] = buffer_slice_new[x];
				std::unique_lock<std::mutex> writeGuard(instructionMutex);
				instructions.push_back(inst);
			}
			else {
				size_t lastMatchEnd(0ull);
				for (size_t mx = 0, ms = bestSeries.size(); mx < ms; ++mx) {
					const auto & match = bestSeries[mx];

					// Copy data block at end of last matching region -> beginning of current matching region
					const auto newDataLength = match.start2 - lastMatchEnd;
					if (newDataLength > 0ull) {
						Instruction inst;
						inst.beginNew = bytesUsed_new + lastMatchEnd;
						inst.newData.resize(newDataLength);
						for (size_t inst_index = 0, buffer_index = lastMatchEnd; inst_index < newDataLength; ++inst_index, ++buffer_index)
							inst.newData[inst_index] = buffer_slice_new[buffer_index];
						std::unique_lock<std::mutex> writeGuard(instructionMutex);
						instructions.push_back(inst);
					}

					// Retain data region for this current match
					std::unique_lock<std::mutex> writeGuard(instructionMutex);
					instructions.emplace_back(Instruction{ bytesUsed_old + match.start1, bytesUsed_old + match.start1 + match.length, bytesUsed_new + match.start2 });
					lastMatchEnd = match.start2 + match.length;
				}

				// Copy data block at end of last matching region -> end of window
				const auto newDataLength = windowSize - lastMatchEnd;
				if (newDataLength > 0ull) {
					Instruction inst;
					inst.beginNew = bytesUsed_new + lastMatchEnd;
					inst.newData.resize(newDataLength);
					for (size_t inst_index = 0, buffer_index = lastMatchEnd; inst_index < newDataLength; ++inst_index, ++buffer_index)
						inst.newData[inst_index] = buffer_slice_new[buffer_index];
					std::unique_lock<std::mutex> writeGuard(instructionMutex);
					instructions.push_back(inst);
				}
			}
			jobsFinished++;
		});
		// increment
		bytesUsed_old += windowSize;
		bytesUsed_new += windowSize;
		jobsStarted++;
	}

	// If we haven't finished reading from buffer_new, append whatever data is left-over
	if (bytesUsed_new < size_new) {
		Instruction instruction;
		instruction.beginNew = bytesUsed_new;
		instruction.newData.resize(size_new - bytesUsed_new);
		for (size_t a = bytesUsed_new, dataIndex = 0; a < size_new; ++a, ++dataIndex)
			instruction.newData[dataIndex] = buffer_new[a];

		std::unique_lock<std::mutex> writeGuard(instructionMutex);
		instructions.push_back(instruction);
	}

	while (jobsStarted != jobsFinished)
		continue;
	threader.shutdown();
	if (instructionCount != nullptr)
		*instructionCount = instructions.size();

	// Create buffer for the patch file, writing all instruction data into it
	size_t size_patch(0ull);
	for each (const auto & instruction in instructions)
		size_patch += size_t((sizeof(size_t) * 4ull) + (sizeof(char) * instruction.newData.size()));
	size_patch += sizeof(size_t);// size increased b/c first bytes == source file size
	char * buffer_patch = new char[size_patch];
	void * writingPtr = buffer_patch;
	// Write-out source file size
	memcpy(writingPtr, reinterpret_cast<char*>(&const_cast<size_t&>(size_new)), sizeof(size_t));
	writingPtr = INCR_PTR(writingPtr, sizeof(size_t));
	// Write-out instructions
	for (auto instruction : instructions) {
		memcpy(writingPtr, reinterpret_cast<char*>(&instruction.beginKeep), sizeof(size_t));
		writingPtr = INCR_PTR(writingPtr, sizeof(size_t));

		memcpy(writingPtr, reinterpret_cast<char*>(&instruction.endKeep), sizeof(size_t));
		writingPtr = INCR_PTR(writingPtr, sizeof(size_t));

		memcpy(writingPtr, reinterpret_cast<char*>(&instruction.beginNew), sizeof(size_t));
		writingPtr = INCR_PTR(writingPtr, sizeof(size_t));

		auto length = instruction.newData.size();
		memcpy(writingPtr, reinterpret_cast<char*>(&length), sizeof(size_t));
		writingPtr = INCR_PTR(writingPtr, sizeof(size_t));

		memcpy(writingPtr, instruction.newData.data(), length);
		writingPtr = INCR_PTR(writingPtr, length);
	}

	// Attempt to compress the buffer
	bool CompressResult = BFT::CompressBuffer(buffer_patch, size_patch, buffer_diff, size_diff);
	delete[] buffer_patch;
	return CompressResult;
}

bool BFT::PatchBuffer(char * buffer_old, const size_t & size_old, char ** buffer_new, size_t & size_new, char * buffer_diff, const size_t & size_diff, size_t * instructionCount)
{
	// Attempt to decompress the buffer
	char * buffer_diff_full(nullptr);
	size_t size_diff_full(0ull);
	if (BFT::DecompressBuffer(buffer_diff, size_diff, &buffer_diff_full, size_diff_full)) {
		struct Instruction {
			size_t beginKeep = 0ull, endKeep = 0ull;	// range to keep
			size_t beginNew = 0ull;						// spot to insert
			std::vector<char> newData;					// data to insert
		};
		std::vector<Instruction> instructions;

		// Convert buffer into instructions
		size_t bytesRead(0ull);
		void * readingPtr = buffer_diff_full;
		size_new = *reinterpret_cast<size_t*>(readingPtr);
		readingPtr = INCR_PTR(readingPtr, size_t(sizeof(size_t)));
		bytesRead += sizeof(size_t);
		while (bytesRead < size_diff_full) {
			Instruction inst;
			inst.beginKeep = *reinterpret_cast<size_t*>(readingPtr);
			readingPtr = INCR_PTR(readingPtr, sizeof(size_t));

			inst.endKeep = *reinterpret_cast<size_t*>(readingPtr);
			readingPtr = INCR_PTR(readingPtr, sizeof(size_t));

			inst.beginNew = *reinterpret_cast<size_t*>(readingPtr);
			readingPtr = INCR_PTR(readingPtr, sizeof(size_t));

			size_t length = *reinterpret_cast<size_t*>(readingPtr);
			readingPtr = INCR_PTR(readingPtr, sizeof(size_t));
			if (length) {
				auto charArray = reinterpret_cast<char*>(readingPtr);
				inst.newData.resize(length);
				for (size_t x = 0; x < length; ++x)
					inst.newData[x] = charArray[x];
				readingPtr = INCR_PTR(readingPtr, length);
			}
			instructions.push_back(inst);
			bytesRead += (size_t(sizeof(size_t)) * 4ull) + (sizeof(char) * length);
		}
		delete[] buffer_diff_full;

		// Reconstruct new buffer from buffer_old + changes
		* buffer_new = new char[size_new];
		for each (const auto & change in instructions) {
			auto index3 = change.beginNew;
			for (auto x = change.beginKeep; x < change.endKeep && x < size_old; ++x) 
				(*buffer_new)[index3++] = buffer_old[x];			
			for each (const auto & value in change.newData)
				(*buffer_new)[index3++] = value;
		}
		if (instructionCount != nullptr)
			*instructionCount = instructions.size();
		return true;
	}
	return false;
}