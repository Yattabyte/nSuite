#include "InstallFrame.h"
#include "TaskLogger.h"
#include <CommCtrl.h>


constexpr static auto CLASS_NAME = "INSTALL_FRAME";
static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

InstallFrame::~InstallFrame()
{
	UnregisterClass(CLASS_NAME, m_hinstance);
	DestroyWindow(m_hwnd);
	DestroyWindow(m_hwndLog);
	DestroyWindow(m_hwndPrgsBar);
	DestroyWindow(m_hwndPrgsText);
	TaskLogger::GetInstance().removeCallback_Text(m_logIndex);
	TaskLogger::GetInstance().removeCallback_Progress(m_taskIndex);
}

InstallFrame::InstallFrame(const HINSTANCE & hInstance, const HWND & parent, const RECT & rc)
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

	// Create log box
	m_hwndLog = CreateWindowEx(WS_EX_CLIENTEDGE, "edit", 0, WS_VISIBLE | WS_OVERLAPPED | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 10, 50, (rc.right - rc.left)-20, (rc.bottom - rc.top)-100, m_hwnd, NULL, hInstance, NULL);
	m_hwndPrgsBar = CreateWindowEx(WS_EX_CLIENTEDGE, PROGRESS_CLASS, 0, WS_CHILD | WS_VISIBLE | WS_OVERLAPPED | WS_DLGFRAME | WS_CLIPCHILDREN | PBS_SMOOTH, 10, (rc.bottom - rc.top) - 40, (rc.right - rc.left) - 20, 25, m_hwnd, NULL, hInstance, NULL);
	m_hwndPrgsText = CreateWindowEx(WS_EX_TRANSPARENT, "static", "0%", WS_CHILD | WS_VISIBLE | SS_CENTER, ((rc.right - rc.left) / 2)-20, 0, 40, 25, m_hwndPrgsBar, NULL, hInstance, NULL);
	auto & logger = TaskLogger::GetInstance();
	SendMessage(m_hwndLog, EM_REPLACESEL, FALSE, (LPARAM)"Installation Log:\r\n");
	SendMessage(m_hwndLog, EM_REPLACESEL, FALSE, (LPARAM)logger.getLog().c_str());
	m_logIndex = logger.addCallback_Text([&](const std::string & message) {
		SendMessage(m_hwndLog, EM_REPLACESEL, FALSE, (LPARAM)message.c_str());
	});
	m_taskIndex = logger.addCallback_Progress([&](const size_t & position, const size_t & range) {
		SendMessage(m_hwndPrgsBar, PBM_SETRANGE32, 0, LPARAM(int_fast32_t(range)));
		SendMessage(m_hwndPrgsBar, PBM_SETPOS, WPARAM(int_fast32_t(position)), 0);
		std::string s = std::to_string( position == range ? 100 : int(std::floorf((float(position) / float(range)) * 100.0f)))+ "%";
		SetWindowText(m_hwndPrgsText, s.c_str());
	});

	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
	setVisible(false);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_PAINT) {
		PAINTSTRUCT ps;
		auto hdc = BeginPaint(hWnd, &ps);
		auto big_font = CreateFont(35, 15, 0, 0, FW_ULTRABOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FF_ROMAN, "Segoe UI");
		auto reg_font = CreateFont(17, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FF_ROMAN, "Segoe UI");

		// Draw Title
		constexpr static auto text = "Installing";
		SelectObject(hdc, big_font);
		SetTextColor(hdc, RGB(25, 125, 225));
		TextOut(hdc, 10, 10, text, (int)strlen(text));
		
		// Cleanup
		DeleteObject(big_font);
		DeleteObject(reg_font);

		EndPaint(hWnd, &ps);
		return 0;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}