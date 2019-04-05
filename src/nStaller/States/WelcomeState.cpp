#include "WelcomeState.h"
#include "DirectoryState.h"
#include "../Installer.h"


WelcomeState::WelcomeState(Installer * installer)
	: State(installer) {}

void WelcomeState::enact()
{
	m_installer->showFrame(Installer::FrameEnums::WELCOME_FRAME);
	m_installer->showButtons(false, true, true);	
}

void WelcomeState::pressPrevious()
{
	// Should never happen
}

void WelcomeState::pressNext()
{
	m_installer->setState(new DirectoryState(m_installer));
	delete this;
}

void WelcomeState::pressClose()
{
	// No new screen
	PostQuitMessage(0);
}