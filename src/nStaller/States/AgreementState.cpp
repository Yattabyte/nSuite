#include "AgreementState.h"
#include "WelcomeState.h"
#include "DirectoryState.h"
#include "../Installer.h"


AgreementState::AgreementState(Installer * installer)
	: State(installer) {}

void AgreementState::enact()
{
	m_installer->showFrame(Installer::FrameEnums::AGREEMENT_FRAME);
	m_installer->showButtons(true, true, true);
}

void AgreementState::pressPrevious()
{
	m_installer->setState(new WelcomeState(m_installer));
}

void AgreementState::pressNext()
{
	auto directory = m_installer->getDirectory();
	
	if (directory == "" || directory == " " || directory.length() < 3)
		MessageBox(
			NULL,
			"Please enter a valid directory before proceeding.",
			"Invalid path!",
			MB_OK | MB_ICONERROR | MB_TASKMODAL
		);
	else
		m_installer->setState(new DirectoryState(m_installer));
}

void AgreementState::pressClose()
{
	// No new screen
	PostQuitMessage(0);
}