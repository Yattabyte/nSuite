#pragma once
#ifndef FINISHFRAME_H
#define FINISHFRAME_H

#include "Frame.h"


/** Custom frame class, representing the installer 'finish' screen. */
class FinishFrame : public Frame {
public:
	// Public (de)Constructors
	~FinishFrame();
	FinishFrame(bool * openDirOnClose, const HINSTANCE & hInstance, const HWND & parent, const int & x, const int & y, const int & w, const int & h);


	bool * m_openDirOnClose = nullptr;
};

#endif // FINISHFRAME_H