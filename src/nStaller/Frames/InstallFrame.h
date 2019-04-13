#pragma once
#ifndef INSTALLFRAME_H
#define INSTALLFRAME_H

#include "Frame.h"
#include <string>


class Installer;

/** Custom frame class, representing the installer 'install' screen. */
class InstallFrame : public Frame {
public:
	// Public (de)Constructors
	~InstallFrame();
	InstallFrame(Installer * installer, const HINSTANCE hInstance, const HWND parent, const RECT & rc);
	

	// Public Attributes
	Installer * m_installer = nullptr;
	HWND m_hwndLog = nullptr, m_hwndPrgsBar = nullptr;
	size_t m_logIndex = 0ull, m_taskIndex = 0ull;
	std::wstring m_progress = L"0%";
};

#endif // INSTALLFRAME_H