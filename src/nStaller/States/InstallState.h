#pragma once
#ifndef INSTALLSTATE_H
#define INSTALLSTATE_H

#include "State.h"
#include <thread>


/** This state encapuslates the "Installing - Screen" state. */
class InstallState : public State {
public:
	// Public (de)Constructors
	~InstallState() = default;
	InstallState(Installer * installer);


	// Public Interface Implementations
	virtual void enact();
	virtual void pressPrevious();
	virtual void pressNext();
	virtual void pressClose();


private:
	// Private Attributes
	std::thread m_thread;
};

#endif // INSTALLSTATE_H