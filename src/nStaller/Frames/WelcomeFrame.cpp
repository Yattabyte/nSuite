#include "WelcomeFrame.h"
#include "Resource.h"
#include "../Installer.h"


static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

WelcomeFrame::~WelcomeFrame()
{
	UnregisterClass("WELCOME_FRAME", m_hinstance);
	DestroyWindow(m_hwnd);
}

WelcomeFrame::WelcomeFrame(Installer * installer, const HINSTANCE hInstance, const HWND parent, const RECT & rc)
{
	// Create window class
	m_installer = installer;
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
	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
	setVisible(false);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	auto ptr = (WelcomeFrame*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
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
		Font        regFont(&fontFamily, 14, FontStyleRegular, UnitPixel);
		SolidBrush  blueBrush(Color(255, 25, 125, 225));
		SolidBrush	blackBrush(Color(255, 0, 0, 0));
		StringFormat format = StringFormat::GenericTypographic();

		// Draw Text
		graphics.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);
		graphics.DrawString(L"Welcome to the Installation Wizard", -1, &bigFont, PointF{ 10, 10 }, &blueBrush);
		auto nameVer = ptr->m_installer->m_name + L" " + ptr->m_installer->m_version;
		if (ptr->m_installer->m_name.empty()) nameVer = L"it's contents";
		graphics.DrawString((L"The Wizard will install " + nameVer + L" on to your computer.").c_str(), -1, &regFont, PointF{ 10, 100 }, &format, &blackBrush);
		graphics.DrawString(ptr->m_installer->m_description.c_str(), -1, &regFont, RectF(10, 150, 630, 300), &format, &blackBrush);
		
		EndPaint(hWnd, &ps);
		return S_OK;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}