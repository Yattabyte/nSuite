#pragma once
#ifndef INSTALLER_H
#define INSTALLER_H

#include "Resource.h"
#include <string>
#include <Windows.h>


class Frame;
class State;

/***/
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
	void showFrame(const FrameEnums & newIndex);
	void setState(State * state);
	int getCurrentIndex() const;
	std::string getDirectory() const;
	char * getPackagePointer() const;
	size_t getPackageSize() const;
	void updateButtons(const WORD btnHandle);
	void showButtons(const bool & prev, const bool & next, const bool & close);
	void enableButtons(const bool & prev, const bool & next, const bool & close);


private:
	// Private Attributes
	Resource m_archive;
	std::string m_directory = "", m_packageName = "";
	bool m_openDirectoryOnClose = true;
	char * m_packagePtr = nullptr;
	size_t m_packageSize = 0ull;
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