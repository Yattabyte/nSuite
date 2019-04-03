#include "Common.h"
#include "DirectoryTools.h"
#include "Resource.h"
#include <chrono>
#include <functional>
//
//
///** Entry point. */
//int main()
//{
//	// Check command line arguments
//	std::string dstDirectory(get_current_directory());
//
//	// Report an overview of supplied procedure
//	std::cout << 
//		"                      ~\r\n"
//		"    Installer        /\r\n"
//		"  ~-----------------~\r\n"
//		" /\r\n"
//		"~\r\n\r\n"
//		"Installing to the following directory:\r\n"
//		"\t> " + dstDirectory + "\\<package name>\r\n"
//		"\r\n";
//	pause_program("Ready to install?");
//	
//	// Acquire archive resource
//	const auto start = std::chrono::system_clock::now();
//	size_t fileCount(0ull), byteCount(0ull);
//	Resource archive(IDR_ARCHIVE, "ARCHIVE");
//	if (!archive.exists())
//		exit_program("Cannot access archive resource (may be absent, corrupt, or have different identifiers), aborting...\r\n");
//	
//	// Unpackage using the resource file
//	if (!DRT::DecompressDirectory(dstDirectory, reinterpret_cast<char*>(archive.getPtr()), archive.getSize(), byteCount, fileCount))
//		exit_program("Cannot decompress embedded package resource, aborting...\r\n");
//
//	// Success, report results
//	const auto end = std::chrono::system_clock::now();
//	const std::chrono::duration<double> elapsed_seconds = end - start;
//	std::cout
//		<< "Files written:  " << fileCount << "\r\n"
//		<< "Bytes written:  " << byteCount << "\r\n"
//		<< "Total duration: " << elapsed_seconds.count() << " seconds\r\n\r\n";
//	system("pause");
//	exit(EXIT_SUCCESS);	
//}*/

#include "Frames/WelcomeFrame.h"
#include "Frames/DirectoryFrame.h"
#include "Frames/InstallFrame.h"
#include "Frames/FinishFrame.h"
#include "Threader.h"
#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <tchar.h>
#include <vector>


constexpr static auto WINDOW_CLASS = "nStaller";
constexpr static auto WINDOW_TITLE = "Installer";
constexpr static auto HEADER_COLOR = RGB(240, 240, 240);
constexpr static auto FOOTER_COLOR = RGB(220, 220, 220);
constexpr static auto FOREGROUND_COLOR = RGB(230, 230, 230);
constexpr static auto BACKGROUND_COLOR = RGB(200, 200, 200);
static Threader threader = Threader(1ull);
static std::string directory = "E:\\Test";
static std::vector<Frame*> frames;
static std::vector<std::function<void()>> frameOperations;
static int FRAME_INDEX = 0;
static Resource ARCHIVE(IDR_ARCHIVE, "ARCHIVE");
static HWND hwnd_window, hwnd_exitButton, hwnd_prevButton, hwnd_nextButton, hwnd_dialogue = NULL;
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

static bool CreateMainWindow(HINSTANCE hInstance)
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
	wcex.lpszClassName = WINDOW_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
	if (!RegisterClassEx(&wcex))
		return false;

	// Try to create window object
	hwnd_window = CreateWindow(
		WINDOW_CLASS, WINDOW_TITLE,
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

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	if (!CreateMainWindow(hInstance))
		exit(EXIT_FAILURE);

	// Welcome Screen
	frames.emplace_back(new WelcomeFrame(hInstance, hwnd_window, 170, 0, 630, 450));
	frameOperations.emplace_back([&]() {
		ShowWindow(hwnd_exitButton, true);
		ShowWindow(hwnd_prevButton, false);
		ShowWindow(hwnd_nextButton, true);
		frames[FRAME_INDEX]->setVisible(true);
	});
	
	// Directory Screen
	frames.emplace_back(new DirectoryFrame(&directory, hInstance, hwnd_window, 170, 0, 630, 450));
	frameOperations.emplace_back([&]() {
		ShowWindow(hwnd_exitButton, true);
		ShowWindow(hwnd_prevButton, true);
		ShowWindow(hwnd_nextButton, true);
		frames[FRAME_INDEX]->setVisible(true);
	});

	// Installation Screen
	frames.emplace_back(new InstallFrame(hInstance, hwnd_window, 170, 0, 630, 450));
	frameOperations.emplace_back([&]() {
		EnableWindow(hwnd_exitButton, false);
		EnableWindow(hwnd_prevButton, false);
		EnableWindow(hwnd_nextButton, false);
		frames[FRAME_INDEX]->setVisible(true);

		// Acquire archive resource	
		threader.addJob([]() {
			const auto start = std::chrono::system_clock::now();
			size_t fileCount(0ull), byteCount(0ull);
			Resource archive(IDR_ARCHIVE, "ARCHIVE");
			if (!archive.exists()) {
				//exit_program("Cannot access archive resource (may be absent, corrupt, or have different identifiers), aborting...\r\n");
			}

			// Unpackage using the resource file
			else if (!DRT::DecompressDirectory(directory, reinterpret_cast<char*>(archive.getPtr()), archive.getSize(), byteCount, fileCount)) {
			//	exit_program("Cannot decompress embedded package resource, aborting...\r\n");
			}

			EnableWindow(hwnd_nextButton, true);
		});		
	});

	// Finish Screen
	frames.emplace_back(new FinishFrame(hInstance, hwnd_window, 170, 0, 630, 450));
	frameOperations.emplace_back([&]() {
		EnableWindow(hwnd_exitButton, true);
		ShowWindow(hwnd_exitButton, true);
		ShowWindow(hwnd_prevButton, false);
		ShowWindow(hwnd_nextButton, false);
		frames[FRAME_INDEX]->setVisible(true);
		SetWindowText(hwnd_exitButton, "Close");
	});

	// Begin first frame
	frameOperations[FRAME_INDEX]();

	// Main message loop:
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
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
			auto color = x == FRAME_INDEX ? RGB(25, 225, 125) : x < FRAME_INDEX ? RGB(25, 125, 225) : RGB(255, 255, 255);
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
			frames[FRAME_INDEX]->setVisible(false);
			// If exit
			if (hndl == LOWORD(hwnd_exitButton))
				PostQuitMessage(0);
			// If previous
			else if (hndl == LOWORD(hwnd_prevButton))
				frameOperations[--FRAME_INDEX]();
			// If next
			else if (hndl == LOWORD(hwnd_nextButton))
				frameOperations[++FRAME_INDEX]();
			
			RECT rc = { 0, 0, 160, 450 };
			RedrawWindow(hwnd_window, &rc, NULL, RDW_INVALIDATE);
		}
	}
	else
		return DefWindowProc(hWnd, message, wParam, lParam);
}