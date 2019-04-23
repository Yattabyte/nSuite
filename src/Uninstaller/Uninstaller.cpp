#include "Uninstaller.h"
#include "Common.h"
#include "DirectoryTools.h"
#include "TaskLogger.h"
#include <CommCtrl.h>
#include <fstream>
#include <regex>
#include <Shlobj.h>
#include <sstream>
#pragma warning(push)
#pragma warning(disable:4458)
#include <gdiplus.h>
#pragma warning(pop)

// Screens used in this GUI application
#include "Screens/Welcome.h"
#include "Screens/Uninstall.h"
#include "Screens/Finish.h"
#include "Screens/Fail.h"


static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE, _In_ LPSTR, _In_ int)
{
	CoInitialize(NULL);
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	Uninstaller uninstaller(hInstance);

	// Main message loop:
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Close
	CoUninitialize();
	return (int)msg.wParam;
}

Uninstaller::Uninstaller() 
	: m_manifest(IDR_MANIFEST, "MANIFEST"), m_threader(1ull)
{
	// Process manifest
	if (m_manifest.exists()) {
		// Create a string stream of the manifest file
		std::wstringstream ss;
		ss << reinterpret_cast<char*>(m_manifest.getPtr());

		// Cycle through every line, inserting attributes into the manifest map
		std::wstring attrib, value;
		while (ss >> attrib && ss >> std::quoted(value)) {
			wchar_t * k = new wchar_t[attrib.length() + 1];
			wcscpy_s(k, attrib.length() + 1, attrib.data());
			m_mfStrings[k] = value;
		}
	}
}

Uninstaller::Uninstaller(const HINSTANCE hInstance) : Uninstaller()
{
	// Ensure that a manifest exists
	bool success = true;
	if (!m_manifest.exists()) {
		TaskLogger::PushText("Critical failure: uninstaller manifest doesn't exist!\r\n");
		success = false;
	}

	// Acquire the installation directory
	m_directory = m_mfStrings[L"directory"];
	if (m_directory.empty())
		m_directory = to_wideString(get_current_directory());

	// Create window class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, (LPCSTR)IDI_ICON1);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = "Uninstaller";
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
	if (!RegisterClassEx(&wcex)) {
		TaskLogger::PushText("Critical failure: could not create main window.\r\n");
		success = false;
	}
	else {
		m_hwnd = CreateWindowW(
			L"Uninstaller",(m_mfStrings[L"name"] + L" Uninstaller").c_str(),
			WS_OVERLAPPED | WS_VISIBLE | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
			CW_USEDEFAULT, CW_USEDEFAULT,
			800, 500,
			NULL, NULL, hInstance, NULL
		);

		// Create
		SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);auto dwStyle = (DWORD)GetWindowLongPtr(m_hwnd, GWL_STYLE);
		auto dwExStyle = (DWORD)GetWindowLongPtr(m_hwnd, GWL_EXSTYLE);
		RECT rc = { 0, 0, 800, 500 };
		ShowWindow(m_hwnd, true);
		UpdateWindow(m_hwnd);
		AdjustWindowRectEx(&rc, dwStyle, false, dwExStyle);
		SetWindowPos(m_hwnd, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_NOMOVE);

		// The portions of the screen that change based on input
		m_screens[WELCOME_SCREEN] = new Welcome(this, hInstance, m_hwnd, { 170,0 }, { 630, 500 });
		m_screens[UNINSTALL_SCREEN] = new Uninstall(this, hInstance, m_hwnd, { 170,0 }, { 630, 500 });
		m_screens[FINISH_SCREEN] = new Finish(this, hInstance, m_hwnd, { 170,0 }, { 630, 500 });
		m_screens[FAIL_SCREEN] = new Fail(this, hInstance, m_hwnd, { 170,0 }, { 630, 500 });
		setScreen(WELCOME_SCREEN);
	}

#ifndef DEBUG
	if (!success)
		invalidate();
#endif
}

void Uninstaller::invalidate()
{
	setScreen(FAIL_SCREEN);
	m_valid = false;
}

void Uninstaller::setScreen(const ScreenEnums & screenIndex)
{	
	if (m_valid) {
		m_screens[m_currentIndex]->setVisible(false);
		m_screens[screenIndex]->enact();
		m_screens[screenIndex]->setVisible(true);
		m_currentIndex = screenIndex;
		RECT rc = { 0, 0, 160, 500 };
		RedrawWindow(m_hwnd, &rc, NULL, RDW_INVALIDATE);
	}
}

std::wstring Uninstaller::getDirectory() const
{
	return m_directory;
}

void Uninstaller::beginUninstallation()
{
	m_threader.addJob([&]() {
		// Find all installed files
		const auto directory = sanitize_path(from_wideString(m_directory));
		const auto entries = get_file_paths(directory);

		// Find all shortcuts
		const auto desktopStrings = m_mfStrings[L"shortcut"], startmenuStrings = m_mfStrings[L"startmenu"];
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
		TaskLogger::SetRange(entries.size() + shortcuts_d.size() + shortcuts_s.size() + 2);
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

		// Remove registry entry for this uninstaller		
		RegDeleteKeyExW(HKEY_LOCAL_MACHINE, (L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + m_mfStrings[L"name"]).c_str(), KEY_ALL_ACCESS, NULL);		
		TaskLogger::SetProgress(++progress);
	});
}

void Uninstaller::dumpErrorLog()
{
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

void Uninstaller::paint()
{
	PAINTSTRUCT ps;
	Graphics graphics(BeginPaint(m_hwnd, &ps));

	// Draw Background
	const LinearGradientBrush backgroundGradient1(
		Point(0, 0),
		Point(0, 500),
		Color(255, 25, 25, 25),
		Color(255, 75, 75, 75)
	);
	graphics.FillRectangle(&backgroundGradient1, 0, 0, 170, 500);

	// Draw Steps
	const SolidBrush lineBrush(Color(255, 100, 100, 100));
	graphics.FillRectangle(&lineBrush, 28, 0, 5, 500);
	constexpr static wchar_t* step_labels[] = { L"Welcome", L"Uninstall", L"Finish" };
	FontFamily  fontFamily(L"Segoe UI");
	Font        font(&fontFamily, 15, FontStyleBold, UnitPixel);
	REAL vertical_offset = 15;
	const auto frameIndex = (int)m_currentIndex;
	for (int x = 0; x < 3; ++x) {
		// Draw Circle
		auto color = x < frameIndex ? Color(255, 100, 100, 100) : x == frameIndex ? Color(255, 25, 225, 125) : Color(255, 255, 255, 255);
		if (x == 2 && frameIndex == 3)
			color = Color(255, 225, 25, 75);
		const SolidBrush brush(color);
		Pen pen(color);
		graphics.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);
		graphics.DrawEllipse(&pen, 20, (int)vertical_offset, 20, 20);
		graphics.FillEllipse(&brush, 20, (int)vertical_offset, 20, 20);

		// Draw Text
		graphics.DrawString(step_labels[x], -1, &font, PointF{ 50, vertical_offset }, &brush);

		if (x == 1)
			vertical_offset = 460;
		else
			vertical_offset += 50;
	}

	EndPaint(m_hwnd, &ps);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	const auto ptr = (Uninstaller*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (message == WM_PAINT)
		ptr->paint();
	else if (message == WM_DESTROY)
		PostQuitMessage(0);	
	return DefWindowProc(hWnd, message, wParam, lParam);
}