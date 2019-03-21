#pragma once
#ifndef PATCHCOMMAND_H
#define PATCHCOMMAND_H

#include "Command.h"


/** Command to patch an entire directory, reading a patchfile. */
class PatchCommand : public Command {
public:
	// Public interface
	virtual void execute(const int & argc, char * argv[]) const override;
};

#endif // PATCHCOMMAND_H