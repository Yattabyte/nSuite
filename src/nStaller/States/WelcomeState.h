#pragma once
#ifndef WELCOMESTATE_H
#define WELCOMESTATE_H

#include "State.h"


/** This state encapuslates the "Welcome - Screen" state. */
class WelcomeState : public FrameState {
public:
	// Public (de)Constructors
	~WelcomeState();
	WelcomeState(Installer * installer, const HINSTANCE hInstance, const HWND parent, const RECT & rc);


	// Public Interface Implementations
	virtual void enact();
	virtual void pressPrevious();
	virtual void pressNext();
	virtual void pressClose();
};

#endif // WELCOMESTATE_H