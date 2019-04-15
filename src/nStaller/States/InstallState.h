#pragma once
#ifndef INSTALLSTATE_H
#define INSTALLSTATE_H

#include "State.h"
#include <thread>


/** This state encapuslates the "Installing - Screen" state. */
class InstallState : public FrameState {
public:
	// Public (de)Constructors
	~InstallState();
	InstallState(Installer * installer, const HINSTANCE hInstance, const HWND parent, const RECT & rc);


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

#endif // INSTALLSTATE_H