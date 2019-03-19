#pragma once
#ifndef PATCHDIRECTORY_H
#define PATCHDIRECTORY_H

#include "Command.h"


/** Command to patch an entire directory, reading a patchfile. */
struct PatchDirectory : Command {
	virtual void execute(const int & argc, char * argv[]) const override;
};

#endif // PATCHDIRECTORY_H
