#include "Screens/Fail.hpp"
#include "Log.hpp"
#include "Uninstaller.hpp"
#include <ctime>
#include <fstream>


static LRESULT CALLBACK WndProc(HWND /*hWnd*/, UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/);

Fail_Screen::~Fail_Screen()
{
	UnregisterClass("FAIL_SCREEN", m_hinstance);
	DestroyWindow(m_hwnd);
	DestroyWindow(m_hwndLog);
	DestroyWindow(m_btnClose);
	yatta::Log::RemoveObserver(m_logIndex);
}

Fail_Screen::Fail_Screen(Uninstaller* unInstaller, const HINSTANCE hInstance, const HWND parent, const vec2& pos, const vec2& size)
	: Screen(unInstaller, pos, size)
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
	m_wcex.lpszClassName = "FAIL_SCREEN";
	m_wcex.hIconSm = LoadIcon(m_wcex.hInstance, IDI_APPLICATION);
	RegisterClassEx(&m_wcex);
	m_hwnd = CreateWindow("FAIL_SCREEN", "", WS_OVERLAPPED | WS_CHILD | WS_VISIBLE, pos.x, pos.y, size.x, size.y, parent, nullptr, hInstance, nullptr);
	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
	setVisible(false);

	// Create error log
	m_hwndLog = CreateWindowEx(WS_EX_CLIENTEDGE, "edit", nullptr, WS_VISIBLE | WS_OVERLAPPED | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 10, 75, size.x - 20, size.y - 125, m_hwnd, nullptr, hInstance, nullptr);
	SendMessage(m_hwndLog, EM_REPLACESEL, FALSE, (LPARAM)"Error Log:\r\n");
	m_logIndex = yatta::Log::AddObserver([&](const std::string& message) {
		SendMessage(m_hwndLog, EM_REPLACESEL, FALSE, (LPARAM)message.c_str());
		});

	// Create Buttons
	constexpr auto BUTTON_STYLES = WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON;
	m_btnClose = CreateWindow("BUTTON", "Close", BUTTON_STYLES, size.x - 95, size.y - 40, 85, 30, m_hwnd, nullptr, hInstance, nullptr);
}

void Fail_Screen::enact()
{
	Uninstaller::dumpErrorLog();
}

void Fail_Screen::paint()
{
	PAINTSTRUCT ps;
	Graphics graphics(BeginPaint(m_hwnd, &ps));

	// Draw Background
	LinearGradientBrush backgroundGradient(
		Point(0, 0),
		Point(0, m_size.y),
		Color(50, 225, 25, 75),
		Color(255, 255, 255, 255)
	);
	graphics.FillRectangle(&backgroundGradient, 0, 0, m_size.x, m_size.y);

	// Preparing Fonts
	FontFamily  fontFamily(L"Segoe UI");
	Font        bigFont(&fontFamily, 25, FontStyleBold, UnitPixel);
	Font        regFont(&fontFamily, 14, FontStyleRegular, UnitPixel);
	SolidBrush  blueBrush(Color(255, 25, 125, 225));

	// Draw Text
	graphics.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);
	graphics.DrawString(L"Un-Installation Incomplete", -1, &bigFont, PointF{ 10, 10 }, &blueBrush);

	EndPaint(m_hwnd, &ps);
}

void Fail_Screen::goClose()
{
	PostQuitMessage(0);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	const auto ptr = reinterpret_cast<Fail_Screen*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	const auto controlHandle = HWND(lParam);
	if (message == WM_PAINT)
		ptr->paint();
	else if (message == WM_CTLCOLORSTATIC) {
		// Make log color white
		SetBkColor(HDC(wParam), RGB(255, 255, 255));
		return (LRESULT)GetStockObject(WHITE_BRUSH);
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