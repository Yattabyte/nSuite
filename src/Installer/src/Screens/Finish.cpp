#include "Screens/Finish.h"
#include "StringConversions.h"
#include "DirectoryTools.h"
#include "Installer.h"


static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

Finish::~Finish()
{
	UnregisterClass("FINISH_SCREEN", m_hinstance);
	DestroyWindow(m_hwnd);
	DestroyWindow(m_checkbox);
	DestroyWindow(m_btnClose);
	for each (auto checkboxHandle in m_shortcutCheckboxes)
		DestroyWindow(checkboxHandle);
}

Finish::Finish(Installer * installer, const HINSTANCE hInstance, const HWND parent, const vec2 & pos, const vec2 & size)
	: Screen(installer, pos, size)
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
	m_wcex.lpszClassName = "FINISH_SCREEN";
	m_wcex.hIconSm = LoadIcon(m_wcex.hInstance, IDI_APPLICATION);
	RegisterClassEx(&m_wcex);
	m_hwnd = CreateWindow("FINISH_SCREEN", "", WS_OVERLAPPED | WS_CHILD | WS_VISIBLE, pos.x, pos.y, size.x, size.y, parent, NULL, hInstance, NULL);
	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
	setVisible(false);

	// Create checkboxes
	m_checkbox = CreateWindow("Button", "Show installation directory on close", WS_OVERLAPPED | WS_VISIBLE | WS_CHILD | BS_CHECKBOX | BS_AUTOCHECKBOX, 10, 150, size.x, 15, m_hwnd, (HMENU)1, hInstance, NULL);
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
		m_shortcutCheckboxes.push_back(CreateWindowW(L"Button", (L"Create a shortcut for " + name + L" on the desktop").c_str(), WS_OVERLAPPED | WS_VISIBLE | WS_CHILD | BS_CHECKBOX | BS_AUTOCHECKBOX, 10, vertical, size.x, 15, m_hwnd, (HMENU)(LONGLONG)checkIndex, hInstance, NULL));
		CheckDlgButton(m_hwnd, checkIndex, BST_CHECKED);
		vertical += 20;
		checkIndex++;
	}
	for each (const auto & shortcut in m_shortcuts_s) {
		const auto name = std::wstring(&shortcut[1], shortcut.length() - 1);
		m_shortcutCheckboxes.push_back(CreateWindowW(L"Button", (L"Create a shortcut for " + name + L" in the start-menu").c_str(), WS_OVERLAPPED | WS_VISIBLE | WS_CHILD | BS_CHECKBOX | BS_AUTOCHECKBOX, 10, vertical, size.x, 15, m_hwnd, (HMENU)(LONGLONG)checkIndex, hInstance, NULL));
		CheckDlgButton(m_hwnd, checkIndex, BST_CHECKED);
		vertical += 20;
		checkIndex++;
	}
	
	// Create Buttons
	constexpr auto BUTTON_STYLES = WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON;
	m_btnClose = CreateWindow("BUTTON", "Close", BUTTON_STYLES, size.x - 95, size.y - 40, 85, 30, m_hwnd, NULL, hInstance, NULL);
}

void Finish::enact()
{
	// Does nothing
}

void Finish::paint()
{
	PAINTSTRUCT ps;
	Graphics graphics(BeginPaint(m_hwnd, &ps));

	// Draw Background
	LinearGradientBrush backgroundGradient(
		Point(0, 0),
		Point(0, m_size.y),
		Color(50, 25, 255, 125),
		Color(255, 255, 255, 255)
	);
	graphics.FillRectangle(&backgroundGradient, 0, 0, m_size.x, m_size.y);

	// Preparing Fonts
	FontFamily  fontFamily(L"Segoe UI");
	Font        bigFont(&fontFamily, 25, FontStyleBold, UnitPixel);
	SolidBrush  blueBrush(Color(255, 25, 125, 225));

	// Draw Text
	graphics.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);
	graphics.DrawString(L"Installation Complete", -1, &bigFont, PointF{ 10, 10 }, &blueBrush);

	EndPaint(m_hwnd, &ps);
}

void Finish::goClose()
{
	m_showDirectory = IsDlgButtonChecked(m_hwnd, 1);
	const auto instDir = m_installer->getDirectory() + "\\" + m_installer->getPackageName();
	// Open the installation directory + if user wants it to
	if (m_showDirectory)
		ShellExecute(NULL, "open", instDir.c_str(), NULL, NULL, SW_SHOWDEFAULT);

	// Create Shortcuts
	int x = 2;
	for each (const auto & shortcut in m_shortcuts_d) {
		if (IsDlgButtonChecked(m_hwnd, x)) {
			std::error_code ec;
			const auto nonwideShortcut = from_wideString(shortcut);
			auto srcPath = instDir;
			if (srcPath.back() == '\\')
				srcPath = std::string(&srcPath[0], srcPath.size() - 1ull);
			srcPath += nonwideShortcut;
			const auto dstPath = DRT::GetDesktopPath() + "\\" + std::filesystem::path(srcPath).filename().string();
			createShortcut(srcPath, instDir, dstPath);
		}
		x++;
	}
	for each (const auto & shortcut in m_shortcuts_s) {
		if (IsDlgButtonChecked(m_hwnd, x)) {
			std::error_code ec;
			const auto nonwideShortcut = from_wideString(shortcut);
			auto srcPath = instDir;
			if (srcPath.back() == '\\')
				srcPath = std::string(&srcPath[0], srcPath.size() - 1ull);
			srcPath += nonwideShortcut;
			const auto dstPath = DRT::GetStartMenuPath() + "\\" + std::filesystem::path(srcPath).filename().string();
			createShortcut(srcPath, instDir, dstPath);
		}
		x++;
	}
	PostQuitMessage(0);	
}

void Finish::createShortcut(const std::string & srcPath, const std::string & wrkPath, const std::string & dstPath)
{
	IShellLink* psl;
	if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl))) {
		IPersistFile* ppf;

		// Set the path to the shortcut target and add the description. 
		psl->SetPath(srcPath.c_str());
		psl->SetWorkingDirectory(wrkPath.c_str());
		psl->SetIconLocation(srcPath.c_str(), 0);

		// Query IShellLink for the IPersistFile interface, used for saving the 
		// shortcut in persistent storage. 
		if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf))) {
			WCHAR wsz[MAX_PATH];

			// Ensure that the string is Unicode. 
			MultiByteToWideChar(CP_ACP, 0, (dstPath + ".lnk").c_str(), -1, wsz, MAX_PATH);

			// Save the link by calling IPersistFile::Save. 
			ppf->Save(wsz, TRUE);
			ppf->Release();
		}
		psl->Release();
	}
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	const auto ptr = (Finish*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	const auto controlHandle = HWND(lParam);
	if (message == WM_PAINT)
		ptr->paint();
	else if (message == WM_CTLCOLORSTATIC) {
		// Make checkbox text background color transparent
		bool isCheckbox = controlHandle == ptr->m_checkbox;
		for each (auto chkHandle in ptr->m_shortcutCheckboxes)
			if (controlHandle == chkHandle) {
				isCheckbox = true;
				break;
			}
		if (isCheckbox) {
			SetBkMode(HDC(wParam), TRANSPARENT);
			return (LRESULT)GetStockObject(NULL_BRUSH);
		}
	}
	else if (message == WM_COMMAND) {
		const auto notification = HIWORD(wParam);
		if (notification == BN_CLICKED) {
			if (controlHandle == ptr->m_btnClose) 
				ptr->goClose();			
		}
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}