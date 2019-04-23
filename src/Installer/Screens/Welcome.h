#pragma once
#ifndef WELCOME_H
#define WELCOME_H

#include "Screen.h"


/** This state encapuslates the "Welcome - Screen" state. */
class Welcome : public Screen {
public:
	// Public (de)Constructors
	~Welcome();
	Welcome(Installer * installer, const HINSTANCE hInstance, const HWND parent, const vec2 & pos, const vec2 & size);


	// Public Interface Implementations
	virtual void enact() override;
	virtual void paint() override;
	

	// Public Methods
	/** Switch to the next state. */
	void goNext();
	/** Switch to the cancel state. */
	void goCancel();
	

	// Public Attributes
	HWND m_btnNext = nullptr, m_btnCancel = nullptr;
};

#endif // WELCOME_H