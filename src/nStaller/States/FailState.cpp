#include "FailState.h"
#include "../Installer.h"


FailState::FailState(Installer * installer)
	: State(installer) {}

void FailState::enact()
{
	m_installer->showFrame(Installer::FrameEnums::FAIL_FRAME);
	m_installer->showButtons(false, false, true);
	m_installer->enableButtons(false, false, true);
}

void FailState::pressPrevious()
{
	// Should never happen
}

void FailState::pressNext()
{
	// Should never happen
}

void FailState::pressClose()
{
	// No new screen
	PostQuitMessage(0);
}