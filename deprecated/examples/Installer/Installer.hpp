#pragma once
#ifndef InstallER_H
#define InstallER_H

#include "Resource.h"
#include "Threader.h"
#include <map>
#include <string>


class Screen;

/** Encapsulates the logical features of the Installer. */
class Installer {
public:
	// Public (de)Constructors
	~Installer() = default;
	explicit Installer(const HINSTANCE hInstance);


	// Public Enumerations
	const enum class ScreenEnums {
		WELCOME_SCREEN, AGREEMENT_SCREEN, DIRECTORY_SCREEN, Install_SCREEN, FINISH_SCREEN, FAIL_SCREEN,
		SCREEN_COUNT
	};


	// Public Methods
	/** When called, invalidates the Installer, halting it from progressing. */
	void invalidate();
	/** Make the screen identified by the supplied enum as active, deactivating the previous screen.
	@param	screenIndex		the new screen to use. */
	void setScreen(const ScreenEnums& screenIndex);
	/** Retrieves the current directory chosen for Installation.
	@return					active Installation directory. */
	std::string getDirectory() const;
	/** Sets a new Installation directory.
	@param	directory		new Installation directory. */
	void setDirectory(const std::string& directory);
	/** Retrieves the size of the drive used in the current directory.
	@return					the drive capacity. */
	size_t getDirectorySizeCapacity() const;
	/** Retrieves the remaining size of the drive used in the current directory.
	@return					the available size. */
	size_t getDirectorySizeAvailable() const;
	/** Retrieves the required size of the uncompressed package to be Installed.
	@return					the (uncompressed) package size. */
	size_t getDirectorySizeRequired() const;
	/** Retrieves the package name.
	@return					the package name. */
	std::string getPackageName() const;
	/** Install the Installer's package contents to the directory previously chosen. */
	void beginInstallation();
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
	Installer();


	// Private Attributes
	yatta::Resource m_archive, m_manifest;
	yatta::Threader m_threader;
	std::string m_directory = "", m_packageName = "";
	bool m_valid = true;
	size_t m_maxSize = 0ull, m_capacity = 0ull, m_available = 0ull;
	ScreenEnums m_currentIndex = ScreenEnums::WELCOME_SCREEN;
	Screen* m_screens[(int)ScreenEnums::SCREEN_COUNT]{};
	HWND m_hwnd = nullptr;
};

#endif // InstallER_H