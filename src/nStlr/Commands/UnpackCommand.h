#pragma once
#ifndef UNPACKCOMMAND_H
#define UNPACKCOMMAND_H

#include "Command.h"


/** Command to decompress an entire directory from a single .npack file into its component files. */
class UnpackCommand : public Command {
public:
	// Public interface implementation
	virtual void execute(const int & argc, char * argv[]) const override;
};

#endif // UNPACKCOMMAND_H