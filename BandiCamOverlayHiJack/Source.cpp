#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <vector>
#include <Dwmapi.h>
 
#pragma comment(lib, "Dwmapi.lib")
 
#define HJWND_PROGRAM L"bdcam.exe"
#define MAX_CLASSNAME 255
#define MAX_WNDNAME 255
#define TRANSPARENCY_COLOR RGB(0, 254, 0)
 
using namespace std;

// Structs
struct WindowsFinderParams {
	DWORD pidOwner = NULL;
	wstring wndClassName = L"";
	wstring wndName = L"";
	RECT pos = { 0, 0, 0, 0 };
	POINT res = { 0, 0 };
	float percentAllScreens = 0.0f;
	float percentMainScreen = 0.0f;
	DWORD style = NULL;
	DWORD styleEx = NULL;
	bool satisfyAllCriteria = false;
	vector<HWND> hwnds;
};
 
// Prototypes
vector<HWND> WindowsFinder(WindowsFinderParams params);
BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam);
HWND HiJackNotepadWindow();
vector<DWORD> GetPIDs(wstring targetProcessName);
 
int main() {
	HWND hwnd = HiJackNotepadWindow();
	if (!hwnd) {
		cout << "Window HiJacking failed (use debugger to investigate why)" << endl;
		return EXIT_FAILURE;
	}
	FreeConsole(); // We won't be needing that anymore
 
	HDC hdc = GetDC(hwnd);
 
	// Getting settings of back buffer bitmap
	DEVMODE devMode;
	devMode.dmSize = sizeof(devMode);
	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);
	BITMAPINFO backBufferBmpInfo;
	backBufferBmpInfo = { 0 };
	backBufferBmpInfo.bmiHeader.biBitCount = devMode.dmBitsPerPel;
	backBufferBmpInfo.bmiHeader.biHeight = GetSystemMetrics(SM_CYSCREEN);
	backBufferBmpInfo.bmiHeader.biWidth = GetSystemMetrics(SM_CXSCREEN);
	backBufferBmpInfo.bmiHeader.biPlanes = 1;
	backBufferBmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
 
	void* backBufferPixels = nullptr;
	POINT res = { GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
	HBRUSH bgTransparencyColor = CreateSolidBrush(TRANSPARENCY_COLOR);
	HFONT hTitleFont = CreateFontA(50, 0, 0, 0, FW_DONTCARE, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Consolas");
	HBRUSH hbrRed = CreateSolidBrush(RGB(255, 0, 0));
	HPEN hPen = CreatePen(PS_NULL, 0, RGB(0, 0, 0));
 
	int i = -1;
 
	while (true) {
		if (!IsWindow(hwnd)) // We lost the hijacked window, exiting
			exit(EXIT_SUCCESS);
 
		i = (i > res.x) ? 0 : ++i; // To simulate movement
 
		// Frame preparation
		HDC hdcBackBuffer = CreateCompatibleDC(hdc); // Create back buffer
		SetBkMode(hdcBackBuffer, TRANSPARENT); // Text background transparency
		HBITMAP hbmBackBuffer = CreateDIBSection(hdcBackBuffer, (BITMAPINFO *)&backBufferBmpInfo, DIB_RGB_COLORS, (void **)&backBufferPixels, NULL, 0); // Create back buffer bitmap
		DeleteObject(SelectObject(hdcBackBuffer, hbmBackBuffer));
		DeleteObject(SelectObject(hdcBackBuffer, bgTransparencyColor)); // To set background in transparent color key
		DeleteObject(SelectObject(hdcBackBuffer, hPen));
		Rectangle(hdcBackBuffer, 0, 0, res.x, res.y); // Making transparency
 
		// Draw calls
		wstring text = L"Enemy\n[123m]";
		RECT textPos;
 
		SelectObject(hdcBackBuffer, hbrRed);
		Rectangle(hdcBackBuffer, i - 40, res.y / 2 - 180, i + 40, res.y / 2 - 4);
		SelectObject(hdcBackBuffer, bgTransparencyColor);
		Rectangle(hdcBackBuffer, i - 39, res.y / 2 - 179, i + 39, res.y / 2 - 5);
 
		textPos.left = i;
		textPos.right = i;
		textPos.top = res.y / 2;
		textPos.bottom = res.y / 2;
		SetTextColor(hdcBackBuffer, RGB(255, 0, 0));
		DrawTextExW(hdcBackBuffer, (wchar_t*)text.c_str(), text.size(), &textPos, DT_CENTER | DT_NOCLIP | DT_NOPREFIX, NULL);
 
		text = L"Overlay & Window HiJacking Demo";
		textPos.left = res.x / 2;
		textPos.right = res.x / 2;
		textPos.top = 100;
		textPos.bottom = 100;
		SetTextColor(hdcBackBuffer, RGB(0, 200, 0));
		DeleteObject(SelectObject(hdcBackBuffer, hTitleFont));
		DrawTextExW(hdcBackBuffer, (wchar_t*)text.c_str(), text.size(), &textPos, DT_CENTER | DT_NOCLIP | DT_NOPREFIX, NULL);
 
		// Frame presentation
		BitBlt(hdc, 0, 0, res.x, res.y, hdcBackBuffer, 0, 0, SRCCOPY);
		
		// Cleanup
		DeleteDC(hdcBackBuffer); // Delete back buffer device context
		DeleteObject(hbmBackBuffer); // Delete back buffer bitmap
		backBufferPixels = nullptr;
	}
 
    return EXIT_SUCCESS;
}
 
HWND HiJackNotepadWindow() {
	vector<HWND> hwnds;
	HWND hwnd = NULL;

	vector<DWORD> notepads = GetPIDs(HJWND_PROGRAM);
	printf("HIJACKING WINDOW... WAIT");

	WindowsFinderParams params;
	params.wndClassName = L"FULLSCREEN";
	params.style = WS_VISIBLE;
	hwnds = WindowsFinder(params);
	hwnd = hwnds[0];
		
	// Making the window usable for overlay puposes
	SetMenu(hwnd, NULL);

	SetWindowLongPtr(hwnd, GWL_STYLE, WS_VISIBLE);
	SetWindowLongPtr(hwnd, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TRANSPARENT| WS_EX_TOPMOST); // WS_EX_NOACTIVATE  and WS_EX_TOOLWINDOW removes it from taskbar
	SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_SHOWWINDOW);

	SetLayeredWindowAttributes(hwnd, TRANSPARENCY_COLOR, 0, LWA_COLORKEY);
	return hwnd;
}
 
vector<DWORD> GetPIDs(wstring targetProcessName) {
	vector<DWORD> pids;
	if (targetProcessName == L"")
		return pids;
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32W entry;
	entry.dwSize = sizeof entry;
	if (!Process32FirstW(snap, &entry)) {
		CloseHandle(snap);
		return pids;
	}
	do {
		if (wstring(entry.szExeFile) == targetProcessName) {
			pids.emplace_back(entry.th32ProcessID);
		}
	} while (Process32NextW(snap, &entry));
	CloseHandle(snap);
	return pids;
}
 
BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam) {
	WindowsFinderParams& params = *(WindowsFinderParams*)lParam;
 
	unsigned char satisfiedCriteria = 0, unSatisfiedCriteria = 0;
 
	// If looking for windows of a specific PDI
	DWORD pid = 0;
	GetWindowThreadProcessId(hwnd, &pid);
	if (params.pidOwner != NULL)
		if (params.pidOwner == pid)
			++satisfiedCriteria; // Doesn't belong to the process targeted
		else
			++unSatisfiedCriteria;
 
	// If looking for windows of a specific class
	wchar_t className[MAX_CLASSNAME] = L"";
	GetClassName(hwnd, className, MAX_CLASSNAME);
	wstring classNameWstr = className;
	if (params.wndClassName != L"")
		if (params.wndClassName == classNameWstr)
			++satisfiedCriteria; // Not the class targeted
		else
			++unSatisfiedCriteria;
 
	// If looking for windows with a specific name
	wchar_t windowName[MAX_WNDNAME] = L"";
	GetWindowText(hwnd, windowName, MAX_CLASSNAME);
	wstring windowNameWstr = windowName;
	if (params.wndName != L"")
		if (params.wndName == windowNameWstr)
			++satisfiedCriteria; // Not the class targeted
		else
			++unSatisfiedCriteria;
 
	// If looking for window at a specific position
	RECT pos;
	GetWindowRect(hwnd, &pos);
	if (params.pos.left || params.pos.top || params.pos.right || params.pos.bottom)
		if (params.pos.left == pos.left && params.pos.top == pos.top && params.pos.right == pos.right && params.pos.bottom == pos.bottom)
			++satisfiedCriteria;
		else
			++unSatisfiedCriteria;
 
	// If looking for window of a specific size
	POINT res = { pos.right - pos.left, pos.bottom - pos.top };
	if (params.res.x || params.res.y)
		if (res.x == params.res.x && res.y == params.res.y)
			++satisfiedCriteria;
		else
			++unSatisfiedCriteria;
 
	// If looking for windows taking more than a specific percentage of all the screens
	float ratioAllScreensX = res.x / GetSystemMetrics(SM_CXSCREEN);
	float ratioAllScreensY = res.y / GetSystemMetrics(SM_CYSCREEN);
	float percentAllScreens = ratioAllScreensX * ratioAllScreensY * 100;
	if (params.percentAllScreens != 0.0f)
		if (percentAllScreens >= params.percentAllScreens)
			++satisfiedCriteria;
		else
			++unSatisfiedCriteria;
 
	// If looking for windows taking more than a specific percentage or the main screen
	RECT desktopRect;
	GetWindowRect(GetDesktopWindow(), &desktopRect);
	POINT desktopRes = { desktopRect.right - desktopRect.left, desktopRect.bottom - desktopRect.top };
	float ratioMainScreenX = res.x / desktopRes.x;
	float ratioMainScreenY = res.y / desktopRes.y;
	float percentMainScreen = ratioMainScreenX * ratioMainScreenY * 100;
	if (params.percentMainScreen != 0.0f)
		if (percentAllScreens >= params.percentMainScreen)
			++satisfiedCriteria;
		else
			++unSatisfiedCriteria;
 
	// Looking for windows with specific styles
	LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
	if (params.style)
		if (params.style & style)
			++satisfiedCriteria;
		else
			++unSatisfiedCriteria;
 
	// Looking for windows with specific extended styles
	LONG_PTR styleEx = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
	if (params.styleEx)
		if (params.styleEx & styleEx)
			++satisfiedCriteria;
		else
			++unSatisfiedCriteria;
 
	if (!satisfiedCriteria)
		return TRUE;
 
	if (params.satisfyAllCriteria && unSatisfiedCriteria)
		return TRUE;
 
	// If looking for multiple windows
	params.hwnds.push_back(hwnd);
	return TRUE;
}
 
vector<HWND> WindowsFinder(WindowsFinderParams params) {
	EnumWindows(EnumWindowsCallback, (LPARAM)&params);
	return params.hwnds;
}