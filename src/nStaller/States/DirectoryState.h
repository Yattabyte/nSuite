#pragma once
#ifndef DIRECTORYSTATE_H
#define DIRECTORYSTATE_H

#include "State.h"


/***/
class DirectoryState : public State {
public:
	// Public (de)Constructors
	~DirectoryState() = default;
	DirectoryState(Installer * installer);


	// Public Methods
	virtual void enact();
	virtual void pressPrevious();
	virtual void pressNext();
	virtual void pressClose();
};

#endif // DIRECTORYSTATE_H