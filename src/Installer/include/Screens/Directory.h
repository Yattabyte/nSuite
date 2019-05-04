#pragma once
#ifndef DIRECTORY_SCREEN_H
#define DIRECTORY_SCREEN_H

#include "Screen.h"


/** This state encapuslates the "Choose a directory - Screen" state. */
class Directory_Screen : public Screen {
public:
	// Public (de)Constructors
	~Directory_Screen();
	Directory_Screen(Installer * installer, const HINSTANCE hInstance, const HWND parent, const vec2 & pos, const vec2 & size);


	// Public Interface Implementations
	virtual void enact() override;
	virtual void paint() override;


	// Public Methods
	/** Browse for an installation directory. */
	void browse();
	/** Switch to the previous state. */
	void goPrevious();
	/** Switch to the next state. */
	void goInstall();
	/** Switch to the cancel state. */
	void goCancel();


	// Public Attributes
	HWND m_directoryField = nullptr, m_packageField = nullptr, m_browseButton = nullptr, m_btnPrev = nullptr, m_btnInst = nullptr, m_btnCancel = nullptr;
};

#endif // DIRECTORY_SCREEN_H