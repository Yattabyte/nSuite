#pragma once
#ifndef DIRECTORYSTATE_H
#define DIRECTORYSTATE_H

#include "State.h"


/** This state encapuslates the "Choose a directory - Screen" state. */
class DirectoryState : public FrameState {
public:
	// Public (de)Constructors
	~DirectoryState();
	DirectoryState(Installer * installer, const HINSTANCE hInstance, const HWND parent, const RECT & rc);


	// Public Interface Implementations
	virtual void enact();
	virtual void pressPrevious();
	virtual void pressNext();
	virtual void pressClose();


	// Public Attributes
	HWND m_directoryField = nullptr, m_browseButton = nullptr;
};

#endif // DIRECTORYSTATE_H