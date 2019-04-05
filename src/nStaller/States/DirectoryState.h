#pragma once
#ifndef DIRECTORYSTATE_H
#define DIRECTORYSTATE_H

#include "State.h"


/** This state encapuslates the "Choose a directory - Screen" state. */
class DirectoryState : public State {
public:
	// Public (de)Constructors
	~DirectoryState() = default;
	DirectoryState(Installer * installer);


	// Public Interface Implementations
	virtual void enact();
	virtual void pressPrevious();
	virtual void pressNext();
	virtual void pressClose();
};

#endif // DIRECTORYSTATE_H