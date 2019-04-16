#pragma once
#ifndef UNINSTALLSTATE_H
#define UNINSTALLSTATE_H

#include "State.h"
#include <string>
#include <thread>


/** This state encapuslates the "Uninstalling - Screen" state. */
class UninstallState : public FrameState {
public:
	// Public (de)Constructors
	~UninstallState();
	UninstallState(Uninstaller * uninstaller, const HINSTANCE hInstance, const HWND parent, const RECT & rc);


	// Public Interface Implementations
	virtual void enact();
	virtual void pressPrevious();
	virtual void pressNext();
	virtual void pressClose();


	// Public Attributes
	HWND m_hwndLog = nullptr, m_hwndPrgsBar = nullptr;
	size_t m_logIndex = 0ull, m_taskIndex = 0ull;
	std::wstring m_progress = L"0%";


private:
	// Private Attributes
	std::thread m_thread;
};

#endif // UNINSTALLSTATE_H