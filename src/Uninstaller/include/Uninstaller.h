#pragma once
#ifndef UNINSTALLER_H
#define UNINSTALLER_H

#include "Resource.h"
#include "Threader.h"
#include <map>
#include <string>


class Screen;

/** Encapsulates the logical features of the uninstaller. */
class Uninstaller {
public:	
	// Public (de)Constructors
	~Uninstaller() = default;
	Uninstaller(const HINSTANCE hInstance);


	// Public Enumerations
	const enum ScreenEnums {
		WELCOME_SCREEN, UNINSTALL_SCREEN, FINISH_SCREEN, FAIL_SCREEN,
		SCREEN_COUNT
	};


	// Public Methods
	/** When called, invalidates the uninstaller, halting it from progressing. */
	void invalidate();
	/** Make the screen identified by the supplied enum as active, deactivating the previous screen.
	@param	screenIndex		the new screen to use. */
	void setScreen(const ScreenEnums & screenIndex);
	/** Retrieves the installation directory.
	@return					active installation directory. */
	std::wstring getDirectory() const;	
	/** Uninstall the application. */
	void beginUninstallation();
	/** Dumps error log to disk. */
	static void dumpErrorLog();
	/** Render this window. */
	void paint();


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
	Threader m_threader;
	Resource m_manifest;
	std::wstring m_directory = L"";
	bool m_valid = true;
	ScreenEnums m_currentIndex = WELCOME_SCREEN;
	Screen * m_screens[SCREEN_COUNT];
	HWND m_hwnd = nullptr;
};


#endif // UNINSTALLER_H