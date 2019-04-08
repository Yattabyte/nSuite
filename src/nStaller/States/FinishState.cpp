#include "FinishState.h"
#include "../Installer.h"


FinishState::FinishState(Installer * installer)
	: State(installer) {}

void FinishState::enact()
{
	m_installer->showFrame(Installer::FrameEnums::FINISH_FRAME);
	m_installer->showButtons(false, false, true);
	m_installer->enableButtons(false, false, true);
	m_installer->finish();
}

void FinishState::pressPrevious()
{
	// Should never happen
}

void FinishState::pressNext()
{
	// Should never happen
}

void FinishState::pressClose()
{
	// No new screen
	PostQuitMessage(0);
}