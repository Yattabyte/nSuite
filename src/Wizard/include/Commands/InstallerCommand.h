#pragma once
#ifndef INSTALLERCOMMAND_H
#define INSTALLERCOMMAND_H

#include "Command.h"


/** Command to compress an entire directory into a portable installer. */
class InstallerCommand : public Command {
public:
	// Public interface implementation
	virtual void execute(const int & argc, char * argv[]) const override;
};

#endif // INSTALLERCOMMAND_H