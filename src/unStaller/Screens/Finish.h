#pragma once
#ifndef FINISHSTATE_H
#define FINISHSTATE_H

#include "Screen.h"


/** This state encapuslates the "Finished - Screen" state. */
class Finish: public Screen {
public:
	// Public (de)Constructors
	~Finish();
	Finish(Uninstaller * uninstaller, const HINSTANCE hInstance, const HWND parent, const vec2 & pos, const vec2 & size);


	// Public Interface Implementations
	virtual void enact() override;
	virtual void paint() override;


	// Public Methods
	/** Switch to the next state. */
	void goClose();


	// Public Attributes
	HWND m_btnClose = nullptr;
};

#endif // FINISHSTATE_H