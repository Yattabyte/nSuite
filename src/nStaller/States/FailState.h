#pragma once
#ifndef FAILSTATE_H
#define FAILSTATE_H

#include "State.h"


/** This state encapuslates the "Failure - Screen" state. */
class FailState: public FrameState {
public:
	// Public (de)Constructors
	~FailState();
	FailState(Installer * installer, const HINSTANCE hInstance, const HWND parent, const RECT & rc);


	// Public Interface Implementations
	virtual void enact();
	virtual void pressPrevious();
	virtual void pressNext();
	virtual void pressClose();


	// Public Attributes
	HWND m_hwndLog = nullptr;
	size_t m_logIndex = 0ull;
};

#endif // FAILSTATE_H