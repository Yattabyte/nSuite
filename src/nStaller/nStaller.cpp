#include "Common.h"
#include "BufferTools.h"
#include "DirectoryTools.h"
#include "Threader.h"
#include "Resource.h"
#include "Frames/WelcomeFrame.h"
#include "Frames/DirectoryFrame.h"
#include "Frames/InstallFrame.h"
#include "Frames/FinishFrame.h"
#include <chrono>
#include <functional>
#include <stdlib.h>
#include <Shlobj.h>
#include <tchar.h>
#include <windows.h>
#include <windowsx.h>
#include <vector>


static Threader threader(1ull);
static std::vector<Frame*> Frames;
static std::vector<std::function<void()>> FrameOperations;
static int FrameIndex = 0;
static HWND hwnd_window, hwnd_exitButton, hwnd_prevButton, hwnd_nextButton, hwnd_dialogue = NULL;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static bool CreateMainWindow(HINSTANCE, const std::string &);

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	// Get user's program files directory
	TCHAR pf[MAX_PATH];
	SHGetSpecialFolderPath(0, pf, CSIDL_PROGRAM_FILES, FALSE);
	std::string writeDirectory(pf), directoryName = "";

	// Get installer's payload
	Resource archive(IDR_ARCHIVE, "ARCHIVE");
	size_t archiveOffset(0ull);
	if (!archive.exists()) {
		// Show error screen
	}
	else {
		// Read the package header
		const auto folderSize = *reinterpret_cast<size_t*>(archive.getPtr());
		directoryName = std::string(reinterpret_cast<char*>(PTR_ADD(archive.getPtr(), size_t(sizeof(size_t)))), folderSize);
		writeDirectory += "\\" + directoryName;
		archiveOffset = size_t(sizeof(size_t)) + folderSize;
	}

	if (!CreateMainWindow(hInstance, directoryName))
		exit(EXIT_FAILURE);

	// Welcome Screen
	Frames.emplace_back(new WelcomeFrame(hInstance, hwnd_window, 170, 0, 630, 450));
	FrameOperations.emplace_back([&]() {
		ShowWindow(hwnd_exitButton, true);
		ShowWindow(hwnd_prevButton, false);
		ShowWindow(hwnd_nextButton, true);
		Frames[FrameIndex]->setVisible(true);
	});
	
	// Directory Screen
	Frames.emplace_back(new DirectoryFrame(&writeDirectory, hInstance, hwnd_window, 170, 0, 630, 450));
	FrameOperations.emplace_back([&]() {
		ShowWindow(hwnd_exitButton, true);
		ShowWindow(hwnd_prevButton, true);
		ShowWindow(hwnd_nextButton, true);
		Frames[FrameIndex]->setVisible(true);
	});

	// Installation Screen
	Frames.emplace_back(new InstallFrame(hInstance, hwnd_window, 170, 0, 630, 450));
	FrameOperations.emplace_back([&]() {
		EnableWindow(hwnd_exitButton, false);
		EnableWindow(hwnd_prevButton, false);
		EnableWindow(hwnd_nextButton, false);
		Frames[FrameIndex]->setVisible(true);

		// Acquire archive resource	
		threader.addJob([&writeDirectory, &archive, &archiveOffset]() {
			// Unpackage using the rest of the resource file
			size_t byteCount(0ull), fileCount(0ull);
			sanitize_path(writeDirectory);
			if (!DRT::DecompressDirectory(writeDirectory, reinterpret_cast<char*>(PTR_ADD(archive.getPtr(), archiveOffset)), archive.getSize() - archiveOffset, byteCount, fileCount)) {
				//	exit_program("Cannot decompress embedded package resource, aborting...\r\n");
			}
			EnableWindow(hwnd_nextButton, true);
		});		
	});

	// Finish Screen
	Frames.emplace_back(new FinishFrame(hInstance, hwnd_window, 170, 0, 630, 450));
	FrameOperations.emplace_back([&]() {
		EnableWindow(hwnd_exitButton, true);
		ShowWindow(hwnd_exitButton, true);
		ShowWindow(hwnd_prevButton, false);
		ShowWindow(hwnd_nextButton, false);
		Frames[FrameIndex]->setVisible(true);
		SetWindowText(hwnd_exitButton, "Close");
	});

	// Begin first frame
	FrameOperations[FrameIndex]();

	// Main message loop:
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}

static bool CreateMainWindow(HINSTANCE hInstance, const std::string & windowName)
{
	// Try to create window class
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
	if (!RegisterClassEx(&wcex))
		return false;

	// Try to create window object
	hwnd_window = CreateWindow(
		"nStaller", std::string(windowName + " - installer").c_str(),
		WS_OVERLAPPED | WS_VISIBLE | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT,
		800, 500,
		NULL, NULL, hInstance, NULL
	);
	constexpr auto BUTTON_STYLES = WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON;
	hwnd_prevButton = CreateWindow("BUTTON", "< Back", BUTTON_STYLES, 510, 460, 85, 30, hwnd_window, NULL, hInstance, NULL);
	hwnd_nextButton = CreateWindow("BUTTON", "Next >", BUTTON_STYLES | BS_DEFPUSHBUTTON, 600, 460, 85, 30, hwnd_window, NULL, hInstance, NULL);
	hwnd_exitButton = CreateWindow("BUTTON", "Cancel", BUTTON_STYLES, 710, 460, 85, 30, hwnd_window, NULL, hInstance, NULL);

	auto dwStyle = GetWindowLongPtr(hwnd_window, GWL_STYLE);
	auto dwExStyle = GetWindowLongPtr(hwnd_window, GWL_EXSTYLE);
	RECT rc = { 0, 0, 800, 500 };
	ShowWindow(hwnd_window, true);
	UpdateWindow(hwnd_window);
	AdjustWindowRectEx(&rc, dwStyle, false, dwExStyle);
	SetWindowPos(hwnd_window, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_NOMOVE);
	return true;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_PAINT) {
		PAINTSTRUCT ps;
		auto hdc = BeginPaint(hWnd, &ps);
		auto font = CreateFont(25, 10, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FF_ROMAN, "Segoe UI");
		SelectObject(hdc, font);

		// Background
		SelectObject(hdc, GetStockObject(DC_BRUSH));
		SelectObject(hdc, GetStockObject(DC_PEN));
		SetDCBrushColor(hdc, RGB(25,25,25));
		SetDCPenColor(hdc, RGB(25,25,25));
		SetBkColor(hdc, RGB(25,25,25));
		Rectangle(hdc, 0, 0, 800, 500);

		// Footer
		SetDCBrushColor(hdc, RGB(75,75,75));
		SetDCPenColor(hdc, RGB(75, 75, 75));
		Rectangle(hdc, 0, 450, 800, 500);

		// Steps
		constexpr static char* step_labels[] = { "Welcome", "Directory", "Install", "Finish" };
		SetDCBrushColor(hdc, RGB(100, 100, 100));
		SetDCPenColor(hdc, RGB(100, 100, 100));
		Rectangle(hdc, 26, 0, 29, 450);
		int vertical_offset = 15;
		for (int x = 0; x < 4; ++x) {
			// Draw Circle
			auto color = x == FrameIndex ? RGB(25, 225, 125) : x < FrameIndex ? RGB(25, 125, 225) : RGB(255, 255, 255);
			SetDCBrushColor(hdc, color);
			SetDCPenColor(hdc, color);
			SetTextColor(hdc, color);
			Ellipse(hdc, 20, vertical_offset + 6, 35, vertical_offset + 21);

			// Draw Text
			TextOut(hdc, 50, vertical_offset, step_labels[x], _tcslen(step_labels[x]));
			vertical_offset += 50;

			if (x == 2)
				vertical_offset = 420;
		}
		
		DeleteObject(font);
		EndPaint(hWnd, &ps);
	}
	else if (message == WM_DESTROY)
		PostQuitMessage(0);
	else if (message == WM_COMMAND) {
		if (HIWORD(wParam) == BN_CLICKED) {
			auto hndl = LOWORD(lParam);
			// A button has been clicked, so SOMETHING drastic is going to happen
			Frames[FrameIndex]->setVisible(false);
			// If exit
			if (hndl == LOWORD(hwnd_exitButton))
				PostQuitMessage(0);
			// If previous
			else if (hndl == LOWORD(hwnd_prevButton))
				FrameOperations[--FrameIndex]();
			// If next
			else if (hndl == LOWORD(hwnd_nextButton))
				FrameOperations[++FrameIndex]();
			
			RECT rc = { 0, 0, 160, 450 };
			RedrawWindow(hwnd_window, &rc, NULL, RDW_INVALIDATE);
		}
	}
	else
		return DefWindowProc(hWnd, message, wParam, lParam);
}