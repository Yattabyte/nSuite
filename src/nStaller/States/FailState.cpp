#include "FailState.h"
#include "Common.h"
#include "TaskLogger.h"
#include "../Installer.h"
#include <ctime>
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
	const auto t = std::time(0);
	char dateData[127];
	ctime_s(dateData, 127, &t);
	std::string logData("");

	// If the log doesn't exist, add header text
	if (!std::filesystem::exists(dir))
		logData += "Installer error log:\r\n";

	// Add remaining log data
	logData += std::string(dateData) + TaskLogger::PullText() + "\r\n";

	// Try to create the file
	std::filesystem::create_directories(std::filesystem::path(dir).parent_path());
	std::ofstream file(dir, std::ios::binary | std::ios::out | std::ios::app);
	if (!file.is_open())
		TaskLogger::PushText("Cannot dump error log to disk...\r\n");
	else 
		file.write(logData.c_str(), (std::streamsize)logData.size());
	file.close();	
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