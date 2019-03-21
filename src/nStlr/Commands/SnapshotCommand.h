#pragma once
#ifndef SNAPSHOTCOMMAND_H
#define SNAPSHOTCOMMAND_H

#include "Command.h"


/** Command to compress an entire directory into a diffable file. */
class SnapshotCommand : public Command {
public:
	// Public interface
	virtual void execute(const int & argc, char * argv[]) const override;
};

#endif // SNAPSHOTCOMMAND_H