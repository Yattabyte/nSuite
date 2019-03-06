#include <direct.h>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>


static bool find_next_best_match(const char * const buffer1, const char * const buffer2, const size_t & size1, const size_t & size2, size_t & index1, size_t & index2)
{
	const auto start2 = index2;
	while (index1 < size1) {
		if (index2 >= size2) {
			// Reached end of buffer2, increment buffer1 index and try again
			index1++;
			if (index1 < size1)
				index2 = start2;
			continue;
		}
		if (buffer1[index1] == buffer2[index2]) {
			// these 2 values match, but lets see if the next set matches
			if ((index1 + 1) < size1 && (index2 + 1) < size2)
				// we can still index further, so lets see if the next set matches
				if (buffer1[index1 + 1] == buffer2[index2 + 1])
					// confirmed that [index1] == [index2] and [index1+1] == [index2+1]
					return true;
				else {
					index2++;
					continue;
				}

			// we can't index any further, so this match is the best we can get
			return true;
		}
		index2++;
	}
	return false;
}

/** Entry point. */
int main()
{
	// Get the running directory
	char cCurrentPath[FILENAME_MAX];
	if (_getcwd(cCurrentPath, sizeof(cCurrentPath)))
		cCurrentPath[sizeof(cCurrentPath) - 1ull] = char('\0');
	const auto directory = std::string(cCurrentPath);

	// Write the file into the archive
	const auto path1 = directory + "\\old.exe", path2 = directory + "\\new.exe";
	const auto size1 = std::filesystem::file_size(path1), size2 = std::filesystem::file_size(path2);
	std::ifstream file1(path1, std::ios::binary | std::ios::beg), file2(path2, std::ios::binary | std::ios::beg);
	char * buffer1 = new char[size1], * buffer2 = new char[size2];
	if (file1.is_open() && file2.is_open()) {
		file1.read(buffer1, (std::streamsize)size1);
		file2.read(buffer2, (std::streamsize)size2);
		file1.close();
		file2.close();
	}

	size_t index1 = 0, index2 = 0;
	struct Instruction {
		size_t beginKeep = 0, endKeep = 0;	// range to keep
		size_t beginNew = 0;				// spot to insert
		std::vector<char> newData;			// data to insert
	};
	std::vector<Instruction> instructions;

	// Continue algorithm as long as both buffers have data left
	while (index1 < size1 && index2 < size2) {
		// Check if bits at the current index is different between the 2 buffers
		if (buffer1[index1] != buffer2[index2]) {
			size_t index1_Next = index1, index2_Next = index2;
			find_next_best_match(buffer1, buffer2, size1, size2, index1_Next, index2_Next);
			// Everything between index1 and index1_Next has been deleted in buffer1
			// Here, we only care about what was added
			// Everything between index2 and index2_Next has been inserted in buffer2
			if (index2_Next > index2) {
				Instruction instruction;
				instruction.beginNew = index2;
				instruction.newData.resize(index2_Next - index2);
				for (size_t a = index2, dataIndex = 0; a < index2_Next; ++a, ++dataIndex)
					instruction.newData[dataIndex] = buffer2[a];
				instructions.push_back(instruction);
				index2 = index2_Next;
			}
			index1 = index1_Next;
		}
		// Both buffers match at index1 and index2
		if (index1+1 < size1 )
			instructions.emplace_back(Instruction{ index1, index1 + 1, index2 });
		index1++;
		index2++;
	}
	// One of the 2 buffers has reached its end
	// If there is data left from buffer 2, append it as the final instruction
	if (index2 < size2) {
		Instruction instruction;
		instruction.beginNew = index2;
		instruction.newData.resize(size2 - index2);
		for (size_t a = index2, dataIndex = 0; a < size2; ++a, ++dataIndex)
			instruction.newData[dataIndex] = buffer2[a];		
		instructions.push_back(instruction);
	}

	delete[] buffer1;
	delete[] buffer2;

	const auto outPath = directory + "\\file.patch";
	if (!std::filesystem::exists(outPath))
		std::filesystem::create_directories(std::filesystem::path(outPath).parent_path());
	std::ofstream file(outPath, std::ios::binary | std::ios::out);
	if (file.is_open()) {		
		for (auto instruction : instructions) {
			file.write(reinterpret_cast<char*>(&instruction.beginKeep), (std::streamsize)(sizeof(size_t)));
			file.write(reinterpret_cast<char*>(&instruction.endKeep), (std::streamsize)(sizeof(size_t)));
			file.write(reinterpret_cast<char*>(&instruction.beginNew), (std::streamsize)(sizeof(size_t)));
			size_t length = instruction.newData.size();
			file.write(reinterpret_cast<char*>(&length), (std::streamsize)(sizeof(size_t)));
			file.write(instruction.newData.data(), (std::streamsize)(instruction.newData.size()));
		}
	}
	file.close();

	// Exit
	system("pause");
	exit(1);
}