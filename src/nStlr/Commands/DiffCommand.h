#pragma once
#ifndef DIFFDIRECTORY_H
#define DIFFDIRECTORY_H

#include "Command.h"
#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>


/** Command to diff an entire directory, creating a patchfile. */
class DiffCommand : public Command {
public:
	// Public interface
	virtual void execute(const int & argc, char * argv[]) const override;
};

#endif // DIFFDIRECTORY_H
