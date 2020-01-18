#pragma once
#ifndef UNInstallER_H
#define UNInstallER_H

#include "Resource.hpp"
#include "Threader.hpp"
#include <map>
#include <string>


class Screen;

/** Encapsulates the logical features of the unInstaller. */
class Uninstaller {
public:
	// Public (de)Constructors
	~Uninstaller() = default;
	explicit Uninstaller(const HINSTANCE hInstance);


	// Public Enumerations
	const enum class ScreenEnums {
		WELCOME_SCREEN, UNInstall_SCREEN, FINISH_SCREEN, FAIL_SCREEN,
		SCREEN_COUNT
	};


	// Public Methods
	/** When called, invalidates the unInstaller, halting it from progressing. */
	void invalidate();
	/** Make the screen identified by the supplied enum as active, deactivating the previous screen.
	@param	screenIndex		the new screen to use. */
	void setScreen(const ScreenEnums& screenIndex);
	/** Retrieves the Installation directory.
	@return					active Installation directory. */
	std::wstring getDirectory() const;
	/** Uninstall the application. */
	void beginUninstallation();
	/** Dumps error log to disk. */
	static void dumpErrorLog();
	/** Render this window. */
	void paint();


	// Public manifest strings
	/** Compares wide-char strings. */
	struct compare_string {
		bool operator()(const wchar_t* a, const wchar_t* b) const {
			return wcscmp(a, b) < 0;
		}
	};
	std::map<const wchar_t*, std::wstring, compare_string> m_mfStrings;


private:
	// Private Constructors
	Uninstaller();


	// Private Attributes
	yatta::Resource m_manifest;
	yatta::Threader m_threader;
	std::wstring m_directory = L"";
	bool m_valid = true;
	ScreenEnums m_currentIndex = ScreenEnums::WELCOME_SCREEN;
	Screen* m_screens[(int)ScreenEnums::SCREEN_COUNT]{};
	HWND m_hwnd = nullptr;
};


#endif // UNInstallER_H