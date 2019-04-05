#pragma once
#ifndef DIRECTORYFRAME_H
#define DIRECTORYFRAME_H

#include "Frame.h"
#include <string>


/** Custom frame class, representing the installer 'directory choosing' screen. */
class DirectoryFrame : public Frame {
public:
	// Public (de)Constructors
	~DirectoryFrame();
	DirectoryFrame(std::string * directory, const HINSTANCE hInstance, const HWND parent, const RECT & rc);


	// Public Methods
	/** Set the directory to be used by this UI element. 
	@param	dir		the directory path. */
	void setDirectory(const std::string & dir);


private:
	// Private Attributes
	std::string * m_directory = nullptr;
	HWND m_directoryField = nullptr, m_browseButton = nullptr;
};

#endif // DIRECTORYFRAME_H