#include "WelcomeFrame.h"


constexpr static auto CLASS_NAME = "WELCOME_FRAME";
static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

WelcomeFrame::~WelcomeFrame()
{
	UnregisterClass(CLASS_NAME, m_hinstance);
	DestroyWindow(m_hwnd);
}

WelcomeFrame::WelcomeFrame(const HINSTANCE hInstance, const HWND & parent, const RECT & rc)
{
	// Try to create window class
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
	m_wcex.lpszClassName = CLASS_NAME;
	m_wcex.hIconSm = LoadIcon(m_wcex.hInstance, IDI_APPLICATION);
	RegisterClassEx(&m_wcex);
	m_hwnd = CreateWindow(CLASS_NAME, CLASS_NAME, WS_OVERLAPPED | WS_VISIBLE | WS_CHILD | WS_DLGFRAME, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, parent, NULL, hInstance, NULL);	
	setVisible(false);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_PAINT) {
		PAINTSTRUCT ps;
		auto hdc = BeginPaint(hWnd, &ps);
		auto big_font = CreateFont(35, 15, 0, 0, FW_ULTRABOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FF_ROMAN, "Segoe UI");
		auto reg_font = CreateFont(17, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FF_ROMAN, "Segoe UI");
		auto reg_font_under = CreateFont(17, 7, 0, 0, FW_NORMAL, FALSE, TRUE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FF_ROMAN, "Segoe UI");
		
		// Draw Text
		constexpr static char* text[] = {
			"Welcome",
			"This custom installer was generated using nSuite",
			"Source-code can be found at:",
			"https://github.com/Yattabyte/nStallr/",
			"Press the 'Next' button to continue . . ."
		};
		SelectObject(hdc, big_font);
		SetTextColor(hdc, RGB(25, 125, 225));
		TextOut(hdc, 10, 10, text[0], (int)strlen(text[0]));

		SelectObject(hdc, reg_font);
		SetTextColor(hdc, RGB(0, 0, 0));
		TextOut(hdc, 10, 100, text[1], (int)strlen(text[1]));
		TextOut(hdc, 10, 115, text[2], (int)strlen(text[2]));

		SelectObject(hdc, reg_font_under);
		SetTextColor(hdc, RGB(0, 0, 238));
		TextOut(hdc, 35, 130, text[3], (int)strlen(text[3]));

		SelectObject(hdc, reg_font);
		SetTextColor(hdc, RGB(0, 0, 0));
		TextOut(hdc, 10, 420, text[4], (int)strlen(text[4]));

		// Cleanup
		DeleteObject(big_font);
		DeleteObject(reg_font);
		DeleteObject(reg_font_under);

		EndPaint(hWnd, &ps);
		return 0;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}