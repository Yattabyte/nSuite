#pragma once
#ifndef FINISHFRAME_H
#define FINISHFRAME_H

#include "Frame.h"


/** Custom frame class, representing the installer 'finish' screen. */
class FinishFrame : public Frame {
public:
	// Public (de)Constructors
	~FinishFrame();
	FinishFrame(const HINSTANCE & hInstance, const HWND & parent, const int & x, const int & y, const int & w, const int & h);
};

#endif // FINISHFRAME_H