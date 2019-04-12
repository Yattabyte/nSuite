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
	DirectoryFrame(std::string * directory, const std::string & packageName, const size_t & requiredSize, const HINSTANCE hInstance, const HWND parent, const RECT & rc);


	// Public Methods
	/** Set the directory to be used by this UI element. 
	@param	dir			the directory path. */
	void setDirectory(const std::string & dir);
	/** Retrieve the drive capacity, availabilty, and required size for this package.
	@param	capacity	reference updated with the size in bytes of the drive found.
	@param	available	reference updated with the size in bytes remaining on the drive found.
	@param	required	reference updated with the size in bytes of the package to be installed. */
	void getSizes(size_t & capacity, size_t & available, size_t & required) const;

private:
	// Private Attributes
	std::string * m_directory = nullptr, m_packageName = "";
	size_t m_requiredSize = 0ull;
	HWND m_directoryField = nullptr, m_browseButton = nullptr;
};

#endif // DIRECTORYFRAME_H