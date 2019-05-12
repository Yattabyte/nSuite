#pragma once
#ifndef INSTALLER_H
#define INSTALLER_H

#include "Resource.h"
#include "Threader.h"
#include <map>
#include <string>


class Screen;

/** Encapsulates the logical features of the installer. */
class Installer {
public:	
	// Public (de)Constructors
	/** Destroy the installer. */
	~Installer() = default;
	/** Construct the installer. */
	Installer(const HINSTANCE hInstance);


	// Public Enumerations
	/** ID's for all the screens involved in this GUI. */
	const enum ScreenEnums {
		WELCOME_SCREEN, AGREEMENT_SCREEN, DIRECTORY_SCREEN, INSTALL_SCREEN, FINISH_SCREEN, FAIL_SCREEN,
		SCREEN_COUNT
	};


	// Public Methods
	/** When called, invalidates the installer, halting it from progressing. */
	void invalidate();
	/** Make the screen identified by the supplied enum as active, deactivating the previous screen.
	@param	screenIndex		the new screen to use. */
	void setScreen(const ScreenEnums & screenIndex);
	/** Retrieves the current directory chosen for installation.
	@return					active installation directory. */
	std::string getDirectory() const;
	/** Sets a new installation directory.
	@param	directory		new installation directory. */
	void setDirectory(const std::string & directory);
	/** Retrieves the size of the drive used in the current directory.
	@return					the drive capacity. */
	size_t getDirectorySizeCapacity() const;
	/** Retrieves the remaining size of the drive used in the current directory.
	@return					the available size. */
	size_t getDirectorySizeAvailable() const;
	/** Retrieves the required size of the uncompressed package to be installed.
	@return					the (uncompressed) package size. */
	size_t getDirectorySizeRequired() const;
	/** Retrieves the package name.
	@return					the package name. */
	std::string getPackageName() const;
	/** Install the installer's package contents to the directory previously chosen. */
	void beginInstallation();
	/** Dumps error log to disk. */
	static void dumpErrorLog();
	/** Render this window. */
	void paint();


	// Public manifest strings
	/** Compares wide-char strings. */
	struct compare_string { 
		/** Compares 2 input strings. */
		bool operator()(const wchar_t * a, const wchar_t * b) const { 
			return std::wcscmp(a, b) < 0; 
		} 
	};
	std::map<const wchar_t*, std::wstring, compare_string> m_mfStrings;


private:
	// Private Constructors
	Installer();


	// Private Attributes
	NST::Threader m_threader;
	NST::Resource m_archive, m_manifest;
	std::string m_directory = "", m_packageName = "";
	bool m_valid = true;
	size_t m_maxSize = 0ull, m_capacity = 0ull, m_available = 0ull;
	ScreenEnums m_currentIndex = WELCOME_SCREEN;
	Screen * m_screens[SCREEN_COUNT];
	HWND m_hwnd = nullptr;
};

#endif // INSTALLER_H