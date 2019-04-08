#include "InstallFrame.h"
#include "TaskLogger.h"
#include <CommCtrl.h>


static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

InstallFrame::~InstallFrame()
{
	UnregisterClass("INSTALL_FRAME", m_hinstance);
	DestroyWindow(m_hwnd);
	DestroyWindow(m_hwndLog);
	DestroyWindow(m_hwndPrgsBar);
	DestroyWindow(m_hwndPrgsText);
	TaskLogger::RemoveCallback_TextAdded(m_logIndex);
	TaskLogger::RemoveCallback_ProgressUpdated(m_taskIndex);
}

InstallFrame::InstallFrame(const HINSTANCE hInstance, const HWND parent, const RECT & rc)
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
	m_wcex.lpszClassName = "INSTALL_FRAME";
	m_wcex.hIconSm = LoadIcon(m_wcex.hInstance, IDI_APPLICATION);
	RegisterClassEx(&m_wcex);
	m_hwnd = CreateWindow("INSTALL_FRAME", "", WS_OVERLAPPED | WS_VISIBLE | WS_CHILD | WS_DLGFRAME, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, parent, NULL, hInstance, NULL);
	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
	m_width = rc.right - rc.left;
	m_height = rc.bottom - rc.top;

	// Create log box and progress bar
	m_hwndLog = CreateWindowEx(WS_EX_CLIENTEDGE, "edit", 0, WS_VISIBLE | WS_OVERLAPPED | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 10, 50, (rc.right - rc.left)-20, (rc.bottom - rc.top)-100, m_hwnd, NULL, hInstance, NULL);
	m_hwndPrgsBar = CreateWindowEx(WS_EX_CLIENTEDGE, PROGRESS_CLASS, 0, WS_CHILD | WS_VISIBLE | WS_OVERLAPPED | WS_DLGFRAME | WS_CLIPCHILDREN | PBS_SMOOTH, 10, (rc.bottom - rc.top) - 40, (rc.right - rc.left) - 20, 25, m_hwnd, NULL, hInstance, NULL);
	m_hwndPrgsText = CreateWindowEx(WS_EX_TRANSPARENT, "static", "0%", WS_CHILD | WS_VISIBLE | SS_CENTER, ((rc.right - rc.left) / 2)-20, 0, 40, 25, m_hwndPrgsBar, NULL, hInstance, NULL);
	SendMessage(m_hwndLog, EM_REPLACESEL, FALSE, (LPARAM)"Installation Log:\r\n");
	m_logIndex = TaskLogger::AddCallback_TextAdded([&](const std::string & message) {
		SendMessage(m_hwndLog, EM_REPLACESEL, FALSE, (LPARAM)message.c_str());
	});
	m_taskIndex = TaskLogger::AddCallback_ProgressUpdated([&](const size_t & position, const size_t & range) {
		SendMessage(m_hwndPrgsBar, PBM_SETRANGE32, 0, LPARAM(int_fast32_t(range)));
		SendMessage(m_hwndPrgsBar, PBM_SETPOS, WPARAM(int_fast32_t(position)), 0);
		std::string s = std::to_string( position == range ? 100 : int(std::floorf((float(position) / float(range)) * 100.0f)))+ "%";
		SetWindowText(m_hwndPrgsText, s.c_str());
	});

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
			Color(50, 25, 125, 225)
		);
		graphics.FillRectangle(&backgroundGradient, 0, 0, frame->m_width, frame->m_height);

		// Preparing Fonts
		FontFamily  fontFamily(L"Segoe UI");
		Font        bigFont(&fontFamily, 25, FontStyleBold, UnitPixel);
		SolidBrush  blueBrush(Color(255, 25, 125, 225));

		// Draw Text
		graphics.DrawString(L"Installing", -1, &bigFont, PointF{ 10, 10 }, &blueBrush);

		EndPaint(hWnd, &ps);
		return S_OK;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}