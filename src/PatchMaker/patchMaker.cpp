#include "Archiver.h"
#include <chrono>
#include <direct.h>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>


/** Attempts to find the next best matching point between 2 buffers, starting and the indices provided.
@param	buffer1		the first buffer.
@param	buffer2		the second buffer.
@param	size1		the size of the first buffer.
@param	size2		the size of the second buffer.
@param	index1		the starting index for buffer1, updated with next best match index.
@param	index2		the starting index for buffer2, updated with next best match index.
@return				true if a match can be found, false otherwise. */
static bool find_next_best_match(char * const buffer1, char * const buffer2, const size_t & size1, const size_t & size2, size_t & index1, size_t & index2, size_t & matchLength)
{
	struct MatchInfo {
		size_t length = 0ull, start1 = 0ull, start2 = 0ull;
	} bestMatch;
	auto end1 = size1, end2 = size2;
	const auto start2 = index2;
	for (; index1 < end1; ++index1) {		
		for (index2 = start2; index2 < end2; ++index2) {
			// Check if values match			
			if (buffer1[index1] == buffer2[index2]) {
				// Optimization step - we want the longest series, so we can bail early if this isn't true
				if (buffer1[index1 + bestMatch.length] == buffer2[index2 + bestMatch.length]) {
					// Check how long this series of matches continues for
					size_t offset = 1ull;
					for (; ((index1 + offset) < end1) && ((index2 + offset) < end2); ++offset)
						if (buffer1[index1 + offset] != buffer2[index2 + offset])
							break;

					if (offset > bestMatch.length) {
						bestMatch = MatchInfo{ offset, index1, index2 };

						// Reduce end amount by best match length
						// avoid checking for matches that can't be any longer
						end1 = size1 - bestMatch.length;
						end2 = size2 - bestMatch.length;
					}
				}
				break;
			}
		}
	}	
	index1 = bestMatch.start1;
	index2 = bestMatch.start2;
	matchLength = bestMatch.length;
	return bool(bestMatch.length > 0ull);
}

/** Increment a pointer's address by the offset provided.
@param	ptr			the pointer to increment by the offset amount.
@param	offset		the offset amount to apply to the pointer's address.
@return				the modified pointer address. */
static void * INCR_PTR(void *const ptr, const size_t & offset)
{
	return (void*)(reinterpret_cast<unsigned char*>(ptr) + offset);
};

/** Entry point. */
int main()
{
	// Get the running directory
	char cCurrentPath[FILENAME_MAX];
	if (_getcwd(cCurrentPath, sizeof(cCurrentPath)))
		cCurrentPath[sizeof(cCurrentPath) - 1ull] = char('\0');
	const auto directory = std::string(cCurrentPath);

	// Get the proper path names, and read the source files into their appropriate buffers.
	const auto start = std::chrono::system_clock::now();
	const auto path_old = directory + "\\old.exe", path_new = directory + "\\new.exe", path_patch = directory + "\\file.patch";
	const auto size_old = std::filesystem::file_size(path_old), size_new = std::filesystem::file_size(path_new);
	std::ifstream file_old(path_old, std::ios::binary | std::ios::beg), file_new(path_new, std::ios::binary | std::ios::beg);
	char * buffer_old = new char[size_old], * buffer_new = new char[size_new];
	if (file_old.is_open() && file_new.is_open()) {
		file_old.read(buffer_old, std::streamsize(size_old));
		file_new.read(buffer_new, std::streamsize(size_new));
		file_old.close();
		file_new.close();
	}
	
	struct Instruction {
		size_t beginKeep = 0ull, endKeep = 0ull;	// range to keep
		size_t beginNew = 0ull;						// spot to insert
		std::vector<char> newData;					// data to insert
	};
	std::vector<Instruction> instructions;
	size_t index1(0ull), index2(0ull);
	// Keep processing data in both buffers, as long as there is data left in both of them
	while (index1 < size_old && index2 < size_new) {
		// Check if the data in both buffers is different
		if (buffer_old[index1] != buffer_new[index2]) {
			// Find the next best point where these 2 buffers match
			auto index1_Next = index1, index2_Next = index2, matchLength = size_t(0ull); 
			find_next_best_match(buffer_old, buffer_new, size_old, size_new, index1_Next, index2_Next, matchLength);

			// Between this and that index, buffer_old data is deleted and buffer_new data is inserted
			if (index2_Next > index2) {
				Instruction instruction;
				instruction.beginNew = index2;
				instruction.newData.resize(index2_Next - index2);
				for (size_t a = index2, dataIndex = 0; a < index2_Next; ++a, ++dataIndex)
					instruction.newData[dataIndex] = buffer_new[a];
				instructions.push_back(instruction);

				// While finding the next best match, we've determined the next range of 'keep' instructions
				// Create it's instruction now and skip ahead by that amount, avoiding redundant comparisons
				instructions.emplace_back(Instruction{ index1_Next, index1_Next + matchLength, index2_Next });
				index2 = index2_Next + matchLength;
				index1 = index1_Next + matchLength;
				continue;
			}
		}

		// Both buffers match at index1 and index2
		if (index1 + 1 < size_old) {
			// Merge all subsequent 'keep' instructions together
			if (instructions.size() && instructions.back().endKeep > 0ull) 
				instructions.back().endKeep = index1 + 1;
			else
				instructions.emplace_back(Instruction{ index1, index1 + 1, index2 });
		}
		index1++;
		index2++;
	}
	// If we haven't finished reading from buffer_new, append whatever data is left-over
	if (index2 < size_new) {
		Instruction instruction;
		instruction.beginNew = index2;
		instruction.newData.resize(size_new - index2);
		for (size_t a = index2, dataIndex = 0; a < size_new; ++a, ++dataIndex)
			instruction.newData[dataIndex] = buffer_new[a];		
		instructions.push_back(instruction);
	}

	// Finished with these source buffers
	delete[] buffer_old;
	delete[] buffer_new;

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
	char * buffer_patch_compressed(nullptr);
	size_t size_patch_compressed(0ull);
	if (!Archiver::CompressBuffer(buffer_patch, size_patch, &buffer_patch_compressed, size_patch_compressed))
		std::cout << "Failed creating compressed patch file.\n";
	else {
		// Compression successfull, delete uncompressed buffer
		delete[] buffer_patch;

		// Write-out compressed buffer to disk
		std::filesystem::create_directories(std::filesystem::path(path_patch).parent_path());
		std::ofstream file(path_patch, std::ios::binary | std::ios::out);
		if (file.is_open()) {
			file.write(buffer_patch_compressed, std::streamsize(size_patch_compressed));
			file.close();
		}
		delete[] buffer_patch_compressed;

		// Output results
		const auto end = std::chrono::system_clock::now();
		const std::chrono::duration<double> elapsed_seconds = end - start;
		std::cout
			<< "Successfully created patch file.\n"
			<< "Patch instructions: " << instructions.size() << "\n"
			<< "Patch size: " << size_patch << " bytes, compressed to " << size_patch_compressed << " bytes\n"
			<< "Elapsed time: " << elapsed_seconds.count() << " seconds\n";
	}

	// Exit
	system("pause");
	exit(1);
}