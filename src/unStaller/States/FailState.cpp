#include "FailState.h"
#include "Common.h"
#include "TaskLogger.h"
#include "../Uninstaller.h"
#include <ctime>
#include <fstream>
#include <shlobj.h>
#include <shlwapi.h>


static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

FailState::~FailState()
{
	UnregisterClass("FAIL_STATE", m_hinstance);
	DestroyWindow(m_hwnd);
	DestroyWindow(m_hwndLog);
	TaskLogger::RemoveCallback_TextAdded(m_logIndex);
}

FailState::FailState(Uninstaller * uninstaller, const HINSTANCE hInstance, const HWND parent, const RECT & rc)
	: FrameState(uninstaller)
{
	// Create window class
	m_hinstance = hInstance;
	m_wcex.cbSize = sizeof(WNDCLASSEX);
	m_wcex.style = CS_HREDRAW | CS_VREDRAW;
	m_wcex.lpfnWndProc = WndProc;
	m_wcex.cbClsExtra = 0;
	m_wcex.cbWndExtra = 0;
	m_wcex.hInstance = hInstance;
	m_wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	m_wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	m_wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	m_wcex.lpszMenuName = NULL;
	m_wcex.lpszClassName = "FAIL_STATE";
	m_wcex.hIconSm = LoadIcon(m_wcex.hInstance, IDI_APPLICATION);
	RegisterClassEx(&m_wcex);
	m_hwnd = CreateWindow("FAIL_STATE", "", WS_OVERLAPPED | WS_VISIBLE | WS_CHILD, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, parent, NULL, hInstance, NULL);
	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
	setVisible(false);

	// Create error log
	m_hwndLog = CreateWindowEx(WS_EX_CLIENTEDGE, "edit", 0, WS_VISIBLE | WS_OVERLAPPED | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 10, 50, (rc.right - rc.left) - 20, (rc.bottom - rc.top) - 100, m_hwnd, NULL, hInstance, NULL);
	SendMessage(m_hwndLog, EM_REPLACESEL, FALSE, (LPARAM)"Error Log:\r\n");
	m_logIndex = TaskLogger::AddCallback_TextAdded([&](const std::string & message) {
		SendMessage(m_hwndLog, EM_REPLACESEL, FALSE, (LPARAM)message.c_str());
	});
}

void FailState::enact()
{
	m_uninstaller->showButtons(false, false, true);
	m_uninstaller->enableButtons(false, false, true);

	// Dump error log to disk
	const auto dir = get_current_directory() + "\\error_log.txt";
	const auto t = std::time(0);
	char dateData[127];
	ctime_s(dateData, 127, &t);
	std::string logData("");

	// If the log doesn't exist, add header text
	if (!std::filesystem::exists(dir))
		logData += "Uninstaller error log:\r\n";

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

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//auto ptr = (FailState*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (message == WM_PAINT) {
		PAINTSTRUCT ps;
		Graphics graphics(BeginPaint(hWnd, &ps));

		// Draw Background
		LinearGradientBrush backgroundGradient(
			Point(0, 0),
			Point(0, 450),
			Color(50, 225, 25, 75),
			Color(255, 255, 255, 255)
		);
		graphics.FillRectangle(&backgroundGradient, 0, 0, 630, 450);

		// Preparing Fonts
		FontFamily  fontFamily(L"Segoe UI");
		Font        bigFont(&fontFamily, 25, FontStyleBold, UnitPixel);
		Font        regFont(&fontFamily, 14, FontStyleRegular, UnitPixel);
		SolidBrush  blueBrush(Color(255, 25, 125, 225));

		// Draw Text
		graphics.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);
		graphics.DrawString(L"Uninstallation Incomplete", -1, &bigFont, PointF{ 10, 10 }, &blueBrush);

		EndPaint(hWnd, &ps);
		return S_OK;
	}
	else if (message == WM_CTLCOLORSTATIC) {
		// Make log color white
		SetBkColor(HDC(wParam), RGB(255, 255, 255));
		return (LRESULT)GetStockObject(WHITE_BRUSH);
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}