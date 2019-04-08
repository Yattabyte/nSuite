#include "Installer.h"
#include "Common.h"
#include "TaskLogger.h"
#include <Shlobj.h>

// Starting State
#include "States/WelcomeState.h"
#include "States/FailState.h"

// Frames used in this GUI application
#include "Frames/WelcomeFrame.h"
#include "Frames/DirectoryFrame.h"
#include "Frames/InstallFrame.h"
#include "Frames/FinishFrame.h"
#include "Frames/FailFrame.h"


static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

Installer::Installer(const HINSTANCE hInstance)
	: m_archive(IDR_ARCHIVE, "ARCHIVE")
{
	bool success = true;
	// Get user's program files directory
	TCHAR pf[MAX_PATH];
	SHGetSpecialFolderPath(0, pf, CSIDL_PROGRAM_FILES, FALSE);
	m_directory = std::string(pf);
	
	// Check archive integrity
	if (!m_archive.exists()) {
		TaskLogger::PushText("Critical failure: archive doesn't exist!\r\n");
		success = false;
	}
	else {
		const auto folderSize = *reinterpret_cast<size_t*>(m_archive.getPtr());
		m_packageName = std::string(reinterpret_cast<char*>(PTR_ADD(m_archive.getPtr(), size_t(sizeof(size_t)))), folderSize);
		m_directory += "\\" + m_packageName;
		m_packagePtr = reinterpret_cast<char*>(PTR_ADD(m_archive.getPtr(), size_t(sizeof(size_t)) + folderSize));
		m_packageSize = m_archive.getSize() - (size_t(sizeof(size_t)) + folderSize);
	}
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
	wcex.lpszClassName = "nStaller";
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
	if (!RegisterClassEx(&wcex)) {
		TaskLogger::PushText("Critical failure: could not create main window.\r\n");
		success = false;
	}
	else {
		m_window = CreateWindow(
			"nStaller", std::string(m_packageName + " Installer").c_str(),
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
		m_frames[WELCOME_FRAME] = new WelcomeFrame(hInstance, m_window, { 170,0,800,450 });
		m_frames[DIRECTORY_FRAME] = new DirectoryFrame(&m_directory, hInstance, m_window, { 170,0,800,450 });
		m_frames[INSTALL_FRAME] = new InstallFrame(hInstance, m_window, { 170,0,800,450 });
		m_frames[FINISH_FRAME] = new FinishFrame(&m_showDirectoryOnClose, hInstance, m_window, { 170,0,800,450 });
		m_frames[FAIL_FRAME] = new FailFrame(hInstance, m_window, { 170,0,800,450 });
		setState(new WelcomeState(this));
	}

#ifndef DEBUG
	if (!success)
		invalidate();
#endif
}

void Installer::invalidate()
{
	setState(new FailState(this));
	showButtons(false, false, true);
	enableButtons(false, false, true);
	m_valid = false;
}

void Installer::finish()
{
	m_finished = true;
}

void Installer::showFrame(const FrameEnums & newIndex)
{
	m_frames[m_currentIndex]->setVisible(false);
	m_frames[newIndex]->setVisible(true);
	m_currentIndex = newIndex;
}

void Installer::setState(State * state)
{
	if (!m_valid) 
		delete state; // refuse new states
	else {
		if (m_state != nullptr)
			delete m_state;
		m_state = state;
		m_state->enact();
		RECT rc = { 0, 0, 160, 450 };
		RedrawWindow(m_window, &rc, NULL, RDW_INVALIDATE);
	}
}

Installer::FrameEnums Installer::getCurrentIndex() const
{
	return m_currentIndex;
}

std::string Installer::getDirectory() const
{
	return m_directory;
}

bool Installer::shouldShowDirectory() const
{
	return m_showDirectoryOnClose && m_valid && m_finished;
}

char * Installer::getPackagePointer() const
{
	return m_packagePtr;
}

size_t Installer::getPackageSize() const
{
	return m_packageSize;
}

void Installer::updateButtons(const WORD btnHandle)
{
	if (btnHandle == LOWORD(m_prevBtn))
		m_state->pressPrevious();
	else if (btnHandle == LOWORD(m_nextBtn))
		m_state->pressNext();
	else if (btnHandle == LOWORD(m_exitBtn))
		m_state->pressClose();
	RECT rc = { 0, 0, 160, 450 };
	RedrawWindow(m_window, &rc, NULL, RDW_INVALIDATE);
}

void Installer::showButtons(const bool & prev, const bool & next, const bool & close)
{
	if (m_valid) {
		ShowWindow(m_prevBtn, prev);
		ShowWindow(m_nextBtn, next);
		ShowWindow(m_exitBtn, close);
	}
}

void Installer::enableButtons(const bool & prev, const bool & next, const bool & close)
{
	if (m_valid) {
		EnableWindow(m_prevBtn, prev);
		EnableWindow(m_nextBtn, next);
		EnableWindow(m_exitBtn, close);
	}
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	auto ptr = (Installer*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (message == WM_PAINT) {
		PAINTSTRUCT ps;
		Graphics graphics(BeginPaint(hWnd, &ps));
		// Draw Background
		const LinearGradientBrush backgroundGradient(
			Point(0, 0),
			Point(0, 500),
			Color(255, 25, 25, 25),
			Color(255, 75, 75, 75)
		);
		graphics.FillRectangle(&backgroundGradient, 0, 0, 800, 500);
		graphics.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);

		// Draw Steps
		const SolidBrush lineBrush(Color(255,100,100,100));
		graphics.FillRectangle(&lineBrush, 28, 0, 4, 500);
		constexpr static wchar_t* step_labels[] = { L"Welcome", L"Directory", L"Install", L"Finish" };
		FontFamily  fontFamily(L"Segoe UI");
		Font        font(&fontFamily, 15, FontStyleBold, UnitPixel);
		REAL vertical_offset = 15;
		const auto frameIndex = (int)ptr->getCurrentIndex();
		for (int x = 0; x < 4; ++x) {
			// Draw Circle
			auto color = x == frameIndex ? Color(255, 25, 225, 125) : Color(255, 255, 255, 255);
			if (x == 3 && frameIndex == 4)
				color = Color(255, 225, 25, 75);
			const SolidBrush brush(color);
			Pen pen(color);
			graphics.DrawEllipse(&pen, 20, (int)vertical_offset, 20, 20 );
			graphics.FillEllipse(&brush, 20, (int)vertical_offset, 20, 20 );

			// Draw Text
			graphics.DrawString(step_labels[x], -1, &font, PointF{ 50, vertical_offset }, &brush);

			if (x == 2)
				vertical_offset = 460;
			else
				vertical_offset += 50;
		}

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
