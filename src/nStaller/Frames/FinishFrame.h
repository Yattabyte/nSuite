#pragma once
#ifndef FINISHFRAME_H
#define FINISHFRAME_H

#include "Frame.h"


class Installer;

/** Custom frame class, representing the installer 'finish' screen. */
class FinishFrame : public Frame {
public:
	// Public (de)Constructors
	~FinishFrame();
	FinishFrame(Installer * installer, const HINSTANCE hInstance, const HWND parent, const RECT & rc);


	// Public Attributes
	Installer * m_installer = nullptr;
	HWND m_checkbox = nullptr;
};

#endif // FINISHFRAME_H