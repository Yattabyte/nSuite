#pragma once
#ifndef UNINSTALL_H
#define UNINSTALL_H

#include "Screen.h"
#include <string>
#include <thread>


/** This state encapuslates the "Uninstalling - Screen" state. */
class Uninstall : public Screen {
public:
	// Public (de)Constructors
	~Uninstall();
	Uninstall(Uninstaller * uninstaller, const HINSTANCE hInstance, const HWND parent, const vec2 & pos, const vec2 & size);


	// Public Interface Implementations
	virtual void enact() override;
	virtual void paint() override;


	// Public Methods
	/** Switch to the next state. */
	void goFinish();


	// Public Attributes
	HWND m_hwndLog = nullptr, m_hwndPrgsBar = nullptr, m_btnFinish = nullptr;
	size_t m_logIndex = 0ull, m_taskIndex = 0ull;
};

#endif // UNINSTALL_H