#pragma once
#ifndef FRAME_H
#define FRAME_H

#include <windows.h>
#pragma warning(push)
#pragma warning(disable:4458)
#include <gdiplus.h>
#pragma warning(pop)


using namespace Gdiplus;

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


	// Public Attributes
	int m_width = 0, m_height = 0;


protected:
	// Private Attributes
	WNDCLASSEX m_wcex;
	HWND m_hwnd = nullptr;
	HINSTANCE m_hinstance;
};

#endif // FRAME_H