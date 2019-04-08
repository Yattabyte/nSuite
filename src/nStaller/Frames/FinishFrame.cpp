#include "FinishFrame.h"
#include <shlobj.h>
#include <shlwapi.h>


static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

FinishFrame::~FinishFrame()
{
	UnregisterClass("FINISH_FRAME", m_hinstance);
	DestroyWindow(m_hwnd);
	DestroyWindow(m_checkbox);
}

FinishFrame::FinishFrame(bool * openDirOnClose, const HINSTANCE hInstance, const HWND parent, const RECT & rc)
{
	// Create window class
	m_openDirOnClose = openDirOnClose;
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
	m_hwnd = CreateWindow("FINISH_FRAME", "", WS_OVERLAPPED | WS_VISIBLE | WS_CHILD | WS_DLGFRAME, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, parent, NULL, hInstance, NULL);
	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
	m_width = rc.right - rc.left;
	m_height = rc.bottom - rc.top;

	// Create checkbox
	m_checkbox = CreateWindow("Button", "Show installation directory on close.", WS_OVERLAPPED | WS_VISIBLE | WS_CHILD | BS_CHECKBOX | BS_AUTOCHECKBOX, 10, 150, rc.right - rc.left -20, 25, m_hwnd, (HMENU)1, hInstance, NULL);
	SetWindowLongPtr(m_checkbox, GWLP_USERDATA, (LONG_PTR)m_openDirOnClose);
	CheckDlgButton(m_hwnd, 1, *openDirOnClose ? BST_CHECKED : BST_UNCHECKED);
	setVisible(false);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_PAINT) {
		PAINTSTRUCT ps;
		Graphics graphics(BeginPaint(hWnd, &ps));
		graphics.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);
		auto frame = (Frame*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

		// Draw Background
		LinearGradientBrush backgroundGradient(
			Point(0, 0),
			Point(frame->m_width, frame->m_height),
			Color(255, 255, 255, 255),
			Color(50, 25, 255, 125)
		);
		graphics.FillRectangle(&backgroundGradient, 0, 0, frame->m_width, frame->m_height);

		// Preparing Fonts
		FontFamily  fontFamily(L"Segoe UI");
		Font        bigFont(&fontFamily, 25, FontStyleBold, UnitPixel);
		Font        regFont(&fontFamily, 14, FontStyleRegular, UnitPixel);
		SolidBrush  blueBrush(Color(255, 25, 125, 225));
		SolidBrush  blackBrush(Color(255, 0, 0, 0));

		// Draw Text
		graphics.DrawString(L"Installation Complete", -1, &bigFont, PointF{ 10, 10 }, &blueBrush);
		graphics.DrawString(L"Press the 'Cancel' button to close . . .", -1, &regFont, PointF{ 10, 420 }, &blackBrush);
		
		EndPaint(hWnd, &ps);
		return S_OK;
	}
	else if (message == WM_COMMAND) {
		const auto notification = HIWORD(wParam);
		auto controlHandle = HWND(lParam);
		if (notification == BN_CLICKED) {
			auto oOCPtr = (bool*)GetWindowLongPtr(controlHandle, GWLP_USERDATA);
			if (oOCPtr) {
				BOOL checked = IsDlgButtonChecked(hWnd, 1);
				*oOCPtr = checked;
				return S_OK;
			}
		}
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}