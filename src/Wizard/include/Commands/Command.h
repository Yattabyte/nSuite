#pragma once
#ifndef COMMAND_H
#define COMMAND_H


/** An interface for specific commands used in the nStlr application.*/
struct Command {
	// Public Interface
	/** Virtual method for executing a command.
	@param		argc		the number of arguments.
	@param		argv		array of arguments. */
	virtual int execute(const int & argc, char * argv[]) const = 0;
};

#endif // COMMAND_H
