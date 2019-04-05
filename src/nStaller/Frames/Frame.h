#pragma once
#ifndef FRAME_H
#define FRAME_H

#include <windows.h>


/** Encapsulation of a windows GDI 'window' object.*/
class Frame {
public:
	// Public Methods
	/** Sets the visibility & enable state of this window.
	@param	state	whether or not to show and activate this window. */
	void setVisible(const bool & state) {
		ShowWindow(m_hwnd, state);
		EnableWindow(m_hwnd, state);
	}


protected:
	// Private Attributes
	WNDCLASSEX m_wcex;
	HWND m_hwnd = nullptr;
	HINSTANCE m_hinstance;
};

#endif // FRAME_H