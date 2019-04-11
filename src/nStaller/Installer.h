#pragma once
#ifndef INSTALLER_H
#define INSTALLER_H

#include "Resource.h"
#include <string>
#include <Windows.h>


class Frame;
class State;

/** Encapsulates the logical features of the installer. */
class Installer {
public:	
	// Public (de)Constructors
	~Installer() = default;
	Installer(const HINSTANCE hInstance);


	// Public Enumerations
	const enum FrameEnums {
		WELCOME_FRAME, DIRECTORY_FRAME, INSTALL_FRAME, FINISH_FRAME, FAIL_FRAME,
		FRAME_COUNT
	};


	// Public Methods
	/** When called, invalidates the installer, halting it from progressing. */
	void invalidate();
	/** Flags the installation as complete. */
	void finish();
	/** Displays the screen matching the supplied enumeration.
	@param	newIndex		the screen to make visible (makes current screen invisible). */
	void showFrame(const FrameEnums & newIndex);
	/** Override the current state, making the supplied state active.
	@param	state			the new state to use. */
	void setState(State * state);
	/** Retrieves the current frame's enumeration. 
	@return					the current frame's index, as an enumeration. */
	FrameEnums getCurrentIndex() const;
	/** Retrieves the current directory chosen for installation.
	@return					active installation directory. */
	std::string getDirectory() const;
	/** Retrieve if the chosen directory should open when the installer closes.
	@return					true if should show, false otherwise. */
	bool shouldShowDirectory() const;
	/** Retrieves the pointer to the compressed packaged contents.
	@return					the package pointer (offset of folder name data). */
	char * getPackagePointer() const;
	/** Retrieves the size of the compressed package.
	@return					the package size (minus the folder name data). */
	size_t getPackageSize() const;
	/** Check which button has been active, and perform it's state operation.
	@param	btnHandle		handle to the currently active button. */
	void updateButtons(const WORD btnHandle);
	/** Sets the visibility state of all 3 buttons.
	@param	prev			make the 'previous' button visible.
	@param	next			make the 'next' button visible.
	@param	close			make the 'close' button visible. */
	void showButtons(const bool & prev, const bool & next, const bool & close);
	/** Sets the enable state of all 3 buttons.
	@param	prev			make the 'previous' button enabled.
	@param	next			make the 'next' button enabled.
	@param	close			make the 'close' button enabled. */
	void enableButtons(const bool & prev, const bool & next, const bool & close);


private:
	// Private Attributes
	Resource m_archive;
	std::string m_directory = "", m_packageName = "";
	bool  m_valid = true, m_showDirectoryOnClose = true, m_finished = false;
	char * m_packagePtr = nullptr;
	size_t m_packageSize = 0ull, m_maxSize = 0ull;
	FrameEnums m_currentIndex = WELCOME_FRAME;
	Frame * m_frames[FRAME_COUNT];
	State * m_state = nullptr;
	HWND 
		m_window = nullptr, 
		m_prevBtn = nullptr,
		m_nextBtn = nullptr,
		m_exitBtn = nullptr;
};


#endif // INSTALLER_H