#include "FinishState.h"
#include "Common.h"
#include "../Installer.h"
#include <algorithm>
#include <filesystem>
#include <shlobj.h>
#include <shlwapi.h>


static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

FinishState::~FinishState()
{
	UnregisterClass("FINISH_STATE", m_hinstance);
	DestroyWindow(m_hwnd);
	DestroyWindow(m_checkbox);
	for each (auto checkboxHandle in m_shortcutCheckboxes)
		DestroyWindow(checkboxHandle);
}

FinishState::FinishState(Installer * installer, const HINSTANCE hInstance, const HWND parent, const RECT & rc)
	: FrameState(installer) 
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
	m_wcex.lpszClassName = "FINISH_STATE";
	m_wcex.hIconSm = LoadIcon(m_wcex.hInstance, IDI_APPLICATION);
	RegisterClassEx(&m_wcex);
	m_hwnd = CreateWindow("FINISH_STATE", "", WS_OVERLAPPED | WS_VISIBLE | WS_CHILD, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, parent, NULL, hInstance, NULL);
	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
	setVisible(false);

	// Create checkboxes
	m_checkbox = CreateWindow("Button", "Show installation directory on close", WS_OVERLAPPED | WS_VISIBLE | WS_CHILD | BS_CHECKBOX | BS_AUTOCHECKBOX, 10, 150, 610, 15, m_hwnd, (HMENU)1, hInstance, NULL);
	CheckDlgButton(m_hwnd, 1, BST_CHECKED);

	// Shortcuts
	const auto desktopStrings = m_installer->m_mfStrings[L"shortcut"], startmenuStrings = m_installer->m_mfStrings[L"startmenu"];
	size_t numD = std::count(desktopStrings.begin(), desktopStrings.end(), L',') + 1ull, numS = std::count(startmenuStrings.begin(), startmenuStrings.end(), L',') + 1ull;
	m_shortcutCheckboxes.reserve(numD + numS);
	m_shortcuts_d.reserve(numD + numS);
	m_shortcuts_s.reserve(numD + numS);
	size_t last = 0;
	if (!desktopStrings.empty())
		for (size_t x = 0; x < numD; ++x) {
			// Find end of shortcut
			auto nextComma = desktopStrings.find(L',', last);
			if (nextComma == std::wstring::npos)
				nextComma = desktopStrings.size();

			// Find demarkation point where left half is the shortcut path, right half is the shortcut name
			m_shortcuts_d.push_back(desktopStrings.substr(last, nextComma - last));

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
			m_shortcuts_s.push_back(startmenuStrings.substr(last, nextComma - last));

			// Skip whitespace, find next element
			last = nextComma + 1ull;
			while (last < startmenuStrings.size() && (startmenuStrings[last] == L' ' || startmenuStrings[last] == L'\r' || startmenuStrings[last] == L'\t' || startmenuStrings[last] == L'\n'))
				last++;
		}
	int vertical = 170, checkIndex = 2;
	for each (const auto & shortcut in m_shortcuts_d) {
		const auto name = std::wstring(&shortcut[1], shortcut.length() - 1);
		m_shortcutCheckboxes.push_back(CreateWindowW(L"Button", (L"Create a shortcut for " + name + L" on the desktop").c_str(), WS_OVERLAPPED | WS_VISIBLE | WS_CHILD | BS_CHECKBOX | BS_AUTOCHECKBOX, 10, vertical, 610, 15, m_hwnd, (HMENU)(LONGLONG)checkIndex, hInstance, NULL));
		CheckDlgButton(m_hwnd, checkIndex, BST_CHECKED);
		vertical += 20;
		checkIndex++;
	}
	for each (const auto & shortcut in m_shortcuts_s) {
		const auto name = std::wstring(&shortcut[1], shortcut.length() - 1);
		m_shortcutCheckboxes.push_back(CreateWindowW(L"Button", (L"Create a shortcut for " + name + L" in the start-menu").c_str(), WS_OVERLAPPED | WS_VISIBLE | WS_CHILD | BS_CHECKBOX | BS_AUTOCHECKBOX, 10, vertical, 610, 15, m_hwnd, (HMENU)(LONGLONG)checkIndex, hInstance, NULL));
		CheckDlgButton(m_hwnd, checkIndex, BST_CHECKED);
		vertical += 20;
		checkIndex++;
	}
}

void FinishState::enact()
{
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
	// Open the installation directory + if user wants it to
	if (IsDlgButtonChecked(m_hwnd, 1))
		ShellExecute(NULL, "open", m_installer->getDirectory().c_str(), NULL, NULL, SW_SHOWDEFAULT);

	// Create Shortcuts
	int x = 2;
	for each (const auto & shortcut in m_shortcuts_d) {
		if (IsDlgButtonChecked(m_hwnd, x)) {
			std::error_code ec;
			const auto nonwideShortcut = from_wideString(shortcut);
			auto instDir = m_installer->getDirectory();
			auto srcPath = instDir;
			if (srcPath.back() == '\\')
				srcPath = std::string(&srcPath[0], srcPath.size() - 1ull);
			srcPath += nonwideShortcut;
			const auto dstPath = get_users_desktop() + "\\" + std::filesystem::path(srcPath).filename().string();
			create_shortcut(srcPath, instDir, dstPath);
		}
		x++;
	}
	for each (const auto & shortcut in m_shortcuts_s) {
		if (IsDlgButtonChecked(m_hwnd, x)) {
			std::error_code ec;
			const auto nonwideShortcut = from_wideString(shortcut);
			auto instDir = m_installer->getDirectory();
			auto srcPath = instDir;
			if (srcPath.back() == '\\')
				srcPath = std::string(&srcPath[0], srcPath.size() - 1ull);
			srcPath += nonwideShortcut;
			const auto dstPath = get_users_startmenu() + "\\" + std::filesystem::path(srcPath).filename().string();
			create_shortcut(srcPath, instDir, dstPath);
		}
		x++;
	}
	
	PostQuitMessage(0);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	auto ptr = (FinishState*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (message == WM_PAINT) {
		PAINTSTRUCT ps;
		Graphics graphics(BeginPaint(hWnd, &ps));

		// Draw Background
		LinearGradientBrush backgroundGradient(
			Point(0, 0),
			Point(0, 450),
			Color(50, 25, 255, 125),
			Color(255, 255, 255, 255)
		);
		graphics.FillRectangle(&backgroundGradient, 0, 0, 630, 450);

		// Preparing Fonts
		FontFamily  fontFamily(L"Segoe UI");
		Font        bigFont(&fontFamily, 25, FontStyleBold, UnitPixel);
		SolidBrush  blueBrush(Color(255, 25, 125, 225));

		// Draw Text
		graphics.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);
		graphics.DrawString(L"Installation Complete", -1, &bigFont, PointF{ 10, 10 }, &blueBrush);		

		EndPaint(hWnd, &ps);
		return S_OK;
	}
	else if (message == WM_CTLCOLORSTATIC) {
		// Make checkbox text background color transparent
		const auto handle = HWND(lParam);
		bool isCheckbox = handle == ptr->m_checkbox;
		for each (auto chkHandle in ptr->m_shortcutCheckboxes)
			if (handle == chkHandle) {
				isCheckbox = true;
				break;
			}
		if (isCheckbox) {
			SetBkMode(HDC(wParam), TRANSPARENT);
			return (LRESULT)GetStockObject(NULL_BRUSH);
		}
		// Make log color white
		SetBkColor(HDC(wParam), RGB(255, 255, 255));
		return (LRESULT)GetStockObject(WHITE_BRUSH);
	}
	else if (message == WM_COMMAND) {
		const auto notification = HIWORD(wParam);
		if (notification == BN_CLICKED) {
			ptr->m_showDirectory = IsDlgButtonChecked(hWnd, 1);
			return S_OK;
		}
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}