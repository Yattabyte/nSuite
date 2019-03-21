#pragma once
#ifndef COMPRESSDIRECTORY_H
#define COMPRESSDIRECTORY_H

#include "Command.h"


/** Command to compress an entire directory, creating a patchfile. */
class InstallerCommand : public Command {
public:
	// Public interface
	virtual void execute(const int & argc, char * argv[]) const override;
};

#endif // COMPRESSDIRECTORY_H
