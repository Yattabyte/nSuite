#pragma once
#ifndef FINISHFRAME_H
#define FINISHFRAME_H

#include "Frame.h"


/** Custom frame class, representing the installer 'finish' screen. */
class FinishFrame : public Frame {
public:
	// Public (de)Constructors
	~FinishFrame();
	FinishFrame(bool * openDirOnClose, const HINSTANCE hInstance, const HWND parent, const RECT & rc);


private:
	// Private Attributes
	bool * m_openDirOnClose = nullptr;
	HWND m_checkbox = nullptr;
};

#endif // FINISHFRAME_H