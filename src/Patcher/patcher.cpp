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

	// Write the file into the archive
	const auto path1 = directory + "\\old.exe", path2 = directory + "\\file.patch";
	const auto size1 = std::filesystem::file_size(path1), size2 = std::filesystem::file_size(path2);
	std::ifstream file1(path1, std::ios::binary | std::ios::beg), file2(path2, std::ios::binary | std::ios::beg);
	char * buffer1 = new char[size1], * buffer2 = new char[size2];
	if (file1.is_open() && file2.is_open()) {
		file1.read(buffer1, (std::streamsize)size1);
		file2.read(buffer2, (std::streamsize)size2);
		file1.close();
		file2.close();
	}

	struct Instruction {
		size_t beginKeep = 0, endKeep = 0;	// range to keep
		size_t beginNew = 0;				// spot to insert
		std::vector<char> newData;			// data to insert
	};
	std::vector<Instruction> instructions;

	size_t bytesRead(0);
	void * readingPtr = buffer2;
	while (bytesRead < size2) {
		Instruction inst;

		inst.beginKeep = *reinterpret_cast<size_t*>(readingPtr);
		readingPtr = INCR_PTR(readingPtr, size_t(sizeof(size_t)));

		inst.endKeep = *reinterpret_cast<size_t*>(readingPtr);
		readingPtr = INCR_PTR(readingPtr, size_t(sizeof(size_t)));

		inst.beginNew = *reinterpret_cast<size_t*>(readingPtr);
		readingPtr = INCR_PTR(readingPtr, size_t(sizeof(size_t)));

		size_t length = *reinterpret_cast<size_t*>(readingPtr);
		readingPtr = INCR_PTR(readingPtr, size_t(sizeof(size_t)));		
		if (length) {
			auto charArray = reinterpret_cast<char*>(readingPtr);
			inst.newData.resize(length);
			for (size_t x = 0; x < length; ++x)
				inst.newData[x] = charArray[x];			
			readingPtr = INCR_PTR(readingPtr, length);
		}
		instructions.push_back(inst);
		bytesRead += (size_t(sizeof(size_t)) * 4ull) + size_t(sizeof(char) * length);
	}

	// Reconstruct buffer2 from buffer1 + changes
	char * buffer3 = new char[122880ull];
	for each (const auto & change in instructions) {
		auto index3 = change.beginNew;
		for (auto x = change.beginKeep; x < change.endKeep; ++x)
			buffer3[index3++] = buffer1[x];
		for each (const auto & value in change.newData) 
			buffer3[index3++] = value;
	}

	const auto outPath = directory + "\\patched.exe";
	if (!std::filesystem::exists(outPath))
		std::filesystem::create_directories(std::filesystem::path(outPath).parent_path());
	std::ofstream file(outPath, std::ios::binary | std::ios::out);
	if (file.is_open()) 
		file.write(buffer3, (std::streamsize)(size_t(122880ull)));
	file.close();

	delete[] buffer1;
	delete[] buffer2;
	delete[] buffer3;

	// Exit
	system("pause");
	exit(1);
}