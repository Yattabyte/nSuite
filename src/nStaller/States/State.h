#pragma once
#ifndef STATE_H
#define STATE_H


class Installer;

/***/
class State {
public:
	// Public (de)Constructors
	State(Installer * installer) : m_installer(installer) {}


	// Public Methods
	virtual void enact() = 0;
	virtual void pressPrevious() = 0;
	virtual void pressNext() = 0;
	virtual void pressClose() = 0;


	// Public Attributes
	Installer * m_installer = nullptr;
};

#endif // STATE_H