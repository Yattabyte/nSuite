#pragma once
#ifndef DIFFDIRECTORY_H
#define DIFFDIRECTORY_H

#include "Command.h"


/** Command to diff an entire directory, creating a patchfile. */
struct DiffDirectory : Command {
	virtual void execute(const int & argc, char * argv[]) const override;
};

#endif // DIFFDIRECTORY_H
