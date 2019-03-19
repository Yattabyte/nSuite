#include "DiffDirectory.h"
#include "BufferTools.h"
#include "Instructions.h"


void DiffDirectory::execute(const int & argc, char * argv[]) const 
{
	// Check command line arguments
	std::string oldDirectory(""), newDirectory(""), dstDirectory("");
	for (int x = 2; x < argc; ++x) {
		std::string command(argv[x], 5);
		std::transform(command.begin(), command.end(), command.begin(), ::tolower);
		if (command == "-old=")
			oldDirectory = std::string(&argv[x][5]);
		else if (command == "-new=")
			newDirectory = std::string(&argv[x][5]);
		else if (command == "-dst=")
			dstDirectory = std::string(&argv[x][5]);
		else
			exit_program("\n"
				"        Help:       /\n"
				" ~-----------------~\n"
				"/\n"
				"\n\n"
			);
	}

	// Create diff file
	const auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::tm bt;
	localtime_s(&bt, &time);
	std::string diffPath = dstDirectory + "\\"
		+ std::to_string(bt.tm_year) + std::to_string(bt.tm_mon) + std::to_string(bt.tm_mday) + std::to_string(bt.tm_hour) + std::to_string(bt.tm_min) + std::to_string(bt.tm_sec)
		+ ".diff";
	std::filesystem::create_directories(std::filesystem::path(diffPath).parent_path());
	std::ofstream file(diffPath, std::ios::binary | std::ios::out);
	if (!file.is_open())
		exit_program("Cannot write diff file to disk, aborting...\n");

	// Retrieve all common, added, and removed files
	PathPairList commonFiles;
	PathList addedFiles, removedFiles;
	Get_File_Lists(oldDirectory, newDirectory, commonFiles, addedFiles, removedFiles);

	// Generate Instructions from file lists
	std::atomic_size_t instructionCount(0ull), bytesWritten(0ull);
	for each (const auto & cFiles in commonFiles) {
		std::string  oldRelativePath(""), newRelativePath("");
		char *oldBuffer(nullptr), *newBuffer(nullptr);
		size_t oldHash(0ull), newHash(0ull);
		if (
			Read_File(cFiles.first, oldDirectory, oldRelativePath, &oldBuffer, oldHash)
			&&
			Read_File(cFiles.second, newDirectory, newRelativePath, &newBuffer, newHash)
			)
		{
			// Successfully opened both files
			if (oldHash != newHash) {
				// Files are different versions
				char * buffer(nullptr);
				size_t size(0ull), instructions(0ull);
				if (BFT::DiffBuffers(oldBuffer, cFiles.first.file_size(), newBuffer, cFiles.second.file_size(), &buffer, size, &instructions)) {
					Write_Instructions(newRelativePath, oldHash, newHash, buffer, size, 'U', file, bytesWritten);
					delete[] buffer;
				}
				instructionCount += instructions;
			}
		}
		delete[] oldBuffer;
		delete[] newBuffer;
	}	
	for each (const auto & nFile in addedFiles) {
		std::string newRelativePath("");
		char *newBuffer(nullptr);
		size_t newHash(0ull);
		if (Read_File(nFile, newDirectory, newRelativePath, &newBuffer, newHash)) {
			// Successfully opened file
			// This file is new
			char * buffer(nullptr);
			size_t size(0ull), instructions(0ull);
			if (BFT::DiffBuffers(nullptr, 0ull, newBuffer, nFile.file_size(), &buffer, size, &instructions)) {
				Write_Instructions(newRelativePath, 0ull, newHash, buffer, size, 'N', file, bytesWritten);
				delete[] buffer;
			}
			instructionCount += instructions;
		}
		delete[] newBuffer;
	}
	for each (const auto & oFile in removedFiles) {
		std::string oldRelativePath("");
		char *oldBuffer(nullptr);
		size_t oldHash(0ull);
		if (Read_File(oFile, oldDirectory, oldRelativePath, &oldBuffer, oldHash)) {
			// Successfully opened file
			// This file is no longer used
			instructionCount++;
			Write_Instructions(oldRelativePath, oldHash, 0ull, nullptr, 0ull, 'D', file, bytesWritten);			
		}
		delete[] oldBuffer;
	}

	// Output results
	std::cout
		<< "Instruction(s): " << instructionCount << "\n"
		<< "Bytes written:  " << bytesWritten << "\n";
}

void DiffDirectory::Get_File_Lists(const std::string & oldDirectory, const std::string & newDirectory, PathPairList & commonFiles, PathList & addFiles, PathList & delFiles)
{
	auto oldFiles = get_file_paths(oldDirectory);
	const auto newFiles = get_file_paths(newDirectory);
	for each (const auto & nFile in newFiles) {
		std::string newRelativePath = Get_Relative_Path(nFile, newDirectory);
		bool found = false;
		size_t oIndex(0ull);
		for each (const auto & oFile in oldFiles) {
			auto oldRelativePath = Get_Relative_Path(oFile, oldDirectory);
			if (oldRelativePath == newRelativePath) {
				// Common file found				
				std::cout << "diffing file \"" << oldRelativePath << "\"\n";
				commonFiles.push_back(std::make_pair(oFile, nFile));
				oldFiles.erase(oldFiles.begin() + oIndex);
				found = true;
				break;
			}
			oIndex++;
		}
		// New file found, add it
		if (!found) {
			std::cout << "adding file \"" << newRelativePath << "\"\n";
			addFiles.push_back(nFile);
		}

	}

	// All 'old files' that remain didn't exist in the 'new file' set
	for each (const auto & oFile in oldFiles) {
		auto oldRelativePath = Get_Relative_Path(oFile, oldDirectory);
		std::cout << "removing file \"" << oldRelativePath << "\"\n";
		delFiles.push_back(oFile);
	}
}

std::string DiffDirectory::Get_Relative_Path(const std::filesystem::directory_entry & file, const std::string & directory)
{
	const auto path = file.path().string();
	return path.substr(directory.length(), path.length() - directory.length());
}

bool DiffDirectory::Read_File(const std::filesystem::directory_entry & file, const std::string & directory, std::string & relativePath, char ** buffer, size_t & hash)
{
	relativePath = Get_Relative_Path(file, directory);
	std::ifstream f(file, std::ios::binary | std::ios::beg);
	if (f.is_open()) {
		*buffer = new char[file.file_size()];
		f.read(*buffer, std::streamsize(file.file_size()));
		f.close();
		hash = BFT::HashBuffer(*buffer, file.file_size());
		return true;
	}
	return false;
}

void DiffDirectory::Write_Instructions(const std::string & path, const size_t & oldHash, const size_t & newHash, char * buffer, const size_t & bufferSize, const char & flag, std::ofstream & file, std::atomic_size_t &bytesWritten) 
{
	auto pathLength = path.length();
	file.write(reinterpret_cast<char*>(&pathLength), std::streamsize(sizeof(size_t)));
	file.write(path.data(), std::streamsize(pathLength));
	file.write(&flag, std::streamsize(sizeof(char)));
	file.write(reinterpret_cast<char*>(const_cast<size_t*>(&oldHash)), std::streamsize(sizeof(size_t)));
	file.write(reinterpret_cast<char*>(const_cast<size_t*>(&newHash)), std::streamsize(sizeof(size_t)));
	file.write(reinterpret_cast<char*>(const_cast<size_t*>(&bufferSize)), std::streamsize(sizeof(size_t)));
	file.write(buffer, std::streamsize(bufferSize));
	bytesWritten += (sizeof(size_t) * 4) + (sizeof(char) * pathLength) + sizeof(char) + bufferSize;
};