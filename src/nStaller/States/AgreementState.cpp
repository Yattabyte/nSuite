#include "AgreementState.h"
#include "../Installer.h"


static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

AgreementState::~AgreementState()
{
	UnregisterClass("AGREEMENT_STATE", m_hinstance);
	DestroyWindow(m_hwnd);
	DestroyWindow(m_checkNo);
	DestroyWindow(m_checkYes);
}

AgreementState::AgreementState(Installer * installer, const HINSTANCE hInstance, const HWND parent, const RECT & rc)
	: FrameState(installer) 
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
	m_wcex.lpszClassName = "AGREEMENT_STATE";
	m_wcex.hIconSm = LoadIcon(m_wcex.hInstance, IDI_APPLICATION);
	RegisterClassEx(&m_wcex);
	m_hwnd = CreateWindow("AGREEMENT_STATE", "", WS_OVERLAPPED | WS_VISIBLE | WS_CHILD, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, parent, NULL, hInstance, NULL);
	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
	setVisible(false);

	// Create eula
	m_log = CreateWindowExW(WS_EX_CLIENTEDGE, L"edit", m_installer->m_mfStrings[L"eula"].c_str(), WS_VISIBLE | WS_OVERLAPPED | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 10, 75, (rc.right - rc.left) - 20, (rc.bottom - rc.top) - 125, m_hwnd, NULL, hInstance, NULL);
	if (m_installer->m_mfStrings[L"eula"].empty())
		SetWindowTextW(m_log, 
			L"nSuite installers can be created freely by anyone, as such those who generate them are responsible for its contents, not the developers.\r\n"
			L"This software is provided as - is, use it at your own risk."
		);

	// Create checkboxes
	m_checkYes = CreateWindow("Button", "I accept the terms of this license agreement", WS_OVERLAPPED | WS_VISIBLE | WS_CHILD | BS_CHECKBOX | BS_AUTOCHECKBOX, 10, (rc.bottom - rc.top) - 40, 610, 15, m_hwnd, (HMENU)1, hInstance, NULL);
	m_checkNo = CreateWindow("Button", "I do not accept the terms of this license agreement", WS_OVERLAPPED | WS_VISIBLE | WS_CHILD | BS_CHECKBOX | BS_AUTOCHECKBOX, 10, (rc.bottom - rc.top) - 20, 610, 15, m_hwnd, (HMENU)2, hInstance, NULL);	
	CheckDlgButton(m_hwnd, 2, BST_CHECKED);
}

void AgreementState::enact()
{
	m_installer->showButtons(true, true, true);
	m_installer->enableButtons(true, m_agrees, true);
}

void AgreementState::pressPrevious()
{
	m_installer->setState(Installer::WELCOME_STATE);
}

void AgreementState::pressNext()
{	
	m_installer->setState(Installer::DIRECTORY_STATE);
}

void AgreementState::pressClose()
{
	// No new screen
	PostQuitMessage(0);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	auto ptr = (AgreementState*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (message == WM_PAINT) {
		PAINTSTRUCT ps;
		Graphics graphics(BeginPaint(hWnd, &ps));

		// Draw Background
		LinearGradientBrush backgroundGradient(
			Point(0, 0),
			Point(0, 450),
			Color(50, 25, 125, 225),
			Color(255, 255, 255, 255)
		);
		graphics.FillRectangle(&backgroundGradient, 0, 0, 630, 450);

		// Preparing Fonts
		FontFamily  fontFamily(L"Segoe UI");
		Font        bigFont(&fontFamily, 25, FontStyleBold, UnitPixel);
		Font        regFont(&fontFamily, 14, FontStyleRegular, UnitPixel);
		SolidBrush  blueBrush(Color(255, 25, 125, 225));
		SolidBrush  blackBrush(Color(255, 0, 0, 0));

		// Draw Text
		graphics.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);
		graphics.DrawString(L"License Agreement", -1, &bigFont, PointF{ 10, 10 }, &blueBrush);
		graphics.DrawString(L"Please read the following license agreement:", -1, &regFont, PointF{ 10, 50 }, &blackBrush);
		
		EndPaint(hWnd, &ps);
		return S_OK;
	}
	else if (message == WM_CTLCOLORSTATIC) {
		// Make checkbox text background color transparent
		if (HWND(lParam) == ptr->m_checkYes || HWND(lParam) == ptr->m_checkNo) {
			SetBkMode(HDC(wParam), TRANSPARENT);
			return (LRESULT)GetStockObject(NULL_BRUSH);
		}
		SetBkColor(HDC(wParam), RGB(255, 255, 255));
		return (LRESULT)GetStockObject(WHITE_BRUSH);
	}
	else if (message == WM_COMMAND) {
		const auto notification = HIWORD(wParam);
		if (notification == BN_CLICKED) {
			auto controlHandle = HWND(lParam);
			if (controlHandle == ptr->m_checkYes && IsDlgButtonChecked(hWnd, 1)) {
				CheckDlgButton(ptr->m_hwnd, 2, BST_UNCHECKED);
				ptr->m_installer->enableButtons(true, true, true);
				ptr->m_agrees = true;
			}
			else if (controlHandle == ptr->m_checkNo && IsDlgButtonChecked(hWnd, 2)) {
				CheckDlgButton(ptr->m_hwnd, 1, BST_UNCHECKED); 
				ptr->m_installer->enableButtons(true, false, true);
				ptr->m_agrees = false;
			}
		}
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}