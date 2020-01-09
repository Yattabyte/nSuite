#pragma once
#ifndef FINISH_SCREEN_H
#define FINISH_SCREEN_H

#include "Screen.h"
#include <string>
#include <vector>


/** This state encapsulates the "Finished - Screen" state. */
class Finish_Screen : public Screen {
public:
	// Public (de)Constructors
	~Finish_Screen();
	Finish_Screen(Installer* Installer, const HINSTANCE hInstance, const HWND parent, const vec2& pos, const vec2& size);


	// Public Interface Implementations
	virtual void enact() override;
	virtual void paint() override;


	// Public Methods
	/** Switch to the next state. */
	void goClose();
	/** Creates a shortcut file for the paths chosen.
	@param	srcPath		path to the target file that the shortcut will link to.
	@param	wrkPath		path to the working directory for the shortcut.
	@param	dstPath		path to where the shortcut should be placed. */
	static void createShortcut(const std::string& srcPath, const std::string& wrkPath, const std::string& dstPath);


	// Public Attributes
	HWND m_checkbox = nullptr, m_btnClose = nullptr;
	bool m_showDirectory = true;
	std::vector<HWND> m_shortcutCheckboxes;
	std::vector<std::wstring> m_shortcuts_d, m_shortcuts_s;
};

#endif // FINISH_SCREEN_H