#pragma once
#ifndef FINISHSTATE_H
#define FINISHSTATE_H

#include "State.h"
#include <vector>


/** This state encapuslates the "Finished - Screen" state. */
class FinishState: public FrameState {
public:
	// Public (de)Constructors
	~FinishState();
	FinishState(Installer * installer, const HINSTANCE hInstance, const HWND parent, const RECT & rc);


	// Public Interface Implementations
	virtual void enact();
	virtual void pressPrevious();
	virtual void pressNext();
	virtual void pressClose();


	// Public Attributes
	HWND m_checkbox = nullptr;
	bool m_showDirectory = true;
	std::vector<HWND> m_shortcutCheckboxes;
	std::vector<std::wstring> m_shortcuts_d, m_shortcuts_s;
};

#endif // FINISHSTATE_H