#include "Screens/Welcome.h"
#include "Uninstaller.h"


static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

Welcome_Screen::~Welcome_Screen()
{
	UnregisterClass("WELCOME_SCREEN", m_hinstance);
	DestroyWindow(m_hwnd);
	DestroyWindow(m_btnNext);
	DestroyWindow(m_btnCancel);
}

Welcome_Screen::Welcome_Screen(Uninstaller * uninstaller, const HINSTANCE hInstance, const HWND parent, const vec2 & pos, const vec2 & size)
	: Screen(uninstaller, pos, size)
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
	m_wcex.hbrBackground = (HBRUSH)(COLOR_WINDOWFRAME);
	m_wcex.lpszMenuName = NULL;
	m_wcex.lpszClassName = "WELCOME_SCREEN";
	m_wcex.hIconSm = LoadIcon(m_wcex.hInstance, IDI_APPLICATION);
	RegisterClassEx(&m_wcex);
	m_hwnd = CreateWindow("WELCOME_SCREEN", "", WS_OVERLAPPED | WS_CHILD | WS_VISIBLE, pos.x, pos.y, size.x, size.y, parent, NULL, hInstance, NULL);
	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
	setVisible(false);
	
	// Create Buttons
	constexpr auto BUTTON_STYLES = WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON;
	m_btnNext = CreateWindow("BUTTON", "Uninstall >", BUTTON_STYLES | BS_DEFPUSHBUTTON, size.x - 200, size.y - 40, 85, 30, m_hwnd, NULL, hInstance, NULL);
	m_btnCancel = CreateWindow("BUTTON", "Cancel", BUTTON_STYLES, size.x - 95, size.y - 40, 85, 30, m_hwnd, NULL, hInstance, NULL);
}

void Welcome_Screen::enact()
{
	// Does nothing
}

void Welcome_Screen::paint()
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
	Font        regUnderFont(&fontFamily, 14, FontStyleUnderline, UnitPixel);
	SolidBrush  blueBrush(Color(255, 25, 125, 225));
	SolidBrush	blackBrush(Color(255, 0, 0, 0));
	SolidBrush  greyBrush(Color(255, 127, 127, 127));
	SolidBrush  blueishBrush(Color(255, 100, 125, 175));
	StringFormat format = StringFormat::GenericTypographic();

	// Draw Text
	graphics.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);
	graphics.DrawString(L"Welcome to the Un-installation Wizard", -1, &bigFont, PointF{ 10, 10 }, &blueBrush);
	auto nameVer = m_uninstaller->m_mfStrings[L"name"] + L" " + m_uninstaller->m_mfStrings[L"version"];
	if (m_uninstaller->m_mfStrings[L"name"].empty()) nameVer = L"it's contents";
	graphics.DrawString((L"The Wizard will remove " + nameVer + L" from your computer.").c_str(), -1, &regFont, PointF{ 10, 75 }, &format, &blackBrush);
	graphics.DrawString(L"Note: the installation directory for this software will be deleted.\r\nIf there are any files that you wish to preserve, move them before continuing.", -1, &regFont, RectF(10, 400, 620, 300), &format, &blackBrush);

	// Draw -watermark-
	graphics.DrawString(L"This software was generated using nSuite", -1, &regFont, PointF(10, REAL(m_size.y - 45)), &greyBrush);
	graphics.DrawString(L"https://github.com/Yattabyte/nSuite", -1, &regUnderFont, PointF(10, REAL(m_size.y - 25)), &blueishBrush);

	EndPaint(m_hwnd, &ps);
}

void Welcome_Screen::goNext()
{
	m_uninstaller->setScreen(Uninstaller::ScreenEnums::UNINSTALL_SCREEN);
}

void Welcome_Screen::goCancel()
{
	PostQuitMessage(0);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	const auto ptr = reinterpret_cast<Welcome_Screen*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	if (message == WM_PAINT)
		ptr->paint();
	else if (message == WM_COMMAND) {
		const auto notification = HIWORD(wParam);
		if (notification == BN_CLICKED) {
			auto controlHandle = HWND(lParam);
			if (controlHandle == ptr->m_btnNext)
				ptr->goNext();
			else if (controlHandle == ptr->m_btnCancel)
				ptr->goCancel();
		}
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}