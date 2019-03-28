#pragma once
#ifndef DIFFCOMMAND_H
#define DIFFCOMMAND_H

#include "Command.h"


/** Command to diff an entire directory, creating a patchfile. */
class DiffCommand : public Command {
public:
	// Public interface implementation
	virtual void execute(const int & argc, char * argv[]) const override;
};

#endif // DIFFCOMMAND_H