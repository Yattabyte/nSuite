#include "WelcomeFrame.h"


static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

WelcomeFrame::~WelcomeFrame()
{
	UnregisterClass("WELCOME_FRAME", m_hinstance);
	DestroyWindow(m_hwnd);
}

WelcomeFrame::WelcomeFrame(const HINSTANCE hInstance, const HWND parent, const RECT & rc)
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
	m_wcex.lpszClassName = "WELCOME_FRAME";
	m_wcex.hIconSm = LoadIcon(m_wcex.hInstance, IDI_APPLICATION);
	RegisterClassEx(&m_wcex);
	m_hwnd = CreateWindow("WELCOME_FRAME", "", WS_OVERLAPPED | WS_VISIBLE | WS_CHILD, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, parent, NULL, hInstance, NULL);
	setVisible(false);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
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
		SolidBrush  blueBrush(Color(255, 25, 125, 225));

		// Draw Text
		graphics.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);
		graphics.DrawString(L"Welcome", -1, &bigFont, PointF{ 10, 10 }, &blueBrush);

		
		EndPaint(hWnd, &ps);
		return S_OK;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}