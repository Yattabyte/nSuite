#include "BufferTools.h"
#include "Instructions.h"
#include "Threader.h"
#include "lz4.h"
#include <algorithm>


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
	std::vector<InstructionTypes> instructions;
	instructions.reserve(std::max(size_old, size_new) / 8ull);
	std::mutex instructionMutex;
	Threader threader;
	std::atomic_size_t jobsStarted(0ull), jobsFinished(0ull);
	constexpr size_t amount(4096);
	size_t bytesUsed_old(0ull), bytesUsed_new(0ull);
	while (bytesUsed_old < size_old && bytesUsed_new < size_new) {
		// Variables for this current chunk
		auto windowSize = std::min<size_t>(amount, std::min<size_t>(size_old - bytesUsed_old, size_new - bytesUsed_new));

		// Find best matches for this chunk
		threader.addJob([&jobsFinished, &instructions, &instructionMutex, &buffer_old, &buffer_new, windowSize, &size_old, &size_new, bytesUsed_old, bytesUsed_new]() {
			// Step 1: Find all regions that match
			struct MatchInfo {
				size_t length = 0ull, start1 = 0ull, start2 = 0ull;
			};
			size_t bestContinuous(0ull), bestMatchCount(windowSize);
			std::vector<MatchInfo> bestSeries;
			auto buffer_slice_old = reinterpret_cast<std::byte*>(PTR_ADD(buffer_old, bytesUsed_old));
			auto buffer_slice_new = reinterpret_cast<std::byte*>(PTR_ADD(buffer_new, bytesUsed_new));
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
			jobsFinished++;
		});
		// increment
		bytesUsed_old += windowSize;
		bytesUsed_new += windowSize;
		jobsStarted++;
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
	while (jobsStarted != jobsFinished)
		continue;

	// Test replacing insertions with some repeat instructions
	for (size_t i = 0; i < instructions.size(); ++i) {
		threader.addJob([i, &instructions,  &instructionMutex, &jobsFinished]() {
			auto & instruction = instructions[i];
			if (const auto inst(std::get_if<Insert_Instruction>(&instruction)); inst) {
				for (size_t x = 0ull, mx = inst->newData.size(); x < mx; ++x) {
					const auto & value_at_x = inst->newData[x];
					for (size_t y = x + 1, my = inst->newData.size(); y < my; ++y) {
						if (value_at_x != inst->newData[y]) {
							const auto length = y - x;
							if (length > 36ull) {
								// Worthwhile to insert a new instruction
								// Keep data up-until region where repeats occur
								std::unique_lock<std::mutex> writeGuard(instructionMutex);
								Insert_Instruction instBefore;
								instBefore.index = inst->index;
								instBefore.newData.resize(x);
								std::memcpy(instBefore.newData.data(), inst->newData.data(), x);
								instructions.push_back(instBefore);
								// Generate new Repeat Instruction
								Repeat_Instruction instRepeat;
								instRepeat.index = inst->index + x;
								instRepeat.value = value_at_x;
								instRepeat.amount = length;
								instructions.push_back(instRepeat);
								// Modify original insert-instruction to contain remainder of the data
								inst->index = inst->index + x + length;
								std::memmove(&inst->newData[0], &inst->newData[y], inst->newData.size() - y);
								inst->newData.resize(inst->newData.size() - y);
								x = ULLONG_MAX; // require overflow, because we want next itteration for x == 0
								break;
							}
							x = y - 1;
							break;
						}
					}
				}
			}
			jobsFinished++;
		});
		jobsStarted++;		
	}

	// Wait for jobs to finish
	while (jobsStarted != jobsFinished)
		continue;
	threader.shutdown();
	if (instructionCount != nullptr)
		*instructionCount = instructions.size();

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
	writingPtr = PTR_ADD(writingPtr, sizeof(size_t));
	// Write-out instructions
	const auto CallWrite = [&writingPtr](const auto & obj) { obj.WRITE(&writingPtr); };
	for (auto & instruction : instructions) 
		std::visit(CallWrite, instruction);		

	// Attempt to compress the buffer
	bool CompressResult = BFT::CompressBuffer(buffer_patch, size_patch, buffer_diff, size_diff);
	delete[] buffer_patch;
	return CompressResult;
	return true;
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
		readingPtr = PTR_ADD(readingPtr, size_t(sizeof(size_t)));
		bytesRead += sizeof(size_t);
		*buffer_new = new char[size_new];
		constexpr auto CallSize = [](const auto & obj) { return obj.SIZE(); };
		const auto CallDo = [&buffer_new, &size_new, &buffer_old, size_old](const auto & obj) { return obj.DO(*buffer_new, size_new, buffer_old, size_old); };
		std::atomic_size_t jobsStarted(0ull), jobsFinished(0ull);
		while (bytesRead < size_diff_full) {			
			// Make the instruction, reading it in from memory
			auto instruction = Instruction_Maker::Make(&readingPtr);
			// Read the instruction size
			bytesRead += std::visit(CallSize, instruction);
			threader.addJob([&jobsFinished, instruction, &CallDo]() {
				// Execute the instruction on a separate thread
				std::visit(CallDo, instruction);
				jobsFinished++;
			});
			jobsStarted++;
		}

		// Wait for jobs to finish, then prepare to end
		while (jobsStarted != jobsFinished)
			continue;
		threader.shutdown();
		delete[] buffer_diff_full;

		if (instructionCount != nullptr)
			*instructionCount = jobsFinished;
		return true;
	}
	return false;
}