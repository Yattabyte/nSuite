#pragma once
#ifndef AGREEMENTFRAME_H
#define AGREEMENTFRAME_H

#include "Frame.h"
#include <string>


class Installer;

/** Custom frame class, representing the installer 'accept agreement' screen. */
class AgreementFrame : public Frame {
public:
	// Public (de)Constructors
	~AgreementFrame();
	AgreementFrame(Installer * installer, const HINSTANCE hInstance, const HWND parent, const RECT & rc);


	// Public Attributes
	Installer * m_installer = nullptr;
	HWND m_log = nullptr, m_checkNo = nullptr, m_checkYes = nullptr;
};

#endif // AGREEMENTFRAME_H