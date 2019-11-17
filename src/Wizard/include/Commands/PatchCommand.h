#pragma once
#ifndef PATCHCOMMAND_H
#define PATCHCOMMAND_H

#include "Command.h"


/** Command to patch an entire directory, reading a patch-file. */
class PatchCommand : public Command {
public:
	// Public interface implementation
	virtual int execute(const int & argc, char * argv[]) const override;
};

#endif // PATCHCOMMAND_H