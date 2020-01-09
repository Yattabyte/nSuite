#pragma once
#ifndef UNInstall_SCREEN_UN_H
#define UNInstall_SCREEN_UN_H

#include "Screens/Screen.hpp


/** This state encapsulates the "Uninstalling - Screen" state. */
class Uninstall_Screen : public Screen {
public:
	// Public (de)Constructors
	~Uninstall_Screen();
	Uninstall_Screen(Uninstaller* unInstaller, const HINSTANCE hInstance, const HWND parent, const vec2& pos, const vec2& size);


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

#endif // UNInstall_SCREEN_UN_H