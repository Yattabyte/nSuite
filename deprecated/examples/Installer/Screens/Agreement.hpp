#pragma once
#ifndef AGREEMENT_SCREEN_H
#define AGREEMENT_SCREEN_H

#include "Screen.h"


/** This state encapsulates the "Accept the license agreement" - Screen" state. */
class Agreement_Screen : public Screen {
public:
	// Public (de)Constructors
	~Agreement_Screen();
	Agreement_Screen(Installer* Installer, const HINSTANCE hInstance, const HWND parent, const vec2& pos, const vec2& size);


	// Public Interface Implementations
	virtual void enact() override;
	virtual void paint() override;


	// Public Methods
	/** Check the 'I agree' button. */
	void checkYes();
	/** Switch to the previous state. */
	void goPrevious();
	/** Switch to the next state. */
	void goNext();
	/** Switch to the cancel state. */
	static void goCancel();


	// Public Attributes
	HWND m_log = nullptr, m_checkYes = nullptr, m_btnPrev = nullptr, m_btnNext = nullptr, m_btnCancel = nullptr;
	bool m_agrees = false;
};

#endif // AGREEMENT_SCREEN_H