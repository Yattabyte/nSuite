#include "Archiver.h"
#include <chrono>
#include <direct.h>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>


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
	const auto path_source = directory + "\\old.exe", path_patch = directory + "\\file.patch";
	const auto size_source = std::filesystem::file_size(path_source), size_patch = std::filesystem::file_size(path_patch);
	std::ifstream file_source(path_source, std::ios::binary | std::ios::beg), file_patch(path_patch, std::ios::binary | std::ios::beg);
	char * buffer_source = new char[size_source], * buffer_patch = new char[size_patch];
	if (file_source.is_open() && file_patch.is_open()) {
		file_source.read(buffer_source, std::streamsize(size_source));
		file_patch.read(buffer_patch, std::streamsize(size_patch));
		file_source.close();
		file_patch.close();
	}

	// Attempt to decompress the buffer
	char * buffer_patch_decompressed(nullptr);
	size_t size_patch_decompressed(0ull);
	if (!Archiver::DecompressBuffer(buffer_patch, size_patch, &buffer_patch_decompressed, size_patch_decompressed))
		std::cout << "Failed decompressing patch file.\n";
	else {
		// Decompression successfull, delete uncompressed buffer
		delete[] buffer_patch;

		struct Instruction {
			size_t beginKeep = 0ull, endKeep = 0ull;	// range to keep
			size_t beginNew = 0ull;						// spot to insert
			std::vector<char> newData;					// data to insert
		};
		std::vector<Instruction> instructions;

		// Convert buffer into instructions
		size_t bytesRead(0ull);
		void * readingPtr = buffer_patch_decompressed;
		const auto size_finalFile = *reinterpret_cast<size_t*>(readingPtr);
		readingPtr = INCR_PTR(readingPtr, size_t(sizeof(size_t)));
		bytesRead += sizeof(size_t);
		while (bytesRead < size_patch_decompressed) {
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
		delete[] buffer_patch_decompressed;

		// Reconstruct new buffer from buffer_source + changes
		char * buffer3 = new char[size_finalFile];
		for each (const auto & change in instructions) {
			auto index3 = change.beginNew;
			for (auto x = change.beginKeep; x < change.endKeep; ++x)
				buffer3[index3++] = buffer_source[x];
			for each (const auto & value in change.newData)
				buffer3[index3++] = value;
		}
		delete[] buffer_source;

		// Write-out patched file to disk
		const auto outPath = directory + "\\patched.exe";
		if (!std::filesystem::exists(outPath))
			std::filesystem::create_directories(std::filesystem::path(outPath).parent_path());
		std::ofstream file(outPath, std::ios::binary | std::ios::out);
		if (file.is_open())
			file.write(buffer3, (std::streamsize)(size_finalFile));
		file.close();
		delete[] buffer3;

		// Output results
		const auto end = std::chrono::system_clock::now();
		const std::chrono::duration<double> elapsed_seconds = end - start;
		std::cout
			<< "Successfully patched file.\n"
			<< "Patch instructions: " << instructions.size() << "\n"
			<< "Patch size: " << size_patch << " bytes, decompressed to " << size_patch_decompressed << " bytes\n"
			<< "Elapsed time: " << elapsed_seconds.count() << " seconds\n";
	}
	
	// Exit
	system("pause");
	exit(1);
}