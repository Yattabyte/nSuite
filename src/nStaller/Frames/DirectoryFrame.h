#pragma once
#ifndef DIRECTORYFRAME_H
#define DIRECTORYFRAME_H

#include "Frame.h"
#include <string>


class Installer;

/** Custom frame class, representing the installer 'directory choosing' screen. */
class DirectoryFrame : public Frame {
public:
	// Public (de)Constructors
	~DirectoryFrame();
	DirectoryFrame(Installer * installer, const HINSTANCE hInstance, const HWND parent, const RECT & rc);

	// Public Attributes
	Installer * m_installer = nullptr;
	HWND m_directoryField = nullptr, m_browseButton = nullptr;
};

#endif // DIRECTORYFRAME_H