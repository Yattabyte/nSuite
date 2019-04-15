#pragma once
#ifndef AGREEMENTSTATE_H
#define AGREEMENTSTATE_H

#include "State.h"


/** This state encapuslates the "Accept the license agreement" - Screen" state. */
class AgreementState : public State {
public:
	// Public (de)Constructors
	~AgreementState() = default;
	AgreementState(Installer * installer);


	// Public Interface Implementations
	virtual void enact();
	virtual void pressPrevious();
	virtual void pressNext();
	virtual void pressClose();
};

#endif // AGREEMENTSTATE_H