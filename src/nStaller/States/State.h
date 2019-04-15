#pragma once
#ifndef STATE_H
#define STATE_H

#include <windows.h>
#pragma warning(push)
#pragma warning(disable:4458)
#include <gdiplus.h>
#pragma warning(pop)


using namespace Gdiplus;
class Installer;

/**Encapsulation of a windows GDI 'window' object, for a particular state of the application. */
class FrameState {
public:
	// Public (de)Constructors
	FrameState(Installer * installer) : m_installer(installer) {}
	
	
	// Public Methods
	/** Sets the visibility & enable state of this window.
	@param	state	whether or not to show and activate this window. */
	void setVisible(const bool & state) {
		ShowWindow(m_hwnd, state);
		EnableWindow(m_hwnd, state);
	}


	// Public Interface Declarations
	/** Trigger this state to perform its screen action. */
	virtual void enact() = 0;
	/** Cause this state to process the "previous" action. */
	virtual void pressPrevious() = 0;
	/** Cause this state to process the "next" action. */
	virtual void pressNext() = 0;
	/** Cause this state to process the "close" action. */
	virtual void pressClose() = 0;


	// Public Attributes
	Installer * m_installer = nullptr;
	HWND m_hwnd = nullptr;


protected:
	// Private Attributes
	WNDCLASSEX m_wcex;
	HINSTANCE m_hinstance;
};

#endif // STATE_H