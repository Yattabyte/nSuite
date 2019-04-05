#include "FailFrame.h"
#include "TaskLogger.h"
#include <shlobj.h>
#include <shlwapi.h>


constexpr static auto CLASS_NAME = "FAIL_FRAME";
static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

FailFrame::~FailFrame()
{
	UnregisterClass(CLASS_NAME, m_hinstance);
	DestroyWindow(m_hwnd);
	DestroyWindow(m_hwndLog);
	TaskLogger::GetInstance().removeCallback_Text(m_logIndex);
}

FailFrame::FailFrame(const HINSTANCE & hInstance, const HWND & parent, const RECT & rc)
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
	m_wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	m_wcex.lpszMenuName = NULL;
	m_wcex.lpszClassName = CLASS_NAME;
	m_wcex.hIconSm = LoadIcon(m_wcex.hInstance, IDI_APPLICATION);
	RegisterClassEx(&m_wcex);
	m_hwnd = CreateWindow(CLASS_NAME, CLASS_NAME, WS_OVERLAPPED | WS_VISIBLE | WS_CHILD | WS_DLGFRAME, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, parent, NULL, hInstance, NULL);

	// Failure log
	m_hwndLog = CreateWindowEx(WS_EX_CLIENTEDGE, "edit", 0, WS_VISIBLE | WS_OVERLAPPED | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 10, 50, (rc.right - rc.left) - 20, (rc.bottom - rc.top) - 100, m_hwnd, NULL, hInstance, NULL);
	auto & logger = TaskLogger::GetInstance();
	SendMessage(m_hwndLog, EM_REPLACESEL, FALSE, (LPARAM)"Error Log:\r\n");
	SendMessage(m_hwndLog, EM_REPLACESEL, FALSE, (LPARAM)logger.getLog().c_str());
	m_logIndex = logger.addCallback_Text([&](const std::string & message) {
		SendMessage(m_hwndLog, EM_REPLACESEL, FALSE, (LPARAM)message.c_str());
	});
	
	setVisible(false);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_PAINT) {
		PAINTSTRUCT ps;
		auto hdc = BeginPaint(hWnd, &ps);
		auto big_font = CreateFont(35, 15, 0, 0, FW_ULTRABOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FF_ROMAN, "Segoe UI");
		auto reg_font = CreateFont(17, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FF_ROMAN, "Segoe UI");

		// Draw Text
		constexpr static char* text[] = {
			"Installation incomplete, see log below:",
			"Press the 'Close' button to finish . . ."
		};
		SelectObject(hdc, big_font);
		SetTextColor(hdc, RGB(25, 125, 225));
		TextOut(hdc, 10, 10, text[0], (int)strlen(text[0]));

		SelectObject(hdc, reg_font);
		SetTextColor(hdc, RGB(0, 0, 0));
		TextOut(hdc, 10, 420, text[1], (int)strlen(text[1]));

		// Cleanup
		DeleteObject(big_font);
		DeleteObject(reg_font);

		EndPaint(hWnd, &ps);
		return 0;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}