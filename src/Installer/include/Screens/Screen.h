#pragma once
#ifndef SCREEN_H
#define SCREEN_H

#include <windows.h>
#pragma warning(push)
#pragma warning(disable:4458)
#include <gdiplus.h>
#pragma warning(pop)


using namespace Gdiplus;
class Installer;
/** Tuple containing x and y values. */
struct vec2 { int x = 0, y = 0; };

/**Encapsulation of a windows GDI 'window' object, for a particular screen of the application. */
class Screen {
public:
	// Public (de)Constructors
	Screen(Installer* installer, const vec2& pos, const vec2& size) : m_installer(installer), m_pos(pos), m_size(size) {}


	// Public Methods
	/** Sets the visibility & enable state of this window.
	@param	state	whether or not to show and activate this window. */
	void setVisible(const bool& state) {
		ShowWindow(m_hwnd, state);
		EnableWindow(m_hwnd, state);
	}


	// Public Interface Declarations
	/** Trigger this state to perform its screen action. */
	virtual void enact() = 0;
	/** Render this window. */
	virtual void paint() = 0;


	// Public Attributes
	Installer* m_installer = nullptr;
	HWND m_hwnd = nullptr;


protected:
	// Private Attributes
	WNDCLASSEX m_wcex{};
	HINSTANCE m_hinstance = nullptr;
	vec2 m_pos, m_size;
};

#endif // SCREEN_H