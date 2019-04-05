#pragma once
#ifndef INSTALLFRAME_H
#define INSTALLFRAME_H

#include "Frame.h"
#include <string>


/** Custom frame class, representing the installer 'install' screen. */
class InstallFrame : public Frame {
public:
	// Public (de)Constructors
	~InstallFrame();
	InstallFrame(const HINSTANCE hInstance, const HWND parent, const RECT & rc);
	

private:
	// Private Attributes
	HWND m_hwndLog = nullptr, m_hwndPrgsBar = nullptr, m_hwndPrgsText = nullptr;
	size_t m_logIndex = 0ull, m_taskIndex = 0ull;
};

#endif // INSTALLFRAME_H