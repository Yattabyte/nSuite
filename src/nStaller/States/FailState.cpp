#include "FailState.h"
#include "Common.h"
#include "TaskLogger.h"
#include "../Installer.h"
#include <fstream>


FailState::FailState(Installer * installer)
	: State(installer) {}

void FailState::enact()
{
	m_installer->showFrame(Installer::FrameEnums::FAIL_FRAME);
	m_installer->showButtons(false, false, true);
	m_installer->enableButtons(false, false, true);

	// Dump error log to disk
	const auto dir = get_current_directory() + "\\error_log.txt";
	const auto data = "Installer error log:\r\n" + TaskLogger::PullText();

	std::filesystem::create_directories(std::filesystem::path(dir).parent_path());
	std::ofstream file(dir, std::ios::binary | std::ios::out);
	if (!file.is_open())
		TaskLogger::PushText("Cannot dump error log to disk...\r\n");
	else {
		file.write(data.c_str(), (std::streamsize)data.size());
		file.close();
	}
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