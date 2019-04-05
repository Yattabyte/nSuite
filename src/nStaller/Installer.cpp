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
	wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
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
			"nStaller", std::string(m_packageName + " - installer").c_str(),
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
		m_frames[FINISH_FRAME] = new FinishFrame(&m_openDirectoryOnClose, hInstance, m_window, { 170,0,800,450 });
		m_frames[FAIL_FRAME] = new FailFrame(hInstance, m_window, { 170,0,800,450 });
		setState(new WelcomeState(this));
	}
	if (!success)
		invalidate();
}

void Installer::invalidate()
{
	setState(new FailState(this));
	showButtons(false, false, true);
	enableButtons(false, false, true);
	m_valid = false;
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
		auto hdc = BeginPaint(hWnd, &ps);
		auto font = CreateFont(25, 10, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FF_ROMAN, "Segoe UI");
		SelectObject(hdc, font);

		// Background
		SelectObject(hdc, GetStockObject(DC_BRUSH));
		SelectObject(hdc, GetStockObject(DC_PEN));
		SetDCBrushColor(hdc, RGB(25, 25, 25));
		SetDCPenColor(hdc, RGB(25, 25, 25));
		SetBkColor(hdc, RGB(25, 25, 25));
		Rectangle(hdc, 0, 0, 800, 500);

		// Footer
		SetDCBrushColor(hdc, RGB(75, 75, 75));
		SetDCPenColor(hdc, RGB(75, 75, 75));
		Rectangle(hdc, 0, 450, 800, 500);

		// Steps
		constexpr static char* step_labels[] = { "Welcome", "Directory", "Install", "Finish" };
		SetDCBrushColor(hdc, RGB(100, 100, 100));
		SetDCPenColor(hdc, RGB(100, 100, 100));
		Rectangle(hdc, 26, 0, 29, 450);
		int vertical_offset = 15;
		int frameIndex = (int)ptr->getCurrentIndex();
		for (int x = 0; x < 4; ++x) {
			// Draw Circle
			auto color = x == frameIndex ? RGB(25, 225, 125) : RGB(255, 255, 255);
			if (x == 3 && frameIndex == 4)
				color = RGB(225, 25, 25);
			SetDCBrushColor(hdc, color);
			SetDCPenColor(hdc, color);
			SetTextColor(hdc, color);
			Ellipse(hdc, 20, vertical_offset + 6, 35, vertical_offset + 21);

			// Draw Text
			TextOut(hdc, 50, vertical_offset, step_labels[x], (int)strlen(step_labels[x]));
			vertical_offset += 50;

			if (x == 2)
				vertical_offset = 420;
		}

		DeleteObject(font);
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
