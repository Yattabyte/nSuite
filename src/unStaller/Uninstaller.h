#pragma once
#ifndef INSTALLER_H
#define INSTALLER_H

#include "Resource.h"
#include <map>
#include <string>
#include <Windows.h>


class FrameState;

/** Encapsulates the logical features of the uninstaller. */
class Uninstaller {
public:	
	// Public (de)Constructors
	~Uninstaller() = default;
	Uninstaller(const HINSTANCE hInstance);


	// Public Enumerations
	const enum StateEnums {
		WELCOME_STATE, UNINSTALL_STATE, FINISH_STATE, FAIL_STATE,
		STATE_COUNT
	};


	// Public Methods
	/** When called, invalidates the uninstaller, halting it from progressing. */
	void invalidate();
	/** Returns whether or not the uninstaller has been invalidated.
	@return					true if valid, false otherwise. */
	bool isValid() const;
	/** Flags the installation as complete. */
	void finish();
	/** Make the state identified by the supplied enum as active, deactivating the previous state.
	@param	stateIndex		the new state to use. */
	void setState(const StateEnums & stateIndex);
	/** Retrieves the current frame's enumeration. 
	@return					the current frame's index, as an enumeration. */
	StateEnums getCurrentIndex() const;
	/** Retrieves the current directory chosen for installation.
	@return					active installation directory. */
	std::wstring getDirectory() const;	
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


	// Public manifest strings
	struct compare_string { 
		bool operator()(const wchar_t * a, const wchar_t * b) const { 
			return wcscmp(a, b) < 0; 
		} 
	};
	std::map<const wchar_t*, std::wstring, compare_string> m_mfStrings;


private:
	// Private Constructors
	Uninstaller();


	// Private Attributes	
	Resource m_manifest;
	std::wstring m_directory = L"";
	bool m_valid = true, m_finished = false;
	StateEnums m_currentIndex = WELCOME_STATE;
	FrameState * m_states[STATE_COUNT];
	HWND 
		m_window = nullptr, 
		m_prevBtn = nullptr,
		m_nextBtn = nullptr,
		m_exitBtn = nullptr;
};


#endif // INSTALLER_H