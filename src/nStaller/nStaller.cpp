#include "Installer.h"
#pragma warning(push)
#pragma warning(disable:4458)
#include <gdiplus.h>
#pragma warning(pop)


int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE, _In_ LPSTR, _In_ int)
{
	CoInitialize(NULL);
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	Installer installer(hInstance);

	// Main message loop:
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	// Close
	CoUninitialize();
	return (int)msg.wParam;
}