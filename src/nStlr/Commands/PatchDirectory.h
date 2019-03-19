#pragma once
#ifndef PATCHDIRECTORY_H
#define PATCHDIRECTORY_H

#include "Command.h"
#include <string>


/** Command to patch an entire directory, reading a patchfile. */
class PatchDirectory : public Command {
public:
	// Public interface
	virtual void execute(const int & argc, char * argv[]) const override;


private:
	// Private methods
	static bool ReadFile(const std::string & path, const size_t & size, char ** buffer, size_t & hash);
};

#endif // PATCHDIRECTORY_H
