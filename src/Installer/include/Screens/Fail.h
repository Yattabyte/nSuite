#pragma once
#ifndef FAIL_SCREEN_H
#define FAIL_SCREEN_H

#include "Screen.h"


/** This state encapuslates the "Failure - Screen" state. */
class Fail_Screen: public Screen {
public:
	// Public (de)Constructors
	~Fail_Screen();
	Fail_Screen(Installer * installer, const HINSTANCE hInstance, const HWND parent, const vec2 & pos, const vec2 & size);


	// Public Interface Implementations
	virtual void enact() override;
	virtual void paint() override;


	// Public Methods
	/** Switch to the next state. */
	void goClose();


	// Public Attributes
	HWND m_hwndLog = nullptr, m_btnClose = nullptr;
	size_t m_logIndex = 0ull;
};

#endif // FAIL_SCREEN_H