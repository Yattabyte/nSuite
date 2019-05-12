#pragma once
#ifndef INSTALL_SCREEN_H
#define INSTALL_SCREEN_H

#include "Screen.h"
#include <string>


/** This state encapuslates the "Installing - Screen" state. */
class Install_Screen : public Screen {
public:
	// Public (de)Constructors
	/** Destroy this screen. */
	~Install_Screen();
	/** Construct this screen. */
	Install_Screen(Installer * installer, const HINSTANCE hInstance, const HWND parent, const vec2 & pos, const vec2 & size);


	// Public Interface Implementations
	virtual void enact() override;
	virtual void paint() override;


	// Public Methods
	/** Switch to the next state. */
	void goFinish();


	// Public Attributes
	HWND m_hwndLog = nullptr, m_hwndPrgsBar = nullptr, m_btnFinish = nullptr;
	size_t m_logIndex = 0ull, m_taskIndex = 0ull;
};

#endif // INSTALL_SCREEN_H