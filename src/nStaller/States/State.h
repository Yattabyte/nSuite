#pragma once
#ifndef STATE_H
#define STATE_H


class Installer;

/***/
class State {
public:
	// Public (de)Constructors
	State(Installer * installer) : m_installer(installer) {}


	// Public Interface Declarations
	/** Trigger this state to perform its screen action. */
	virtual void enact() = 0;
	/** Cause this state to process the "previous" action. */
	virtual void pressPrevious() = 0;
	/** Cause this state to process the "next" action. */
	virtual void pressNext() = 0;
	/** Cause this state to process the "close" action. */
	virtual void pressClose() = 0;


	// Public Attributes
	Installer * m_installer = nullptr;
};

#endif // STATE_H