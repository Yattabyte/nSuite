#pragma once
#ifndef InstallERCOMMAND_H
#define InstallERCOMMAND_H

#include "Command.h"


/** Command to compress an entire directory into a fully-fledged Installer. */
class InstallerCommand : public Command {
public:
	// Public interface implementation
	virtual int execute(const int& argc, char* argv[]) const override;
};

#endif // InstallERCOMMAND_H