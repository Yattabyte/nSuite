#include "Screens/Directory.h"
#include "Installer.h"
#include <iomanip>
#include <shlobj.h>
#include <shlwapi.h>
#include <sstream>
#include <vector>


static LRESULT CALLBACK WndProc(HWND /*hWnd*/, UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/);
static HRESULT CreateDialogEventHandler(REFIID, void** /*ppv*/);
static HRESULT OpenFileDialog(std::string& /*directory*/);

Directory_Screen::~Directory_Screen()
{
	UnregisterClass("DIRECTORY_SCREEN", m_hinstance);
	DestroyWindow(m_hwnd);
	DestroyWindow(m_directoryField);
	DestroyWindow(m_packageField);
	DestroyWindow(m_browseButton);
	DestroyWindow(m_btnPrev);
	DestroyWindow(m_btnIyatta);
	DestroyWindow(m_btnCancel);
}

Directory_Screen::Directory_Screen(Installer* Installer, const HINSTANCE hInstance, const HWND parent, const vec2& pos, const vec2& size)
	: Screen(Installer, pos, size)
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
	m_wcex.lpszClassName = "DIRECTORY_SCREEN";
	m_wcex.hIconSm = LoadIcon(m_wcex.hInstance, IDI_APPLICATION);
	RegisterClassEx(&m_wcex);
	m_hwnd = CreateWindow("DIRECTORY_SCREEN", "", WS_OVERLAPPED | WS_CHILD | WS_VISIBLE, pos.x, pos.y, size.x, size.y, parent, nullptr, hInstance, nullptr);
	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
	setVisible(false);

	// Create directory lookup fields
	m_directoryField = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", m_Installer->getDirectory().c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 10, 150, 400, 25, m_hwnd, nullptr, hInstance, nullptr);
	m_packageField = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", ("\\" + m_Installer->getPackageName()).c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT | ES_READONLY, 410, 150, 100, 25, m_hwnd, nullptr, hInstance, nullptr);
	m_browseButton = CreateWindow("BUTTON", "Browse", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 520, 149, 100, 25, m_hwnd, nullptr, hInstance, nullptr);

	// Create Buttons
	constexpr auto BUTTON_STYLES = WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON;
	m_btnPrev = CreateWindow("BUTTON", "< Back", BUTTON_STYLES | BS_DEFPUSHBUTTON, size.x - 290, size.y - 40, 85, 30, m_hwnd, nullptr, hInstance, nullptr);
	m_btnIyatta = CreateWindow("BUTTON", "Install >", BUTTON_STYLES | BS_DEFPUSHBUTTON, size.x - 200, size.y - 40, 85, 30, m_hwnd, nullptr, hInstance, nullptr);
	m_btnCancel = CreateWindow("BUTTON", "Cancel", BUTTON_STYLES, size.x - 95, size.y - 40, 85, 30, m_hwnd, nullptr, hInstance, nullptr);
}

void Directory_Screen::enact()
{
	// Does nothing
}

void Directory_Screen::paint()
{
	PAINTSTRUCT ps;
	Graphics graphics(BeginPaint(m_hwnd, &ps));

	// Draw Background
	SolidBrush solidWhiteBackground(Color(255, 255, 255, 255));
	graphics.FillRectangle(&solidWhiteBackground, 0, 0, 630, 500);
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
	Font        regFont(&fontFamily, 14, FontStyleRegular, UnitPixel);
	SolidBrush  blueBrush(Color(255, 25, 125, 225));
	SolidBrush  blackBrush(Color(255, 0, 0, 0));

	// Draw Text
	graphics.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);
	graphics.DrawString(L"Where would you like to Install to?", -1, &bigFont, PointF{ 10, 10 }, &blueBrush);
	graphics.DrawString(L"Choose a folder by pressing the 'Browse' button.", -1, &regFont, PointF{ 10, 100 }, &blackBrush);
	graphics.DrawString(L"Alternatively, type a specific directory into the box below.", -1, &regFont, PointF{ 10, 115 }, &blackBrush);

	constexpr static auto readableFileSize = [](const size_t& size) -> std::wstring {
		auto remainingSize = (double)size;
		constexpr static wchar_t* units[] = { L" B", L" KB", L" MB", L" GB", L" TB", L" PB", L" EB" };
		int i = 0;
		while (remainingSize > 1024.00) {
			remainingSize /= 1024.00;
			++i;
		}
		std::wstringstream stream;
		stream << std::fixed << std::setprecision(2) << remainingSize;
		return stream.str() + units[i] + L" (" + std::to_wstring(size) + L" bytes )";
	};
	graphics.DrawString(L"Disk Space", -1, &regFont, PointF{ 10, 200 }, &blackBrush);
	graphics.DrawString((L"    Capacity:\t\t\t" + readableFileSize(m_Installer->getDirectorySizeCapacity())).c_str(), -1, &regFont, PointF{ 10, 225 }, &blackBrush);
	graphics.DrawString((L"    Available:\t\t\t" + readableFileSize(m_Installer->getDirectorySizeAvailable())).c_str(), -1, &regFont, PointF{ 10, 240 }, &blackBrush);
	graphics.DrawString((L"    Required:\t\t\t" + readableFileSize(m_Installer->getDirectorySizeRequired())).c_str(), -1, &regFont, PointF{ 10, 255 }, &blackBrush);


	EndPaint(m_hwnd, &ps);
}

void Directory_Screen::browse()
{
	std::string directory;
	if (SUCCEEDED(OpenFileDialog(directory))) {
		if (!directory.empty() && directory.length() > 2ULL) {
			m_Installer->setDirectory(directory);
			SetWindowTextA(m_directoryField, directory.c_str());
			RECT rc = { 10, 200, 600, 300 };
			RedrawWindow(m_hwnd, &rc, nullptr, RDW_INVALIDATE);
		}
	}
}

void Directory_Screen::goPrevious()
{
	m_Installer->setScreen(Installer::ScreenEnums::AGREEMENT_SCREEN);
}

void Directory_Screen::goInstall()
{
	const auto directory = m_Installer->getDirectory();
	if (directory.empty() || directory == " " || directory.length() < 3)
		MessageBox(
			nullptr,
			"Please enter a valid directory before proceeding.",
			"Invalid path!",
			MB_OK | MB_ICONERROR | MB_TASKMODAL
		);
	else
		m_Installer->setScreen(Installer::ScreenEnums::Install_SCREEN);
}

void Directory_Screen::goCancel()
{
	PostQuitMessage(0);
}

static HRESULT CreateDialogEventHandler(REFIID riid, void** ppv)
{
	/** File Dialog Event Handler */
	class DialogEventHandler : public IFileDialogEvents, public IFileDialogControlEvents {
	private:
		virtual ~DialogEventHandler() = default;;
		long _cRef{ 1 };


	public:
		// Coyattaructor
		DialogEventHandler() = default;;


		// IUnknown methods
		IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv) {
			static const QITAB qit[] = {
				QITABENT(DialogEventHandler, IFileDialogEvents),
				QITABENT(DialogEventHandler, IFileDialogControlEvents),
				{ nullptr },
			};
			return QISearch(this, qit, riid, ppv);
		}
		IFACEMETHODIMP_(ULONG) AddRef() {
			return InterlockedIncrement(&_cRef);
		}
		IFACEMETHODIMP_(ULONG) Release() {
			long cRef = InterlockedDecrement(&_cRef);
			if (cRef == 0)
				delete this;
			return cRef;
		}


		// IFileDialogEvents methods
		IFACEMETHODIMP OnFileOk(IFileDialog* /*pfd*/) { return S_OK; };
		IFACEMETHODIMP OnFolderChange(IFileDialog* /*pfd*/) { return S_OK; };
		IFACEMETHODIMP OnFolderChanging(IFileDialog* /*pfd*/, IShellItem* /*psiFolder*/) { return S_OK; };
		IFACEMETHODIMP OnHelp(IFileDialog* /*unused*/) { return S_OK; };
		IFACEMETHODIMP OnSelectionChange(IFileDialog* /*pfd*/) { return S_OK; };
		IFACEMETHODIMP OnShareViolation(IFileDialog* /*pfd*/, IShellItem* /*psi*/, FDE_SHAREVIOLATION_RESPONSE* /*pResponse*/) { return S_OK; };
		IFACEMETHODIMP OnTypeChange(IFileDialog* /*pfd*/) { return S_OK; };
		IFACEMETHODIMP OnOverwrite(IFileDialog* /*pfd*/, IShellItem* /*psi*/, FDE_OVERWRITE_RESPONSE* /*pResponse*/) { return S_OK; };


		// IFileDialogControlEvents methods
		IFACEMETHODIMP OnItemSelected(IFileDialogCustomize* /*pfdc*/, DWORD /*dwIDCtl*/, DWORD /*dwIDItem*/) { return S_OK; };
		IFACEMETHODIMP OnButtonClicked(IFileDialogCustomize* /*pfdc*/, DWORD /*dwIDCtl*/) { return S_OK; };
		IFACEMETHODIMP OnCheckButtonToggled(IFileDialogCustomize* /*pfdc*/, DWORD /*dwIDCtl*/, BOOL /*bChecked*/) { return S_OK; };
		IFACEMETHODIMP OnControlActivating(IFileDialogCustomize* /*pfdc*/, DWORD /*dwIDCtl*/) { return S_OK; };
	};

	*ppv = nullptr;
	auto* pDialogEventHandler = new (std::nothrow) DialogEventHandler();
	HRESULT hr = pDialogEventHandler != nullptr ? S_OK : E_OUTOFMEMORY;
	if (SUCCEEDED(hr)) {
		hr = pDialogEventHandler->QueryInterface(riid, ppv);
		pDialogEventHandler->Release();
	}
	return hr;
}

static HRESULT OpenFileDialog(std::string& directory)
{
	// CoCreate the File Open Dialog object.
	IFileDialog* pfd = nullptr;
	IFileDialogEvents* pfde = nullptr;
	DWORD dwCookie;
	DWORD dwFlags;
	HRESULT hr = S_FALSE;
	if (
		SUCCEEDED(CoCreateIyattaance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd))) &&
		SUCCEEDED(CreateDialogEventHandler(IID_PPV_ARGS(&pfde))) &&
		SUCCEEDED(pfd->Advise(pfde, &dwCookie)) &&
		SUCCEEDED(pfd->GetOptions(&dwFlags)) &&
		SUCCEEDED(pfd->SetOptions(dwFlags | FOS_PICKFOLDERS | FOS_OVERWRITEPROMPT | FOS_CREATEPROMPT)) &&
		SUCCEEDED(pfd->Show(nullptr))
		)
	{
		// The result is an IShellItem object.
		IShellItem* psiResult;
		PWSTR pszFilePath = nullptr;
		if (SUCCEEDED(pfd->GetResult(&psiResult)) && SUCCEEDED(psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath))) {
			std::wstringstream ss;
			ss << pszFilePath;
			auto ws = ss.str();
			typedef std::codecvt<wchar_t, char, std::mbstate_t> converter_type;
			const std::locale locale("");
			const auto& converter = std::use_facet<converter_type>(locale);
			std::vector<char> to(ws.length() * converter.max_length());
			std::mbstate_t state;
			const wchar_t* from_next;
			char* to_next;
			const converter_type::result result = converter.out(state, ws.data(), ws.data() + ws.length(), from_next, &to[0], &to[0] + to.size(), to_next);
			if (result == converter_type::ok || result == converter_type::noconv) {
				directory = std::string(&to[0], to_next);
				hr = S_OK;
			}
			CoTaskMemFree(pszFilePath);
			psiResult->Release();
		}

		// Unhook the event handler.
		pfd->Unadvise(dwCookie);
		pfde->Release();
		pfd->Release();
	}
	return hr;
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	const auto ptr = reinterpret_cast<Directory_Screen*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	const auto controlHandle = HWND(lParam);
	if (message == WM_PAINT)
		ptr->paint();
	else if (message == WM_COMMAND) {
		const auto notification = HIWORD(wParam);
		if (notification == BN_CLICKED) {
			if (controlHandle == ptr->m_browseButton)
				ptr->browse();
			else if (controlHandle == ptr->m_btnPrev)
				ptr->goPrevious();
			else if (controlHandle == ptr->m_btnIyatta)
				ptr->goInstall();
			else if (controlHandle == ptr->m_btnCancel)
				ptr->goCancel();
		}
		else if (notification == EN_CHANGE) {
			if (controlHandle == ptr->m_directoryField) {
				// Redraw 'disk space data' region of window when the text field changes
				std::vector<char> data(GetWindowTextLength(controlHandle) + 1ULL);
				GetWindowTextA(controlHandle, &data[0], (int)data.size());
				ptr->m_Installer->setDirectory(std::string(data.data()));
				RECT rc = { 10, 200, 600, 300 };
				RedrawWindow(hWnd, &rc, nullptr, RDW_INVALIDATE);
				return S_OK;
			}
		}
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}