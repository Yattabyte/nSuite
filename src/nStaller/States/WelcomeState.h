#pragma once
#ifndef WELCOMESTATE_H
#define WELCOMESTATE_H

#include "State.h"


/***/
class WelcomeState : public State {
public:
	// Public (de)Constructors
	~WelcomeState() = default;
	WelcomeState(Installer * installer);


	// Public Methods
	virtual void enact();
	virtual void pressPrevious();
	virtual void pressNext();
	virtual void pressClose();
};

#endif // WELCOMESTATE_H