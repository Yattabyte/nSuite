#pragma once
#ifndef PACKCOMMAND_H
#define PACKCOMMAND_H

#include "Command.h"


/** Command to compress an entire directory into a single .npack file. */
class PackCommand : public Command {
public:
	// Public interface implementation
	virtual int execute(const int& argc, char* argv[]) const override;
};

#endif // PACKCOMMAND_H