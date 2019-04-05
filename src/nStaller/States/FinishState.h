#pragma once
#ifndef FINISHSTATE_H
#define FINISHSTATE_H

#include "State.h"


/***/
class FinishState: public State {
public:
	// Public (de)Constructors
	~FinishState() = default;
	FinishState(Installer * installer);


	// Public Methods
	virtual void enact();
	virtual void pressPrevious();
	virtual void pressNext();
	virtual void pressClose();
};

#endif // FINISHSTATE_H