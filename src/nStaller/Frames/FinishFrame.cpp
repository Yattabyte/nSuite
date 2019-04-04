#include "FinishFrame.h"
#include <tchar.h>
#include <shlobj.h>
#include <shlwapi.h>


constexpr static auto CLASS_NAME = "FINISH_FRAME";
constexpr static auto FOREGROUND_COLOR = RGB(230, 230, 230);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

FinishFrame::~FinishFrame()
{
	UnregisterClass(CLASS_NAME, m_hinstance);
	DeleteObject(m_hwnd);
}

FinishFrame::FinishFrame(bool * openDirOnClose, const HINSTANCE & hInstance, const HWND & parent, const int & x, const int & y, const int & w, const int & h)
{
	// Try to create window class
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
	m_wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	m_wcex.lpszMenuName = NULL;
	m_wcex.lpszClassName = CLASS_NAME;
	m_wcex.hIconSm = LoadIcon(m_wcex.hInstance, IDI_APPLICATION);
	RegisterClassEx(&m_wcex);

	m_hwnd = CreateWindow(CLASS_NAME, CLASS_NAME, WS_OVERLAPPED | WS_VISIBLE | WS_CHILD | WS_DLGFRAME, x, y, w, h, parent, NULL, hInstance, NULL);
	auto checkbox = CreateWindow("Button", "Open installation directory on close.", WS_OVERLAPPED | WS_VISIBLE | WS_CHILD | BS_CHECKBOX | BS_AUTOCHECKBOX, 10, 150, w-20, 25, m_hwnd, (HMENU)1, hInstance, NULL);
	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
	SetWindowLongPtr(checkbox, GWLP_USERDATA, (LONG_PTR)this);
	CheckDlgButton(m_hwnd, 1, *openDirOnClose ? BST_CHECKED : BST_UNCHECKED);
	setVisible(false);
}

static LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	auto ptr = (FinishFrame*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (message == WM_PAINT) {
		PAINTSTRUCT ps;
		auto hdc = BeginPaint(hWnd, &ps);
		auto big_font = CreateFont(35, 15, 0, 0, FW_ULTRABOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FF_ROMAN, "Segoe UI");
		auto reg_font = CreateFont(17, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FF_ROMAN, "Segoe UI");

		// Draw Text
		constexpr static char* text[] = {
			"Installation Complete",
			"Press the 'Close' button to finish . . ."
		};
		SelectObject(hdc, big_font);
		SetTextColor(hdc, RGB(25, 125, 225));
		TextOut(hdc, 10, 10, text[0], _tcslen(text[0]));

		SelectObject(hdc, reg_font);
		SetTextColor(hdc, RGB(0, 0, 0));
		TextOut(hdc, 10, 420, text[1], _tcslen(text[1]));

		// Cleanup
		DeleteObject(big_font);
		DeleteObject(reg_font);

		EndPaint(hWnd, &ps);
		return 0;
	}
	else if (message == WM_COMMAND) {
		if (HIWORD(wParam) == BN_CLICKED) {
			BOOL checked = IsDlgButtonChecked(hWnd, 1);
			*(ptr->m_openDirOnClose) = checked;
		}
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}