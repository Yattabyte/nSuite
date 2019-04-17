#include "Agreement.h"
#include "../Installer.h"


static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

Agreement::~Agreement()
{
	UnregisterClass("AGREEMENT_STATE", m_hinstance);
	DestroyWindow(m_hwnd);
	DestroyWindow(m_checkYes);
}

Agreement::Agreement(Installer * installer, const HINSTANCE hInstance, const HWND parent, const vec2 & pos, const vec2 & size)
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
	m_wcex.lpszClassName = "AGREEMENT_STATE";
	m_wcex.hIconSm = LoadIcon(m_wcex.hInstance, IDI_APPLICATION);
	RegisterClassEx(&m_wcex);
	m_hwnd = CreateWindow("AGREEMENT_STATE", "", WS_OVERLAPPED | WS_CHILD | WS_VISIBLE, pos.x, pos.y, size.x, size.y, parent, NULL, hInstance, NULL);
	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
	setVisible(false);

	// Create eula
	m_log = CreateWindowExW(WS_EX_CLIENTEDGE, L"edit", m_installer->m_mfStrings[L"eula"].c_str(), WS_VISIBLE | WS_OVERLAPPED | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 10, 75, size.x - 20, size.y - 125, m_hwnd, NULL, hInstance, NULL);
	if (m_installer->m_mfStrings[L"eula"].empty())
		SetWindowTextW(m_log, 
			L"nSuite installers can be created freely by anyone, as such those who generate them are responsible for its contents, not the developers.\r\n"
			L"This software is provided as - is, use it at your own risk."
		);

	// Create checkboxes
	m_checkYes = CreateWindow("Button", "I accept the terms of this license agreement", WS_OVERLAPPED | WS_VISIBLE | WS_CHILD | BS_CHECKBOX | BS_AUTOCHECKBOX, 10, size.y - 35, 310, 15, m_hwnd, (HMENU)1, hInstance, NULL);
	CheckDlgButton(m_hwnd, 1, BST_UNCHECKED);

	// Create Buttons
	constexpr auto BUTTON_STYLES = WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON;
	m_btnPrev = CreateWindow("BUTTON", "< Back", BUTTON_STYLES | BS_DEFPUSHBUTTON, size.x - 290, size.y - 40, 85, 30, m_hwnd, NULL, hInstance, NULL);
	m_btnNext = CreateWindow("BUTTON", "Next >", BUTTON_STYLES | BS_DEFPUSHBUTTON, size.x - 200, size.y - 40, 85, 30, m_hwnd, NULL, hInstance, NULL);
	m_btnCancel = CreateWindow("BUTTON", "Cancel", BUTTON_STYLES, size.x - 95, size.y - 40, 85, 30, m_hwnd, NULL, hInstance, NULL);
}

void Agreement::enact()
{
	EnableWindow(m_btnNext, m_agrees);
}

void Agreement::paint()
{
	PAINTSTRUCT ps;
	Graphics graphics(BeginPaint(m_hwnd, &ps));

	// Draw Background
	LinearGradientBrush backgroundGradient(
		Point(0, 0),
		Point(0, m_size.y),
		Color(50, 25, 125, 225),
		Color(255, 255, 255, 255)
	);
	graphics.FillRectangle(&backgroundGradient, 0, 0, m_size.x, m_size.y);

	// Preparing Fonts
	FontFamily  fontFamily(L"Segoe UI");
	Font        bigFont(&fontFamily, 25, FontStyleBold, UnitPixel);
	Font        regFont(&fontFamily, 14, FontStyleRegular, UnitPixel);
	SolidBrush  blueBrush(Color(255, 25, 125, 225));
	SolidBrush  blackBrush(Color(255, 0, 0, 0));

	// Draw Text
	graphics.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);
	graphics.DrawString(L"License Agreement", -1, &bigFont, PointF{ 10, 10 }, &blueBrush);
	graphics.DrawString(L"Please read the following license agreement:", -1, &regFont, PointF{ 10, 50 }, &blackBrush);

	EndPaint(m_hwnd, &ps);
}

void Agreement::checkYes()
{
	m_agrees = IsDlgButtonChecked(m_hwnd, 1);
	CheckDlgButton(m_hwnd, 1, m_agrees ? BST_CHECKED : BST_UNCHECKED);
	EnableWindow(m_btnNext, m_agrees);
}

void Agreement::goPrevious()
{
	m_installer->setState(Installer::StateEnums::WELCOME_STATE);
}

void Agreement::goNext()
{
	m_installer->setState(Installer::StateEnums::DIRECTORY_STATE);
}

void Agreement::goCancel()
{
	PostQuitMessage(0);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	const auto ptr = (Agreement*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	const auto controlHandle = HWND(lParam);
	if (message == WM_PAINT)
		ptr->paint();	
	else if (message == WM_CTLCOLORSTATIC) {
		// Make checkbox text background color transparent
		if (controlHandle == ptr->m_checkYes) {
			SetBkMode(HDC(wParam), TRANSPARENT);
			return (LRESULT)GetStockObject(NULL_BRUSH);
		}
		SetBkColor(HDC(wParam), RGB(255, 255, 255));
		return (LRESULT)GetStockObject(WHITE_BRUSH);
	}
	else if (message == WM_COMMAND) {
		const auto notification = HIWORD(wParam);
		if (notification == BN_CLICKED) {
			if (controlHandle == ptr->m_checkYes) 
				ptr->checkYes();
			else if (controlHandle == ptr->m_btnPrev)
				ptr->goPrevious();
			else if (controlHandle == ptr->m_btnNext)
				ptr->goNext();
			else if (controlHandle == ptr->m_btnCancel)
				ptr->goCancel();
		}
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}