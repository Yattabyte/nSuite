#include "InstallFrame.h"
#include "TaskLogger.h"
#include <CommCtrl.h>
#include <tchar.h>


constexpr static auto CLASS_NAME = "INSTALL_FRAME";
constexpr static auto FOREGROUND_COLOR = RGB(230, 230, 230);
constexpr static auto FOREGROUND_COLOR2 = RGB(255, 255, 255);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

InstallFrame::~InstallFrame()
{
	UnregisterClass(CLASS_NAME, m_hinstance);
	DeleteObject(m_hwnd);
	TaskLogger::GetInstance().removeCallback_Text(m_logIndex);
	TaskLogger::GetInstance().removeCallback_Progress(m_taskIndex);
}

InstallFrame::InstallFrame(const HINSTANCE & hInstance, const HWND & parent, const int & x, const int & y, const int & w, const int & h)
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
	m_hwnd = CreateWindow(CLASS_NAME, CLASS_NAME, WS_OVERLAPPED | WS_VISIBLE | WS_CHILD | WS_DLGFRAME, x, y, w, h, parent, NULL, hInstance, NULL);

	// Create log box
	m_hwndLog = CreateWindowEx(WS_EX_CLIENTEDGE, "edit", 0, WS_VISIBLE | WS_OVERLAPPED | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 10, 50, w-20, h-100, m_hwnd, NULL, hInstance, NULL);
	m_hwndPrgsBar = CreateWindowEx(WS_EX_CLIENTEDGE, PROGRESS_CLASS, 0, WS_CHILD | WS_VISIBLE | WS_OVERLAPPED | WS_DLGFRAME | WS_CLIPCHILDREN | PBS_SMOOTH, 10, h - 40, w - 20, 25, m_hwnd, NULL, hInstance, NULL);
	m_hwndPrgsText = CreateWindowEx(WS_EX_TRANSPARENT, "static", "0%", WS_CHILD | WS_VISIBLE | SS_CENTER, (w / 2)-20, 0, 40, 25, m_hwndPrgsBar, NULL, hInstance, NULL);
	SendMessage(m_hwndLog, EM_REPLACESEL, FALSE, (LPARAM)"Installation Log:\r\n");
	auto & logger = TaskLogger::GetInstance();
	m_logIndex = logger.addCallback_Text([&](const std::string & message) {
		SendMessage(m_hwndLog, EM_REPLACESEL, FALSE, (LPARAM)message.c_str());
	});
	m_taskIndex = logger.addCallback_Progress([&](const size_t & position, const size_t & range) {
		SendMessage(m_hwndPrgsBar, PBM_SETRANGE32, 0, LPARAM(int_fast32_t(range)));
		SendMessage(m_hwndPrgsBar, PBM_SETPOS, WPARAM(int_fast32_t(position)), 0);
		std::string s = std::to_string(int((float(position) / float(range)) * 100.0f))+ "%";
		SetWindowText(m_hwndPrgsText, s.c_str());
	});

	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
	setVisible(false);
}

static LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	auto ptr = (InstallFrame*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (message == WM_PAINT) {
		PAINTSTRUCT ps;
		auto hdc = BeginPaint(hWnd, &ps);
		auto big_font = CreateFont(35, 15, 0, 0, FW_ULTRABOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FF_ROMAN, "Segoe UI");
		auto reg_font = CreateFont(17, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FF_ROMAN, "Segoe UI");

		// Draw Title
		constexpr static auto text = "Installing";
		SelectObject(hdc, big_font);
		SetTextColor(hdc, RGB(25, 125, 225));
		TextOut(hdc, 10, 10, text, _tcslen(text));
		
		// Cleanup
		DeleteObject(big_font);
		DeleteObject(reg_font);

		EndPaint(hWnd, &ps);
		return 0;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}