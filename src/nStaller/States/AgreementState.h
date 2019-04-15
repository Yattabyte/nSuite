#pragma once
#ifndef AGREEMENTSTATE_H
#define AGREEMENTSTATE_H

#include "State.h"


/** This state encapuslates the "Accept the license agreement" - Screen" state. */
class AgreementState : public FrameState {
public:
	// Public (de)Constructors
	~AgreementState();
	AgreementState(Installer * installer, const HINSTANCE hInstance, const HWND parent, const RECT & rc);


	// Public Interface Implementations
	virtual void enact();
	virtual void pressPrevious();
	virtual void pressNext();
	virtual void pressClose();


	// Public Attributes
	HWND m_log = nullptr, m_checkNo = nullptr, m_checkYes = nullptr;
	bool m_agrees = false;
};

#endif // AGREEMENTSTATE_H