#include "Installer.h"
#include "StringConversions.h"
#include "Directory.h"
#include "Log.h"
#include <filesystem>
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


static LRESULT CALLBACK WndProc(HWND /*hWnd*/, UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/);

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE /*unused*/, _In_ LPSTR /*unused*/, _In_ int /*unused*/)
{
	CoInitialize(nullptr);
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
	Installer installer(hInstance);

	// Main message loop:
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Close
	CoUninitialize();
	return (int)msg.wParam;
}

Installer::Installer()
	: m_archive(IDR_ARCHIVE, "ARCHIVE"), m_manifest(IDR_MANIFEST, "MANIFEST"), m_threader(1ULL)
{
	// Process manifest
	if (m_manifest.exists()) {
		// Create a string stream of the manifest file
		std::wstringstream ss;
		ss << reinterpret_cast<char*>(m_manifest.getPtr());

		// Cycle through every line, inserting attributes into the manifest map
		std::wstring attrib;
		std::wstring value;
		while (ss >> attrib && ss >> std::quoted(value)) {
			auto* k = new wchar_t[attrib.length() + 1];
			wcscpy_s(k, attrib.length() + 1, attrib.data());
			m_mfStrings[k] = value;
		}
	}
}

Installer::Installer(const HINSTANCE hInstance) : Installer()
{
	// Get user's program files directory
	TCHAR pf[MAX_PATH];
	SHGetSpecialFolderPath(nullptr, pf, CSIDL_PROGRAM_FILES, FALSE);
	setDirectory(std::string(pf));

	// Check archive integrity
	bool success_archive = false;
	if (!m_archive.exists())
		NST::Log::PushText("Critical failure: archive doesn't exist!\r\n");
	else {
		// Read in header
		NST::Directory::PackageHeader packageHeader;
		std::byte* dataPtr(nullptr);
		size_t dataSize(0ULL);
		NST::Buffer(m_archive.getPtr(), m_archive.getSize(), false).readHeader(&packageHeader, &dataPtr, dataSize);

		// Ensure header title matches
		if (!packageHeader.isValid())
			NST::Log::PushText("Critical failure: cannot parse packaged content's header!\r\n");
		else {
			m_packageName = packageHeader.m_folderName;
			NST::Buffer::CompressionHeader header;
			NST::Buffer(dataPtr, dataSize, false).readHeader(&header, &dataPtr, dataSize);

			// Ensure header title matches
			if (!header.isValid())
				NST::Log::PushText("Critical failure: cannot parse archive's packaged content's header!\r\n");
			else {
				// Get header payload -> uncompressed size
				m_maxSize = header.m_uncompressedSize;

				// If no name is found, use the package name (if available)
				if (m_mfStrings[L"name"].empty() && !m_packageName.empty())
					m_mfStrings[L"name"] = NST::to_wideString(m_packageName);

				success_archive = true;
			}
		}
	}
	// Create window class
	bool success_window = false;
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, (LPCSTR)IDI_ICON1);
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = "Installer";
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
	if (!RegisterClassEx(&wcex))
		NST::Log::PushText("Critical failure: could not create main window!\r\n");
	else {
		// Create window
		m_hwnd = CreateWindowW(
			L"Installer", (m_mfStrings[L"name"] + L" Installer").c_str(),
			WS_OVERLAPPED | WS_VISIBLE | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
			CW_USEDEFAULT, CW_USEDEFAULT,
			800, 500,
			nullptr, nullptr, hInstance, nullptr
		);
		SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
		auto dwStyle = (DWORD)GetWindowLongPtr(m_hwnd, GWL_STYLE);
		auto dwExStyle = (DWORD)GetWindowLongPtr(m_hwnd, GWL_EXSTYLE);
		RECT rc = { 0, 0, 800, 500 };
		ShowWindow(m_hwnd, 1);
		UpdateWindow(m_hwnd);
		AdjustWindowRectEx(&rc, dwStyle, 0, dwExStyle);
		SetWindowPos(m_hwnd, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_NOMOVE);

		// The portions of the screen that change based on input
		m_screens[(int)ScreenEnums::WELCOME_SCREEN] = new Welcome_Screen(this, hInstance, m_hwnd, { 170,0 }, { 630, 500 });
		m_screens[(int)ScreenEnums::AGREEMENT_SCREEN] = new Agreement_Screen(this, hInstance, m_hwnd, { 170,0 }, { 630, 500 });
		m_screens[(int)ScreenEnums::DIRECTORY_SCREEN] = new Directory_Screen(this, hInstance, m_hwnd, { 170,0 }, { 630, 500 });
		m_screens[(int)ScreenEnums::INSTALL_SCREEN] = new Install_Screen(this, hInstance, m_hwnd, { 170,0 }, { 630, 500 });
		m_screens[(int)ScreenEnums::FINISH_SCREEN] = new Finish_Screen(this, hInstance, m_hwnd, { 170,0 }, { 630, 500 });
		m_screens[(int)ScreenEnums::FAIL_SCREEN] = new Fail_Screen(this, hInstance, m_hwnd, { 170,0 }, { 630, 500 });
		setScreen(ScreenEnums::WELCOME_SCREEN);

		success_window = true;
	}

#ifndef DEBUG
	if (!success_archive || !success_window)
		invalidate();
#endif
}

void Installer::invalidate()
{
	setScreen(ScreenEnums::FAIL_SCREEN);
	m_valid = false;
}

void Installer::setScreen(const ScreenEnums& screenIndex)
{
	if (m_valid) {
		m_screens[(int)m_currentIndex]->setVisible(false);
		m_screens[(int)screenIndex]->enact();
		m_screens[(int)screenIndex]->setVisible(true);
		m_currentIndex = screenIndex;
		RECT rc = { 0, 0, 160, 500 };
		RedrawWindow(m_hwnd, &rc, nullptr, RDW_INVALIDATE);
	}
}

std::string Installer::getDirectory() const
{
	return m_directory;
}

void Installer::setDirectory(const std::string& directory)
{
	m_directory = directory;
	try {
		const auto spaceInfo = std::filesystem::space(std::filesystem::path(getDirectory()).root_path());
		m_capacity = spaceInfo.capacity;
		m_available = spaceInfo.available;
	}
	catch (std::filesystem::filesystem_error&) {
		m_capacity = 0ULL;
		m_available = 0ULL;
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
		NST::Resource uninstaller(IDR_UNINSTALLER, "UNINSTALLER");
		NST::Resource manifest(IDR_MANIFEST, "MANIFEST");
		if (!uninstaller.exists()) {
			NST::Log::PushText("Cannot access installer resource, aborting...\r\n");
			setScreen(Installer::ScreenEnums::FAIL_SCREEN);
		}
		else {
			// Un-package using the rest of the resource file
			const auto directory = NST::Directory::SanitizePath(getDirectory());
			const auto virtual_directory = NST::Directory(NST::Buffer(m_archive.getPtr(), m_archive.getSize(), false), directory);
			if (!virtual_directory.apply_folder())
				invalidate();
			else {
				// Write uninstaller to disk
				const auto fullDirectory = directory + "\\" + m_packageName;
				const auto uninstallerPath = fullDirectory + "\\uninstaller.exe";
				std::filesystem::create_directories(std::filesystem::path(uninstallerPath).parent_path());
				std::ofstream file(uninstallerPath, std::ios::binary | std::ios::out);
				if (!file.is_open()) {
					NST::Log::PushText("Cannot write uninstaller to disk, aborting...\r\n");
					invalidate();
				}
				NST::Log::PushText("Uninstaller: \"" + uninstallerPath + "\"\r\n");
				file.write(reinterpret_cast<char*>(uninstaller.getPtr()), (std::streamsize)uninstaller.getSize());
				file.close();

				// Update uninstaller's resources
				std::string newDir = std::regex_replace(fullDirectory, std::regex("\\\\"), "\\\\");
				const std::string newManifest(
					std::string(reinterpret_cast<char*>(manifest.getPtr()), manifest.getSize())
					+ "\r\ndirectory \"" + newDir + "\""
				);
				auto handle = BeginUpdateResource(uninstallerPath.c_str(), 0);
				if (!(bool)UpdateResource(handle, "MANIFEST", MAKEINTRESOURCE(IDR_MANIFEST), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPVOID)newManifest.c_str(), (DWORD)newManifest.size())) {
					NST::Log::PushText("Cannot write manifest contents to the uninstaller, aborting...\r\n");
					invalidate();
				}
				EndUpdateResource(handle, FALSE);

				// Add uninstaller to system registry
				HKEY hkey;
				if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, (L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + m_mfStrings[L"name"]).c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &hkey, nullptr) == ERROR_SUCCESS) {
					auto
						name = NST::from_wideString(m_mfStrings[L"name"]);
					auto
						version = NST::from_wideString(m_mfStrings[L"version"]);
					auto
						publisher = NST::from_wideString(m_mfStrings[L"publisher"]);
					auto
						icon = NST::from_wideString(m_mfStrings[L"icon"]);
					DWORD ONE = 1;
					auto SIZE = (DWORD)(m_maxSize / 1024ull);
					if (icon.empty())
						icon = uninstallerPath;
					else
						icon = fullDirectory + icon;
					RegSetKeyValueA(hkey, nullptr, "UninstallString", REG_SZ, (LPCVOID)uninstallerPath.c_str(), (DWORD)uninstallerPath.size());
					RegSetKeyValueA(hkey, nullptr, "DisplayIcon", REG_SZ, (LPCVOID)icon.c_str(), (DWORD)icon.size());
					RegSetKeyValueA(hkey, nullptr, "DisplayName", REG_SZ, (LPCVOID)name.c_str(), (DWORD)name.size());
					RegSetKeyValueA(hkey, nullptr, "DisplayVersion", REG_SZ, (LPCVOID)version.c_str(), (DWORD)version.size());
					RegSetKeyValueA(hkey, nullptr, "InstallLocation", REG_SZ, (LPCVOID)fullDirectory.c_str(), (DWORD)fullDirectory.size());
					RegSetKeyValueA(hkey, nullptr, "Publisher", REG_SZ, (LPCVOID)publisher.c_str(), (DWORD)publisher.size());
					RegSetKeyValueA(hkey, nullptr, "NoModify", REG_DWORD, (LPCVOID)&ONE, (DWORD)sizeof(DWORD));
					RegSetKeyValueA(hkey, nullptr, "NoRepair", REG_DWORD, (LPCVOID)&ONE, (DWORD)sizeof(DWORD));
					RegSetKeyValueA(hkey, nullptr, "EstimatedSize", REG_DWORD, (LPCVOID)&SIZE, (DWORD)sizeof(DWORD));
				}
				RegCloseKey(hkey);
			}
		}
		});
}

void Installer::dumpErrorLog()
{
	// Dump error log to disk
	const auto dir = NST::Directory::GetRunningDirectory() + "\\error_log.txt";
	const auto t = std::time(nullptr);
	char dateData[127];
	ctime_s(dateData, 127, &t);
	std::string logData;

	// If the log doesn't exist, add header text
	if (!std::filesystem::exists(dir))
		logData += "Installer error log:\r\n";

	// Add remaining log data
	logData += std::string(dateData) + NST::Log::PullText() + "\r\n";

	// Try to create the file
	std::filesystem::create_directories(std::filesystem::path(dir).parent_path());
	std::ofstream file(dir, std::ios::binary | std::ios::out | std::ios::app);
	if (!file.is_open())
		NST::Log::PushText("Cannot dump error log to disk...\r\n");
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
	const auto ptr = reinterpret_cast<Installer*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	if (message == WM_PAINT)
		ptr->paint();
	else if (message == WM_DESTROY)
		PostQuitMessage(0);
	return DefWindowProc(hWnd, message, wParam, lParam);
}