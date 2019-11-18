#pragma once
#ifndef FINISH_SCREEN_UN_H
#define FINISH_SCREEN_UN_H

#include "Screen.h"


/** This state encapsulates the "Finished - Screen" state. */
class Finish_Screen : public Screen {
public:
	// Public (de)Constructors
	~Finish_Screen();
	Finish_Screen(Uninstaller* uninstaller, const HINSTANCE hInstance, const HWND parent, const vec2& pos, const vec2& size);


	// Public Interface Implementations
	virtual void enact() override;
	virtual void paint() override;


	// Public Methods
	/** Switch to the next state. */
	void goClose();


	// Public Attributes
	HWND m_btnClose = nullptr;
};

#endif // FINISH_SCREEN_UN_H