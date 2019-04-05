#include "DirectoryState.h"
#include "WelcomeState.h"
#include "InstallState.h"
#include "../Installer.h"


DirectoryState::DirectoryState(Installer * installer)
	: State(installer) {}

void DirectoryState::enact()
{
	m_installer->showFrame(Installer::FrameEnums::DIRECTORY_FRAME);
	m_installer->showButtons(true, true, true);
}

void DirectoryState::pressPrevious()
{
	m_installer->setState(new WelcomeState(m_installer));
}

void DirectoryState::pressNext()
{
	m_installer->setState(new InstallState(m_installer));
}

void DirectoryState::pressClose()
{
	// No new screen
	PostQuitMessage(0);
}