#pragma once
#ifndef FINISH_H
#define FINISH_H

#include "Screen.h"
#include <vector>


/** This state encapuslates the "Finished - Screen" state. */
class Finish: public Screen {
public:
	// Public (de)Constructors
	~Finish();
	Finish(Installer * installer, const HINSTANCE hInstance, const HWND parent, const vec2 & pos, const vec2 & size);


	// Public Interface Implementations
	virtual void enact() override;
	virtual void paint() override;


	// Public Methods
	/** Switch to the next state. */
	void goClose();


	// Public Attributes
	HWND m_checkbox = nullptr, m_btnClose = nullptr;
	bool m_showDirectory = true;
	std::vector<HWND> m_shortcutCheckboxes;
	std::vector<std::wstring> m_shortcuts_d, m_shortcuts_s;
};

#endif // FINISH_H