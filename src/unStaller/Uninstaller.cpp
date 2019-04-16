#include "Uninstaller.h"
#include "Common.h"
#include "TaskLogger.h"
#include <Shlobj.h>
#include <strsafe.h>
#include <sstream>
#pragma warning(push)
#pragma warning(disable:4458)
#include <gdiplus.h>
#pragma warning(pop)

// States used in this GUI application
#include "States/WelcomeState.h"
#include "States/UninstallState.h"
#include "States/FinishState.h"
#include "States/FailState.h"


static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE, _In_ LPSTR, _In_ int)
{
	CoInitialize(NULL);
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	Uninstaller uninstaller(hInstance);

	// Main message loop:
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

#ifdef NDEBUG
	// Delete scraps of the installation directory
	if (uninstaller.isValid()) {
		std::wstring cmd(L"cmd.exe /C ping 1.1.1.1 -n 1 -w 5000 > Nul & rmdir /q/s \"" + uninstaller.getDirectory());
		cmd.erase(std::find(cmd.begin(), cmd.end(), L'\0'), cmd.end());
		cmd += L"\"";
		STARTUPINFOW si = { 0 };
		PROCESS_INFORMATION pi = { 0 };

		CreateProcessW(NULL, (LPWSTR)cmd.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);

		std::error_code er;
		std::filesystem::remove_all(uninstaller.getDirectory(), er);
	}
#endif

	// Close
	CoUninitialize();
	return (int)msg.wParam;
}

Uninstaller::Uninstaller() 
	: m_manifest(IDR_MANIFEST, "MANIFEST")
{
	// Process manifest
	if (m_manifest.exists()) {
		// Create a string stream of the manifest file
		std::wstringstream ss;
		ss << reinterpret_cast<char*>(m_manifest.getPtr());

		// Cycle through every line, inserting attributes into the manifest map
		std::wstring attrib, value;
		while (ss >> attrib && ss >> std::quoted(value)) {
			wchar_t * k = new wchar_t[attrib.length() + 1];
			wcscpy_s(k, attrib.length() + 1, attrib.data());
			m_mfStrings[k] = value;
		}
	}
}

Uninstaller::Uninstaller(const HINSTANCE hInstance) : Uninstaller()
{
	// Ensure that a manifest exists
	bool success = true;
	if (!m_manifest.exists()) {
		TaskLogger::PushText("Critical failure: uninstaller manifest doesn't exist!\r\n");
		success = false;
	}

	// Acquire the installation directory
	m_directory = m_mfStrings[L"directory"];
	if (m_directory.empty())
		m_directory = to_wideString(get_current_directory());

	// Create window class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, (LPCSTR)IDI_ICON1);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = "Uninstaller";
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
	if (!RegisterClassEx(&wcex)) {
		TaskLogger::PushText("Critical failure: could not create main window.\r\n");
		success = false;
	}
	else {
		m_window = CreateWindowW(
			L"Uninstaller",(m_mfStrings[L"name"] + L" Uninstaller").c_str(),
			WS_OVERLAPPED | WS_VISIBLE | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
			CW_USEDEFAULT, CW_USEDEFAULT,
			800, 500,
			NULL, NULL, hInstance, NULL
		);

		// Create
		SetWindowLongPtr(m_window, GWLP_USERDATA, (LONG_PTR)this);
		constexpr auto BUTTON_STYLES = WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON;
		m_prevBtn = CreateWindow("BUTTON", "< Back", BUTTON_STYLES, 510, 460, 85, 30, m_window, NULL, hInstance, NULL);
		m_nextBtn = CreateWindow("BUTTON", "Next >", BUTTON_STYLES | BS_DEFPUSHBUTTON, 600, 460, 85, 30, m_window, NULL, hInstance, NULL);
		m_exitBtn = CreateWindow("BUTTON", "Cancel", BUTTON_STYLES, 710, 460, 85, 30, m_window, NULL, hInstance, NULL);

		auto dwStyle = (DWORD)GetWindowLongPtr(m_window, GWL_STYLE);
		auto dwExStyle = (DWORD)GetWindowLongPtr(m_window, GWL_EXSTYLE);
		RECT rc = { 0, 0, 800, 500 };
		ShowWindow(m_window, true);
		UpdateWindow(m_window);
		AdjustWindowRectEx(&rc, dwStyle, false, dwExStyle);
		SetWindowPos(m_window, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_NOMOVE);

		// The portions of the screen that change based on input
		m_states[WELCOME_STATE] = new WelcomeState(this, hInstance, m_window, { 170,0,800,450 });
		m_states[UNINSTALL_STATE] = new UninstallState(this, hInstance, m_window, { 170,0,800,450 });
		m_states[FINISH_STATE] = new FinishState(this, hInstance, m_window, { 170,0,800,450 });
		m_states[FAIL_STATE] = new FailState(this, hInstance, m_window, { 170,0,800,450 });
		setState(WELCOME_STATE);
	}

#ifndef DEBUG
	if (!success)
		invalidate();
#endif
}

void Uninstaller::invalidate()
{
	setState(FAIL_STATE);
	showButtons(false, false, true);
	enableButtons(false, false, true);
	m_valid = false;
}

bool Uninstaller::isValid() const
{
	return m_valid;
}

void Uninstaller::finish()
{
	m_finished = true;
}

void Uninstaller::setState(const StateEnums & stateIndex)
{	
	if (m_valid) {
		m_states[m_currentIndex]->setVisible(false);
		m_states[stateIndex]->enact();
		m_states[stateIndex]->setVisible(true);
		m_currentIndex = stateIndex;
		RECT rc = { 0, 0, 160, 500 };
		RedrawWindow(m_window, &rc, NULL, RDW_INVALIDATE);
	}
}

Uninstaller::StateEnums Uninstaller::getCurrentIndex() const
{
	return m_currentIndex;
}

std::wstring Uninstaller::getDirectory() const
{
	return m_directory;
}

void Uninstaller::updateButtons(const WORD btnHandle)
{
	if (btnHandle == LOWORD(m_prevBtn))
		m_states[m_currentIndex]->pressPrevious();
	else if (btnHandle == LOWORD(m_nextBtn))
		m_states[m_currentIndex]->pressNext();
	else if (btnHandle == LOWORD(m_exitBtn))
		m_states[m_currentIndex]->pressClose();
	RECT rc = { 0, 0, 160, 500 };
	RedrawWindow(m_window, &rc, NULL, RDW_INVALIDATE);
}

void Uninstaller::showButtons(const bool & prev, const bool & next, const bool & close)
{
	if (m_valid) {
		ShowWindow(m_prevBtn, prev);
		ShowWindow(m_nextBtn, next);
		ShowWindow(m_exitBtn, close);
	}
}

void Uninstaller::enableButtons(const bool & prev, const bool & next, const bool & close)
{
	if (m_valid) {
		EnableWindow(m_prevBtn, prev);
		EnableWindow(m_nextBtn, next);
		EnableWindow(m_exitBtn, close);
	}
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	auto ptr = (Uninstaller*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (message == WM_PAINT) {
		PAINTSTRUCT ps;
		Graphics graphics(BeginPaint(hWnd, &ps));
		// Draw Background
		const LinearGradientBrush backgroundGradient1(
			Point(0, 0),
			Point(0, 500),
			Color(255, 25, 25, 25),
			Color(255, 75, 75, 75)
		);
		graphics.FillRectangle(&backgroundGradient1, 0, 0, 170, 500);

		// Draw Steps
		const SolidBrush lineBrush(Color(255,100,100,100));
		graphics.FillRectangle(&lineBrush, 28, 0, 5, 500);
		constexpr static wchar_t* step_labels[] = { L"Welcome", L"Uninstall", L"Finish" };
		FontFamily  fontFamily(L"Segoe UI");
		Font        font(&fontFamily, 15, FontStyleBold, UnitPixel);
		REAL vertical_offset = 15;
		const auto frameIndex = (int)ptr->getCurrentIndex();
		for (int x = 0; x < 3; ++x) {
			// Draw Circle
			auto color = x < frameIndex ? Color(255, 100, 100, 100) : x == frameIndex ? Color(255, 25, 225, 125) : Color(255, 255, 255, 255);
			if (x == 2 && frameIndex == 3)
				color = Color(255, 225, 25, 75);
			const SolidBrush brush(color);
			Pen pen(color);
			graphics.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);
			graphics.DrawEllipse(&pen, 20, (int)vertical_offset, 20, 20 );
			graphics.FillEllipse(&brush, 20, (int)vertical_offset, 20, 20 );

			// Draw Text
			graphics.DrawString(step_labels[x], -1, &font, PointF{ 50, vertical_offset }, &brush);

			if (x == 1)
				vertical_offset = 460;
			else
				vertical_offset += 50;
		}

		// Draw -watermark-
		Font        regFont(&fontFamily, 14, FontStyleRegular, UnitPixel);
		Font        regUnderFont(&fontFamily, 14, FontStyleUnderline, UnitPixel);
		SolidBrush  greyBrush(Color(255, 127, 127, 127));
		SolidBrush  blueishBrush(Color(255, 100, 125, 175));
		graphics.DrawString(L"This software was generated using nSuite", -1, &regFont, PointF{ 180, 455 }, &greyBrush);
		graphics.DrawString(L"https://github.com/Yattabyte/nSuite", -1, &regUnderFont, PointF{ 180, 475 }, &blueishBrush);

		EndPaint(hWnd, &ps);
		return S_OK;
	}
	else if (message == WM_DESTROY)
		PostQuitMessage(0);
	else if (message == WM_COMMAND) {
		if (HIWORD(wParam) == BN_CLICKED) {
			ptr->updateButtons(LOWORD(lParam));
			return S_OK;
		}
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}