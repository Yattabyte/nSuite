#pragma once
#ifndef FAILSTATE_H
#define FAILSTATE_H

#include "State.h"


/** This state encapuslates the "Failure - Screen" state. */
class FailState: public State {
public:
	// Public (de)Constructors
	~FailState() = default;
	FailState(Installer * installer);


	// Public Interface Implementations
	virtual void enact();
	virtual void pressPrevious();
	virtual void pressNext();
	virtual void pressClose();
};

#endif // FAILSTATE_H