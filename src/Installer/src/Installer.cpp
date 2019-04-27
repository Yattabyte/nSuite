#include "Installer.h"
#include "StringConversions.h"
#include "DirectoryTools.h"
#include "BufferTools.h"
#include "Log.h"
#include <fstream>
#include <regex>
#include <sstream>
#pragma warning(push)
#pragma warning(disable:4458)
#include <gdiplus.h>
#pragma warning(pop)

// States used in this GUI application
#include "Screens/Welcome.h"
#include "Screens/Agreement.h"
#include "Screens/Directory.h"
#include "Screens/Install.h"
#include "Screens/Finish.h"
#include "Screens/Fail.h"


static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE, _In_ LPSTR, _In_ int)
{
	CoInitialize(NULL);
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	Installer installer(hInstance);

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

Installer::Installer() 
	: m_archive(IDR_ARCHIVE, "ARCHIVE"), m_manifest(IDR_MANIFEST, "MANIFEST"), m_threader(1ull)
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

Installer::Installer(const HINSTANCE hInstance) : Installer()
{
	// Get user's program files directory
	TCHAR pf[MAX_PATH];
	SHGetSpecialFolderPath(0, pf, CSIDL_PROGRAM_FILES, FALSE);
	setDirectory(std::string(pf));
	
	// Check archive integrity
	bool success = true;
	if (!m_archive.exists()) {
		Log::PushText("Critical failure: archive doesn't exist!\r\n");
		success = false;
	}
	else {
		// Parse the header
		char * packagedData(nullptr);
		size_t packagedDataSize(0ull);
		bool result = DRT::ParseHeader(reinterpret_cast<char*>(m_archive.getPtr()), m_archive.getSize(), m_packageName, &packagedData, packagedDataSize);
		if (!result) {
			Log::PushText("Critical failure: cannot parse archive package's header.\r\n");
			success = false;
		}
		else {
			/////////////////////////////////////////////////
			Buffer DELETE_ME(packagedData, packagedDataSize);
			/////////////////////////////////////////////////
			Buffer packageBuffer;
			result = BFT::ParseHeader(DELETE_ME, m_maxSize, packageBuffer);
			if (!result) {
				Log::PushText("Critical failure: cannot parse archive's packaged content's header.\r\n");
				success = false;
			}
			else {
				// If no name is found, use the package name (if available)
				if (m_mfStrings[L"name"].empty() && !m_packageName.empty())
					m_mfStrings[L"name"] = to_wideString(m_packageName);
			}
		}
	}
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
	wcex.lpszClassName = "Installer";
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
	if (!RegisterClassEx(&wcex)) {
		Log::PushText("Critical failure: could not create main window.\r\n");
		success = false;
	}
	else {
		// Create window
		m_hwnd = CreateWindowW(
			L"Installer",(m_mfStrings[L"name"] + L" Installer").c_str(),
			WS_OVERLAPPED | WS_VISIBLE | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
			CW_USEDEFAULT, CW_USEDEFAULT,
			800, 500,
			NULL, NULL, hInstance, NULL
		);
		SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);		
		auto dwStyle = (DWORD)GetWindowLongPtr(m_hwnd, GWL_STYLE);
		auto dwExStyle = (DWORD)GetWindowLongPtr(m_hwnd, GWL_EXSTYLE);
		RECT rc = { 0, 0, 800, 500 };
		ShowWindow(m_hwnd, true);
		UpdateWindow(m_hwnd);
		AdjustWindowRectEx(&rc, dwStyle, false, dwExStyle);
		SetWindowPos(m_hwnd, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_NOMOVE);

		// The portions of the screen that change based on input
		m_screens[WELCOME_SCREEN] = new Welcome(this, hInstance, m_hwnd, { 170,0 }, { 630, 500 });
		m_screens[AGREEMENT_SCREEN] = new Agreement(this, hInstance, m_hwnd, { 170,0 }, { 630, 500 });
		m_screens[DIRECTORY_SCREEN] = new Directory(this, hInstance, m_hwnd, { 170,0 }, { 630, 500 });
		m_screens[INSTALL_SCREEN] = new Install(this, hInstance, m_hwnd, { 170,0 }, { 630, 500 });
		m_screens[FINISH_SCREEN] = new Finish(this, hInstance, m_hwnd, { 170,0 }, { 630, 500 });
		m_screens[FAIL_SCREEN] = new Fail(this, hInstance, m_hwnd, { 170,0 }, { 630, 500 });
		setScreen(WELCOME_SCREEN);
	}

#ifndef DEBUG
	if (!success)
		invalidate();
#endif
}

void Installer::invalidate()
{
	setScreen(FAIL_SCREEN);
	m_valid = false;
}

void Installer::setScreen(const ScreenEnums & screenIndex)
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

std::string Installer::getDirectory() const
{
	return m_directory;
}

void Installer::setDirectory(const std::string & directory)
{
	m_directory = directory;
	try {
		const auto spaceInfo = std::filesystem::space(std::filesystem::path(getDirectory()).root_path());
		m_capacity = spaceInfo.capacity;
		m_available = spaceInfo.available;
	}
	catch (std::filesystem::filesystem_error &) {
		m_capacity = 0ull;
		m_available = 0ull;
	}
}

size_t Installer::getDirectorySizeCapacity() const
{
	return m_capacity;
}

size_t Installer::getDirectorySizeAvailable() const
{
	return m_available;
}

size_t Installer::getDirectorySizeRequired() const
{
	return m_maxSize;
}

std::string Installer::getPackageName() const
{
	return m_packageName;
}

void Installer::beginInstallation()
{
	m_threader.addJob([&]() {
		// Acquire the uninstaller resource
		Resource uninstaller(IDR_UNINSTALLER, "UNINSTALLER"), manifest(IDR_MANIFEST, "MANIFEST");
		if (!uninstaller.exists()) {
			Log::PushText("Cannot access installer resource, aborting...\r\n");
			setScreen(Installer::FAIL_SCREEN);
		}
		else {
			// Unpackage using the rest of the resource file
			auto directory = DRT::SanitizePath(getDirectory());
			if (!DRT::DecompressDirectory(directory, reinterpret_cast<char*>(m_archive.getPtr()), m_archive.getSize()))
				invalidate();
			else {
				// Write uninstaller to disk
				const auto fullDirectory = directory + "\\" + m_packageName;
				const auto uninstallerPath = fullDirectory + "\\uninstaller.exe";
				std::filesystem::create_directories(std::filesystem::path(uninstallerPath).parent_path());
				std::ofstream file(uninstallerPath, std::ios::binary | std::ios::out);
				if (!file.is_open()) {
					Log::PushText("Cannot write uninstaller to disk, aborting...\r\n");
					invalidate();
				}
				Log::PushText("Uninstaller: \"" + uninstallerPath + "\"\r\n");
				file.write(reinterpret_cast<char*>(uninstaller.getPtr()), (std::streamsize)uninstaller.getSize());
				file.close();

				// Update uninstaller's resources
				std::string newDir = std::regex_replace(fullDirectory, std::regex("\\\\"), "\\\\");
				const std::string newManifest(
					std::string(reinterpret_cast<char*>(manifest.getPtr()), manifest.getSize())
					+ "\r\ndirectory \"" + newDir + "\""
				);
				auto handle = BeginUpdateResource(uninstallerPath.c_str(), false);
				if (!(bool)UpdateResource(handle, "MANIFEST", MAKEINTRESOURCE(IDR_MANIFEST), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPVOID)newManifest.c_str(), (DWORD)newManifest.size())) {
					Log::PushText("Cannot write manifest contents to the uninstaller, aborting...\r\n");
					invalidate();
				}
				EndUpdateResource(handle, FALSE);

				// Add uninstaller to system registry
				HKEY hkey;
				if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, (L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + m_mfStrings[L"name"]).c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL) == ERROR_SUCCESS) {
					auto
						name = from_wideString(m_mfStrings[L"name"]),
						version = from_wideString(m_mfStrings[L"version"]),
						publisher = from_wideString(m_mfStrings[L"publisher"]),
						icon = from_wideString(m_mfStrings[L"icon"]);
					DWORD ONE = 1, SIZE = (DWORD)(m_maxSize/1024ull);
					if (icon.empty())
						icon = uninstallerPath;
					else
						icon = fullDirectory + icon;
					RegSetKeyValueA(hkey, 0, "UninstallString", REG_SZ, (LPCVOID)uninstallerPath.c_str(), (DWORD)uninstallerPath.size());
					RegSetKeyValueA(hkey, 0, "DisplayIcon", REG_SZ, (LPCVOID)icon.c_str(), (DWORD)icon.size());
					RegSetKeyValueA(hkey, 0, "DisplayName", REG_SZ, (LPCVOID)name.c_str(), (DWORD)name.size());
					RegSetKeyValueA(hkey, 0, "DisplayVersion", REG_SZ, (LPCVOID)version.c_str(), (DWORD)version.size());
					RegSetKeyValueA(hkey, 0, "InstallLocation", REG_SZ, (LPCVOID)fullDirectory.c_str(), (DWORD)fullDirectory.size());
					RegSetKeyValueA(hkey, 0, "Publisher", REG_SZ, (LPCVOID)publisher.c_str(), (DWORD)publisher.size());
					RegSetKeyValueA(hkey, 0, "NoModify", REG_DWORD, (LPCVOID)&ONE, (DWORD)sizeof(DWORD));
					RegSetKeyValueA(hkey, 0, "NoRepair", REG_DWORD, (LPCVOID)&ONE, (DWORD)sizeof(DWORD));
					RegSetKeyValueA(hkey, 0, "EstimatedSize", REG_DWORD, (LPCVOID)&SIZE, (DWORD)sizeof(DWORD));
				}
				RegCloseKey(hkey);
			}
		}
	});
}

void Installer::dumpErrorLog()
{
	// Dump error log to disk
	const auto dir = DRT::GetRunningDirectory() + "\\error_log.txt";
	const auto t = std::time(0);
	char dateData[127];
	ctime_s(dateData, 127, &t);
	std::string logData("");

	// If the log doesn't exist, add header text
	if (!std::filesystem::exists(dir))
		logData += "Installer error log:\r\n";

	// Add remaining log data
	logData += std::string(dateData) + Log::PullText() + "\r\n";

	// Try to create the file
	std::filesystem::create_directories(std::filesystem::path(dir).parent_path());
	std::ofstream file(dir, std::ios::binary | std::ios::out | std::ios::app);
	if (!file.is_open())
		Log::PushText("Cannot dump error log to disk...\r\n");
	else
		file.write(logData.c_str(), (std::streamsize)logData.size());
	file.close();
}

void Installer::paint()
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
	constexpr static wchar_t* step_labels[] = { L"Welcome", L"EULA", L"Directory", L"Install", L"Finish" };
	FontFamily  fontFamily(L"Segoe UI");
	Font        font(&fontFamily, 15, FontStyleBold, UnitPixel);
	REAL vertical_offset = 15;
	const auto frameIndex = (int)m_currentIndex;
	for (int x = 0; x < 5; ++x) {
		// Draw Circle
		auto color = x < frameIndex ? Color(255, 100, 100, 100) : x == frameIndex ? Color(255, 25, 225, 125) : Color(255, 255, 255, 255);
		if (x == 4 && frameIndex == 5)
			color = Color(255, 225, 25, 75);
		const SolidBrush brush(color);
		Pen pen(color);
		graphics.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);
		graphics.DrawEllipse(&pen, 20, (int)vertical_offset, 20, 20);
		graphics.FillEllipse(&brush, 20, (int)vertical_offset, 20, 20);

		// Draw Text
		graphics.DrawString(step_labels[x], -1, &font, PointF{ 50, vertical_offset }, &brush);

		if (x == 3)
			vertical_offset = 460;
		else
			vertical_offset += 50;
	}

	EndPaint(m_hwnd, &ps);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	const auto ptr = (Installer*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (message == WM_PAINT)
		ptr->paint();
	else if (message == WM_DESTROY)
		PostQuitMessage(0);
	return DefWindowProc(hWnd, message, wParam, lParam);
}