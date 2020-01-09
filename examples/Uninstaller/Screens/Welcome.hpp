#pragma once
#ifndef WELCOME_SCREEN_UN_H
#define WELCOME_SCREEN_UN_H

#include "Screens/Screen.hpp"


/** This state encapsulates the "Welcome - Screen" state. */
class Welcome_Screen : public Screen {
public:
	// Public (de)Constructors
	~Welcome_Screen();
	Welcome_Screen(Uninstaller* unInstaller, const HINSTANCE hInstance, const HWND parent, const vec2& pos, const vec2& size);


	// Public Interface Implementations
	virtual void enact() override;
	virtual void paint() override;


	// Public Methods
	/** Switch to the next state. */
	void goNext();
	/** Switch to the cancel state. */
	static void goCancel();


	// Public Attributes
	HWND m_btnNext = nullptr, m_btnCancel = nullptr;
};

#endif // WELCOME_SCREEN_UN_H