#include "Installer.h"


int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE, _In_ LPSTR, _In_ int)
{
	Installer installer(hInstance);

	// Main message loop:
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Open the installation directory if finished successfully + if user wants it to
	if (installer.shouldShowDirectory())
		ShellExecute(NULL, "open", installer.getDirectory().c_str(), NULL, NULL, SW_SHOWDEFAULT);

	// Close
	return (int)msg.wParam;
}