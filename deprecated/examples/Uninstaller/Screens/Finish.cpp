#include "Screens/Finish.h"
#include "Uninstaller.h"
#include <algorithm>
#include <filesystem>


static LRESULT CALLBACK WndProc(HWND /*hWnd*/, UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/);

Finish_Screen::~Finish_Screen()
{
	UnregisterClass("FINISH_SCREEN", m_hinstance);
	DestroyWindow(m_hwnd);
	DestroyWindow(m_btnClose);
}

Finish_Screen::Finish_Screen(Uninstaller* unInstaller, const HINSTANCE hInstance, const HWND parent, const vec2& pos, const vec2& size)
	: Screen(unInstaller, pos, size)
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
	m_wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	m_wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	m_wcex.lpszMenuName = nullptr;
	m_wcex.lpszClassName = "FINISH_SCREEN";
	m_wcex.hIconSm = LoadIcon(m_wcex.hInstance, IDI_APPLICATION);
	RegisterClassEx(&m_wcex);
	m_hwnd = CreateWindow("FINISH_SCREEN", "", WS_OVERLAPPED | WS_CHILD | WS_VISIBLE, pos.x, pos.y, size.x, size.y, parent, nullptr, hInstance, nullptr);
	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
	setVisible(false);

	// Create Buttons
	constexpr auto BUTTON_STYLES = WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON;
	m_btnClose = CreateWindow("BUTTON", "Close", BUTTON_STYLES, size.x - 95, size.y - 40, 85, 30, m_hwnd, nullptr, hInstance, nullptr);
}

void Finish_Screen::enact()
{
	// Does nothing
}

void Finish_Screen::paint()
{
	PAINTSTRUCT ps;
	Graphics graphics(BeginPaint(m_hwnd, &ps));

	// Draw Background
	LinearGradientBrush backgroundGradient(
		Point(0, 0),
		Point(0, m_size.y),
		Color(50, 25, 255, 125),
		Color(255, 255, 255, 255)
	);
	graphics.FillRectangle(&backgroundGradient, 0, 0, m_size.x, m_size.y);

	// Preparing Fonts
	FontFamily  fontFamily(L"Segoe UI");
	Font        bigFont(&fontFamily, 25, FontStyleBold, UnitPixel);
	SolidBrush  blueBrush(Color(255, 25, 125, 225));

	// Draw Text
	graphics.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);
	graphics.DrawString(L"Un-Installation Complete", -1, &bigFont, PointF{ 10, 10 }, &blueBrush);

	EndPaint(m_hwnd, &ps);
}

void Finish_Screen::goClose()
{
#ifndef DEBUG
	// Delete scraps of the Installation directory (nuke the remaining directory)
	std::wstring cmd(L"cmd.exe /C ping 1.1.1.1 -n 1 -w 5000 > Nul & rmdir /q/s \"" + m_unInstaller->getDirectory());
	cmd.erase(std::find(cmd.begin(), cmd.end(), L'\0'), cmd.end());
	cmd += L"\"";
	STARTUPINFOW si = { 0 };
	PROCESS_INFORMATION pi = { nullptr };

	CreateProcessW(nullptr, (LPWSTR)cmd.c_str(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	std::error_code er;
	std::filesystem::remove_all(m_unInstaller->getDirectory(), er);
#endif
	PostQuitMessage(0);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	const auto ptr = reinterpret_cast<Finish_Screen*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	const auto controlHandle = HWND(lParam);
	if (message == WM_PAINT)
		ptr->paint();
	else if (message == WM_COMMAND) {
		const auto notification = HIWORD(wParam);
		if (notification == BN_CLICKED) {
			if (controlHandle == ptr->m_btnClose)
				ptr->goClose();
		}
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}