#pragma once
#ifndef DIFFDIRECTORY_H
#define DIFFDIRECTORY_H

#include "Command.h"
#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>


/** Command to diff an entire directory, creating a patchfile. */
class DiffDirectory : public Command {
public:
	// Public interface
	virtual void execute(const int & argc, char * argv[]) const override;


private:
	// Private declarations
	typedef std::vector<std::filesystem::directory_entry> PathList;
	typedef std::vector<std::pair<std::filesystem::directory_entry, std::filesystem::directory_entry>> PathPairList;


	// Private methods
	static std::string Get_Relative_Path(const std::filesystem::directory_entry & file, const std::string & directory);
	static void Get_File_Lists(const std::string & oldDirectory, const std::string & newDirectory, PathPairList & commonFiles, PathList & addFiles, PathList & delFiles);
	static bool Read_File(const std::filesystem::directory_entry & file, const std::string & directory, std::string & relativePath, char ** buffer, size_t & hash);
	static void Write_Instructions(const std::string & path, const size_t & oldHash, const size_t & newHash, char * buffer, const size_t & bufferSize, const char & flag, std::ofstream & file, std::atomic_size_t & bytesWritten);
};

#endif // DIFFDIRECTORY_H
