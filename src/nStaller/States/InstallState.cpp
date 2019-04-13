#include "InstallState.h"
#include "FinishState.h"
#include "Common.h"
#include "BufferTools.h"
#include "DirectoryTools.h"
#include "../Installer.h"


InstallState::InstallState(Installer * installer)
	: State(installer) {}

void InstallState::enact()
{
	m_installer->showFrame(Installer::FrameEnums::INSTALL_FRAME);
	m_installer->showButtons(true, true, true);
	m_installer->enableButtons(false, false, false);

	m_thread = std::thread([&]() {
		// Unpackage using the rest of the resource file
		size_t byteCount(0ull), fileCount(0ull);
		auto directory = m_installer->getDirectory();
		sanitize_path(directory);
		if (!DRT::DecompressDirectory(directory, m_installer->getPackagePointer(), m_installer->getCompressedPackageSize(), byteCount, fileCount))
			m_installer->invalidate();
		else
			m_installer->enableButtons(false, true, false);
	});
	m_thread.detach();
}

void InstallState::pressPrevious()
{
	// Should never happen
}

void InstallState::pressNext()
{
	m_installer->setState(new FinishState(m_installer));
}

void InstallState::pressClose()
{
	// Should never happen
}