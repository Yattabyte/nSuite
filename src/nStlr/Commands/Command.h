#pragma once
#ifndef COMMAND_H
#define COMMAND_H

struct Command {
	virtual void execute(const int & argc, char * argv[]) const = 0;
};

#endif // COMMAND_H
