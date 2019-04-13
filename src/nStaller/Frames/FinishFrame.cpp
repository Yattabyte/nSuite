#include "FinishFrame.h"
#include "../Installer.h"
#include <shlobj.h>
#include <shlwapi.h>


static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

FinishFrame::~FinishFrame()
{
	UnregisterClass("FINISH_FRAME", m_hinstance);
	DestroyWindow(m_hwnd);
	DestroyWindow(m_checkbox);
}

FinishFrame::FinishFrame(Installer * installer, const HINSTANCE hInstance, const HWND parent, const RECT & rc)
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
	m_wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	m_wcex.lpszMenuName = NULL;
	m_wcex.lpszClassName = "FINISH_FRAME";
	m_wcex.hIconSm = LoadIcon(m_wcex.hInstance, IDI_APPLICATION);
	RegisterClassEx(&m_wcex);
	m_hwnd = CreateWindow("FINISH_FRAME", "", WS_OVERLAPPED | WS_VISIBLE | WS_CHILD, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, parent, NULL, hInstance, NULL);
	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

	// Create checkbox
	m_checkbox = CreateWindow("Button", "", WS_OVERLAPPED | WS_VISIBLE | WS_CHILD | WS_BORDER | BS_CHECKBOX | BS_AUTOCHECKBOX, 10, 150, 15, 15, m_hwnd, (HMENU)1, hInstance, NULL);
	CheckDlgButton(m_hwnd, 1, BST_CHECKED);
	setVisible(false);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	auto ptr = (FinishFrame*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
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
		Font        regFont(&fontFamily, 14, FontStyleRegular, UnitPixel);
		SolidBrush  blueBrush(Color(255, 25, 125, 225));
		SolidBrush  blackBrush(Color(255, 0, 0, 0));

		// Draw Text
		graphics.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);
		graphics.DrawString(L"Installation Complete", -1, &bigFont, PointF{ 10, 10 }, &blueBrush);
		graphics.DrawString(L"Show installation directory on close.", -1, &regFont, PointF{ 30, 147 }, &blackBrush);
				
		EndPaint(hWnd, &ps);
		return S_OK;
	}
	else if (message == WM_COMMAND) {
		const auto notification = HIWORD(wParam);
		if (notification == BN_CLICKED) {
			BOOL checked = IsDlgButtonChecked(hWnd, 1);
			ptr->m_installer->showDirectoryOnClose(checked);
			return S_OK;
		}
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}