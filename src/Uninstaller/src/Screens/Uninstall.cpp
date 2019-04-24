#include "Screens/Uninstall.h"
#include "Common.h"
#include "Uninstaller.h"


static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

Uninstall::~Uninstall()
{
	UnregisterClass("UNINSTALL_SCREEN", m_hinstance);
	DestroyWindow(m_hwnd);
	DestroyWindow(m_hwndLog);
	DestroyWindow(m_hwndPrgsBar);
	DestroyWindow(m_btnFinish);
	TaskLogger::RemoveCallback_TextAdded(m_logIndex);
	TaskLogger::RemoveCallback_ProgressUpdated(m_taskIndex);
}

Uninstall::Uninstall(Uninstaller * uninstaller, const HINSTANCE hInstance, const HWND parent, const vec2 & pos, const vec2 & size)
	: Screen(uninstaller, pos, size)
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
	m_wcex.lpszClassName = "UNINSTALL_SCREEN";
	m_wcex.hIconSm = LoadIcon(m_wcex.hInstance, IDI_APPLICATION);
	RegisterClassEx(&m_wcex);
	m_hwnd = CreateWindow("UNINSTALL_SCREEN", "", WS_OVERLAPPED | WS_CHILD | WS_VISIBLE, pos.x, pos.y, size.x, size.y, parent, NULL, hInstance, NULL);
	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
	setVisible(false);

	// Create log box and progress bar
	m_hwndLog = CreateWindowEx(WS_EX_CLIENTEDGE, "edit", 0, WS_VISIBLE | WS_OVERLAPPED | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 10, 75, size.x - 20, size.y - 125, m_hwnd, NULL, hInstance, NULL);
	m_hwndPrgsBar = CreateWindowEx(WS_EX_CLIENTEDGE, PROGRESS_CLASS, 0, WS_CHILD | WS_VISIBLE | WS_OVERLAPPED | WS_DLGFRAME | WS_CLIPCHILDREN | PBS_SMOOTH, 10, size.y - 40, size.x - 115, 30, m_hwnd, NULL, hInstance, NULL);
	m_logIndex = TaskLogger::AddCallback_TextAdded([&](const std::string & message) {
		SendMessage(m_hwndLog, EM_REPLACESEL, FALSE, (LPARAM)message.c_str());
	});
	m_taskIndex = TaskLogger::AddCallback_ProgressUpdated([&](const size_t & position, const size_t & range) {
		SendMessage(m_hwndPrgsBar, PBM_SETRANGE32, 0, LPARAM(int_fast32_t(range)));
		SendMessage(m_hwndPrgsBar, PBM_SETPOS, WPARAM(int_fast32_t(position)), 0);
		RECT rc = { 580, 410, 800, 450 };
		RedrawWindow(m_hwnd, &rc, NULL, RDW_INVALIDATE);

		std::string progress = std::to_string(position == range ? 100 : int(std::floorf((float(position) / float(range)) * 100.0f))) + "%";
		EnableWindow(m_btnFinish, position == range);
		SetWindowTextA(m_btnFinish, position == range ? "Finish >" : progress.c_str());
	});
	
	// Create Buttons
	constexpr auto BUTTON_STYLES = WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON;
	m_btnFinish = CreateWindow("BUTTON", "Finish", BUTTON_STYLES, size.x - 95, size.y - 40, 85, 30, m_hwnd, NULL, hInstance, NULL);
	EnableWindow(m_btnFinish, false);
}

void Uninstall::enact()
{
	m_uninstaller->beginUninstallation();
}

void Uninstall::paint()
{
	PAINTSTRUCT ps;
	Graphics graphics(BeginPaint(m_hwnd, &ps));

	// Draw Background
	LinearGradientBrush backgroundGradient(
		Point(0, 0),
		Point(0, m_size.y),
		Color(50, 25, 125, 225),
		Color(255, 255, 255, 255)
	);
	graphics.FillRectangle(&backgroundGradient, 0, 0, m_size.x, m_size.y);

	// Preparing Fonts
	FontFamily  fontFamily(L"Segoe UI");
	Font        bigFont(&fontFamily, 25, FontStyleBold, UnitPixel);
	SolidBrush  blueBrush(Color(255, 25, 125, 225));

	// Draw Text
	graphics.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);
	graphics.DrawString(L"Uninstalling", -1, &bigFont, PointF{ 10, 10 }, &blueBrush);

	EndPaint(m_hwnd, &ps);
}

void Uninstall::goFinish()
{
	m_uninstaller->setScreen(Uninstaller::ScreenEnums::FINISH_SCREEN);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	const auto ptr = (Uninstall*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	const auto controlHandle = HWND(lParam);
	if (message == WM_PAINT)
		ptr->paint();
	else if (message == WM_CTLCOLORSTATIC) {
		// Make log color white
		SetBkColor(HDC(wParam), RGB(255, 255, 255));
		return (LRESULT)GetStockObject(WHITE_BRUSH);
	}
	else if (message == WM_COMMAND) {
		const auto notification = HIWORD(wParam);
		if (notification == BN_CLICKED) {
			if (controlHandle == ptr->m_btnFinish)
				ptr->goFinish();
		}
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}