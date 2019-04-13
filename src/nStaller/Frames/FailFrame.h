#pragma once
#ifndef FAILFRAME_H
#define FAILFRAME_H

#include "Frame.h"


class Installer;

/** Custom frame class, representing the installer 'failure' screen. */
class FailFrame : public Frame {
public:
	// Public (de)Constructors
	~FailFrame();
	FailFrame(Installer * installer, const HINSTANCE hInstance, const HWND parent, const RECT & rc);


	// Public Attributes
	Installer * m_installer = nullptr;
	HWND m_hwndLog = nullptr;
	size_t m_logIndex = 0ull;
};

#endif // FAILFRAME_H