#pragma once
#ifndef WELCOMEFRAME_H
#define WELCOMEFRAME_H

#include "Frame.h"


/** Custom frame class, representing the installer 'welcome' screen. */
class WelcomeFrame : public Frame {
public:
	// Public (de)Constructors
	~WelcomeFrame();
	WelcomeFrame(const HINSTANCE hInstance, const HWND parent, const RECT & rc);
};

#endif // WELCOMEFRAME_H