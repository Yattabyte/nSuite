#pragma once
#ifndef FINISHSTATE_H
#define FINISHSTATE_H

#include "State.h"


/** This state encapuslates the "Finished - Screen" state. */
class FinishState: public FrameState {
public:
	// Public (de)Constructors
	~FinishState();
	FinishState(Uninstaller * uninstaller, const HINSTANCE hInstance, const HWND parent, const RECT & rc);


	// Public Interface Implementations
	virtual void enact();
	virtual void pressPrevious();
	virtual void pressNext();
	virtual void pressClose();
};

#endif // FINISHSTATE_H