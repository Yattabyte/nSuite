#include "WelcomeState.h"
#include "../Installer.h"


static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

WelcomeState::~WelcomeState()
{
	UnregisterClass("WELCOME_STATE", m_hinstance);
	DestroyWindow(m_hwnd);
}

WelcomeState::WelcomeState(Installer * installer, const HINSTANCE hInstance, const HWND parent, const RECT & rc)
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
	m_wcex.hbrBackground = (HBRUSH)(COLOR_WINDOWFRAME);
	m_wcex.lpszMenuName = NULL;
	m_wcex.lpszClassName = "WELCOME_STATE";
	m_wcex.hIconSm = LoadIcon(m_wcex.hInstance, IDI_APPLICATION);
	RegisterClassEx(&m_wcex);
	m_hwnd = CreateWindow("WELCOME_STATE", "", WS_OVERLAPPED | WS_VISIBLE | WS_CHILD, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, parent, NULL, hInstance, NULL);
	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
	setVisible(false);
}

void WelcomeState::enact()
{
	m_installer->showButtons(false, true, true);
	m_installer->enableButtons(false, true, true);
}

void WelcomeState::pressPrevious()
{
	// Should never happen
}

void WelcomeState::pressNext()
{
	m_installer->setState(Installer::StateEnums::AGREEMENT_STATE);
}

void WelcomeState::pressClose()
{
	// No new screen
	PostQuitMessage(0);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	auto ptr = (WelcomeState*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
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
		auto nameVer = ptr->m_installer->m_mfStrings[L"name"] + L" " + ptr->m_installer->m_mfStrings[L"version"];
		if (ptr->m_installer->m_mfStrings[L"name"].empty()) nameVer = L"it's contents";
		graphics.DrawString((L"The Wizard will install " + nameVer + L" on to your computer.").c_str(), -1, &regFont, PointF{ 10, 100 }, &format, &blackBrush);
		graphics.DrawString(ptr->m_installer->m_mfStrings[L"description"].c_str(), -1, &regFont, RectF(10, 150, 620, 300), &format, &blackBrush);

		EndPaint(hWnd, &ps);
		return S_OK;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}