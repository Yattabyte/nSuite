#include "UninstallState.h"
#include "Common.h"
#include "BufferTools.h"
#include "DirectoryTools.h"
#include "../Uninstaller.h"
#include <CommCtrl.h>


static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

UninstallState::~UninstallState()
{
	UnregisterClass("UNINSTALL_STATE", m_hinstance);
	DestroyWindow(m_hwnd);
	DestroyWindow(m_hwndLog);
	DestroyWindow(m_hwndPrgsBar);
	TaskLogger::RemoveCallback_TextAdded(m_logIndex);
	TaskLogger::RemoveCallback_ProgressUpdated(m_taskIndex);
}

UninstallState::UninstallState(Uninstaller * uninstaller, const HINSTANCE hInstance, const HWND parent, const RECT & rc)
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
	m_wcex.lpszClassName = "UNINSTALL_STATE";
	m_wcex.hIconSm = LoadIcon(m_wcex.hInstance, IDI_APPLICATION);
	RegisterClassEx(&m_wcex);
	m_hwnd = CreateWindow("UNINSTALL_STATE", "", WS_OVERLAPPED | WS_VISIBLE | WS_CHILD, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, parent, NULL, hInstance, NULL);
	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
	setVisible(false);

	// Create log box and progress bar
	m_hwndLog = CreateWindowEx(WS_EX_CLIENTEDGE, "edit", 0, WS_VISIBLE | WS_OVERLAPPED | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 10, 50, (rc.right - rc.left) - 20, (rc.bottom - rc.top) - 100, m_hwnd, NULL, hInstance, NULL);
	m_hwndPrgsBar = CreateWindowEx(WS_EX_CLIENTEDGE, PROGRESS_CLASS, 0, WS_CHILD | WS_VISIBLE | WS_OVERLAPPED | WS_DLGFRAME | WS_CLIPCHILDREN | PBS_SMOOTH, 10, (rc.bottom - rc.top) - 40, (rc.right - rc.left) - 70, 25, m_hwnd, NULL, hInstance, NULL);
	m_logIndex = TaskLogger::AddCallback_TextAdded([&](const std::string & message) {
		SendMessage(m_hwndLog, EM_REPLACESEL, FALSE, (LPARAM)message.c_str());
	});
	m_taskIndex = TaskLogger::AddCallback_ProgressUpdated([&](const size_t & position, const size_t & range) {
		SendMessage(m_hwndPrgsBar, PBM_SETRANGE32, 0, LPARAM(int_fast32_t(range)));
		SendMessage(m_hwndPrgsBar, PBM_SETPOS, WPARAM(int_fast32_t(position)), 0);
		m_progress = std::to_wstring(position == range ? 100 : int(std::floorf((float(position) / float(range)) * 100.0f))) + L"%";
		RECT rc = { 580, 410, 800, 450 };
		RedrawWindow(m_hwnd, &rc, NULL, RDW_INVALIDATE);
	});
}

void UninstallState::enact()
{
	m_uninstaller->showButtons(false, true, true);
	m_uninstaller->enableButtons(false, false, false);

	m_thread = std::thread([&]() {
		const auto directory = from_wideString(m_uninstaller->getDirectory());

		// Find all installed files
		const auto entries = get_file_paths(directory);

		// Find all shortcuts
		const auto desktopStrings = m_uninstaller->m_mfStrings[L"shortcut"], startmenuStrings = m_uninstaller->m_mfStrings[L"startmenu"];
		size_t numD = std::count(desktopStrings.begin(), desktopStrings.end(), L',') + 1ull, numS = std::count(startmenuStrings.begin(), startmenuStrings.end(), L',') + 1ull;
		std::vector<std::wstring> shortcuts_d, shortcuts_s;
		shortcuts_d.reserve(numD + numS);
		shortcuts_s.reserve(numD + numS);
		size_t last = 0;
		if (!desktopStrings.empty())
			for (size_t x = 0; x < numD; ++x) {
				// Find end of shortcut
				auto nextComma = desktopStrings.find(L',', last);
				if (nextComma == std::wstring::npos)
					nextComma = desktopStrings.size();

				// Find demarkation point where left half is the shortcut path, right half is the shortcut name
				shortcuts_d.push_back(desktopStrings.substr(last, nextComma - last));

				// Skip whitespace, find next element
				last = nextComma + 1ull;
				while (last < desktopStrings.size() && (desktopStrings[last] == L' ' || desktopStrings[last] == L'\r' || desktopStrings[last] == L'\t' || desktopStrings[last] == L'\n'))
					last++;
			}
		last = 0;
		if (!startmenuStrings.empty())
			for (size_t x = 0; x < numS; ++x) {
				// Find end of shortcut
				auto nextComma = startmenuStrings.find(L',', last);
				if (nextComma == std::wstring::npos)
					nextComma = startmenuStrings.size();

				// Find demarkation point where left half is the shortcut path, right half is the shortcut name
				shortcuts_s.push_back(startmenuStrings.substr(last, nextComma - last));

				// Skip whitespace, find next element
				last = nextComma + 1ull;
				while (last < startmenuStrings.size() && (startmenuStrings[last] == L' ' || startmenuStrings[last] == L'\r' || startmenuStrings[last] == L'\t' || startmenuStrings[last] == L'\n'))
					last++;
			}

		// Set progress bar range to include all files + shortcuts + 1 (cleanup step)
		TaskLogger::SetRange(entries.size() + shortcuts_d.size() + shortcuts_s.size() + 1);
		size_t progress = 0ull;

		// Remove all files in the installation folder, list them
		std::error_code er;
		if (!entries.size())
			TaskLogger::PushText("Already uninstalled / no files found.\r\n");
		else {
			for each (const auto & entry in entries) {
				TaskLogger::PushText("Deleting file: \"" + entry.path().string() + "\"\r\n");
				std::filesystem::remove(entry, er);
				TaskLogger::SetProgress(++progress);
			}
		}

		// Remove all shortcuts
		for each (const auto & shortcut in shortcuts_d) {
			const auto path = get_users_desktop() + "\\" + std::filesystem::path(shortcut).filename().string() + ".lnk";
			TaskLogger::PushText("Deleting desktop shortcut: \"" + path + "\"\r\n");
			std::filesystem::remove(path, er);
			progress++;
		}
		for each (const auto & shortcut in shortcuts_s) {
			const auto path = get_users_startmenu() + "\\" + std::filesystem::path(shortcut).filename().string() + ".lnk";
			TaskLogger::PushText("Deleting start-menu shortcut: \"" + path + "\"\r\n");
			std::filesystem::remove(path, er);
			progress++;
		}

		// Clean up whatever's left (empty folders)
		std::filesystem::remove_all(directory, er);
		TaskLogger::SetProgress(++progress);

		m_uninstaller->enableButtons(false, true, false);
	});
	m_thread.detach();
}

void UninstallState::pressPrevious()
{
	// Should never happen
}

void UninstallState::pressNext()
{
	m_uninstaller->setState(Uninstaller::FINISH_STATE);
}

void UninstallState::pressClose()
{
	// Should never happen
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	auto ptr = (UninstallState*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (message == WM_PAINT) {
		PAINTSTRUCT ps;
		Graphics graphics(BeginPaint(hWnd, &ps));

		// Draw Background
		LinearGradientBrush backgroundGradient(
			Point(0, 0),
			Point(0, 450),
			Color(50, 25, 125, 225),
			Color(255, 255, 255, 255)
		);
		graphics.FillRectangle(&backgroundGradient, 0, 0, 630, 450);

		// Preparing Fonts
		FontFamily  fontFamily(L"Segoe UI");
		Font        bigFont(&fontFamily, 25, FontStyleBold, UnitPixel);
		Font        regBoldFont(&fontFamily, 14, FontStyleBold, UnitPixel);
		SolidBrush  blueBrush(Color(255, 25, 125, 225));
		SolidBrush  blackBrush(Color(255, 0, 0, 0));

		// Draw Text
		graphics.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);
		graphics.DrawString(L"Uninstalling", -1, &bigFont, PointF{ 10, 10 }, &blueBrush);
		graphics.DrawString(ptr->m_progress.c_str(), -1, &regBoldFont, PointF{ 580, 412 }, &blackBrush);

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