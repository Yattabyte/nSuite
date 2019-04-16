#include "FinishState.h"
#include "Common.h"
#include "../Uninstaller.h"
#include <algorithm>
#include <filesystem>
#include <shlobj.h>
#include <shlwapi.h>


static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

FinishState::~FinishState()
{
	UnregisterClass("FINISH_STATE", m_hinstance);
	DestroyWindow(m_hwnd);
}

FinishState::FinishState(Uninstaller * uninstaller, const HINSTANCE hInstance, const HWND parent, const RECT & rc)
	: FrameState(uninstaller) 
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
}

void FinishState::enact()
{
	m_uninstaller->showButtons(false, false, true);
	m_uninstaller->enableButtons(false, false, true);
	m_uninstaller->finish();
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
		graphics.DrawString(L"Uninstallation Complete", -1, &bigFont, PointF{ 10, 10 }, &blueBrush);

		EndPaint(hWnd, &ps);
		return S_OK;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}