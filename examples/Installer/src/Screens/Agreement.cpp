#include "Screens/Agreement.h"
#include "Installer.h"


static LRESULT CALLBACK WndProc(HWND /*hWnd*/, UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/);

Agreement_Screen::~Agreement_Screen()
{
	UnregisterClass("AGREEMENT_SCREEN", m_hinstance);
	DestroyWindow(m_hwnd);
	DestroyWindow(m_checkYes);
	DestroyWindow(m_btnPrev);
	DestroyWindow(m_btnNext);
	DestroyWindow(m_btnCancel);
}

Agreement_Screen::Agreement_Screen(Installer* installer, const HINSTANCE hInstance, const HWND parent, const vec2& pos, const vec2& size)
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
	m_wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	m_wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	m_wcex.lpszMenuName = nullptr;
	m_wcex.lpszClassName = "AGREEMENT_SCREEN";
	m_wcex.hIconSm = LoadIcon(m_wcex.hInstance, IDI_APPLICATION);
	RegisterClassEx(&m_wcex);
	m_hwnd = CreateWindow("AGREEMENT_SCREEN", "", WS_OVERLAPPED | WS_CHILD | WS_VISIBLE, pos.x, pos.y, size.x, size.y, parent, nullptr, hInstance, nullptr);
	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
	setVisible(false);

	// Create EULA
	m_log = CreateWindowExW(WS_EX_CLIENTEDGE, L"edit", m_installer->m_mfStrings[L"EULA"].c_str(), WS_VISIBLE | WS_OVERLAPPED | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 10, 75, size.x - 20, size.y - 125, m_hwnd, nullptr, hInstance, nullptr);
	if (m_installer->m_mfStrings[L"EULA"].empty())
		SetWindowTextW(m_log,
			L"nSuite installers can be created freely by anyone, as such those who generate them are responsible for its contents, not the developers.\r\n"
			L"This software is provided as - is, use it at your own risk."
		);

	// Create check-boxes
	m_checkYes = CreateWindow("Button", "I accept the terms of this license agreement", WS_OVERLAPPED | WS_VISIBLE | WS_CHILD | BS_CHECKBOX | BS_AUTOCHECKBOX, 10, size.y - 35, 310, 15, m_hwnd, (HMENU)1, hInstance, nullptr);
	CheckDlgButton(m_hwnd, 1, BST_UNCHECKED);

	// Create Buttons
	constexpr auto BUTTON_STYLES = WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON;
	m_btnPrev = CreateWindow("BUTTON", "< Back", BUTTON_STYLES | BS_DEFPUSHBUTTON, size.x - 290, size.y - 40, 85, 30, m_hwnd, nullptr, hInstance, nullptr);
	m_btnNext = CreateWindow("BUTTON", "Next >", BUTTON_STYLES | BS_DEFPUSHBUTTON, size.x - 200, size.y - 40, 85, 30, m_hwnd, nullptr, hInstance, nullptr);
	m_btnCancel = CreateWindow("BUTTON", "Cancel", BUTTON_STYLES, size.x - 95, size.y - 40, 85, 30, m_hwnd, nullptr, hInstance, nullptr);
}

void Agreement_Screen::enact()
{
	EnableWindow(m_btnNext, static_cast<BOOL>(m_agrees));
}

void Agreement_Screen::paint()
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

void Agreement_Screen::checkYes()
{
	m_agrees = (IsDlgButtonChecked(m_hwnd, 1) != 0U);
	CheckDlgButton(m_hwnd, 1, m_agrees ? BST_CHECKED : BST_UNCHECKED);
	EnableWindow(m_btnNext, static_cast<BOOL>(m_agrees));
}

void Agreement_Screen::goPrevious()
{
	m_installer->setScreen(Installer::ScreenEnums::WELCOME_SCREEN);
}

void Agreement_Screen::goNext()
{
	m_installer->setScreen(Installer::ScreenEnums::DIRECTORY_SCREEN);
}

void Agreement_Screen::goCancel()
{
	PostQuitMessage(0);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	const auto ptr = reinterpret_cast<Agreement_Screen*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	const auto controlHandle = HWND(lParam);
	if (message == WM_PAINT)
		ptr->paint();
	else if (message == WM_CTLCOLORSTATIC) {
		// Make check-box text background color transparent
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