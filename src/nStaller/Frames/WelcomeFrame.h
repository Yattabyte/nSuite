#pragma once
#ifndef WELCOMEFRAME_H
#define WELCOMEFRAME_H

#include "Frame.h"


class Installer;

/** Custom frame class, representing the installer 'welcome' screen. */
class WelcomeFrame : public Frame {
public:
	// Public (de)Constructors
	~WelcomeFrame();
	WelcomeFrame(Installer * installer, HINSTANCE hInstance, const HWND parent, const RECT & rc);


	// Public Attributes
	Installer * m_installer = nullptr;
};

#endif // WELCOMEFRAME_H