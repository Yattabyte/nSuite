#pragma once
#ifndef Install_SCREEN_H
#define Install_SCREEN_H

#include "Screen.h"
#include <string>


/** This state encapsulates the "Installing - Screen" state. */
class Install_Screen : public Screen {
public:
	// Public (de)Constructors
	~Install_Screen();
	Install_Screen(Installer* Installer, const HINSTANCE hInstance, const HWND parent, const vec2& pos, const vec2& size);


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

#endif // Install_SCREEN_H