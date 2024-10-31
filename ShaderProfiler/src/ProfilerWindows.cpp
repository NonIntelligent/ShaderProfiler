#include "ProfilerWindows.h"
#include <dxgi.h>
#include <dwmapi.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

LRESULT CALLBACK window_procedure(HWND window, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK overlay_procedure(HWND window, UINT msg, WPARAM wParam, LPARAM lParam);

WindowManager::~WindowManager() {
	auto it = windows.begin();

	while (it != windows.end()) {
		DestroyWindow(it->second);
		UnregisterClassW(it->first.c_str(), _instance);
		it++;
	}
}

void WindowManager::updateWindows() {
	auto it = windows.begin();

	while (it != windows.end()) {
		UpdateWindow(it->second);
		it++;
	}
}

HWND WindowManager::createMainWindow(int nShowCmd) {
	// Setup window class config
	std::wstring classname = L"Main Window Class";

	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = window_procedure;
	wc.hInstance = _instance;
	wc.lpszClassName = classname.c_str();

	RegisterClassExW(&wc);

	// Get monitor width and height
	int width = GetSystemMetrics(0);
	int height = GetSystemMetrics(1);

	// Window creation
	const HWND window = CreateWindowExW(
		WS_EX_OVERLAPPEDWINDOW,
		wc.lpszClassName,
		L"Application",
		WS_OVERLAPPEDWINDOW,
		0, 0,
		width, height,
		nullptr,
		nullptr,
		_instance,
		nullptr
	);

	// Set rendering to the entire screen
	{
		RECT clientArea{};
		GetClientRect(window, &clientArea);

		RECT windowArea{};
		GetWindowRect(window, &windowArea);

		POINT diff{};
		ClientToScreen(window, &diff);

		const MARGINS margins{
			windowArea.left + (diff.x - windowArea.left),
			windowArea.top + (diff.y - windowArea.top),
			windowArea.right,
			windowArea.bottom
		};

		DwmExtendFrameIntoClientArea(window, &margins);
	}

	windows.emplace(classname, window);

	ShowWindow(window,nShowCmd);

	return window;
}

HWND WindowManager::createOverlay() {
	// Setup overlay class
	std::wstring classname = L"Shader Overlay Class";

	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = overlay_procedure;
	wc.hInstance = _instance;
	wc.lpszClassName = classname.c_str();

	RegisterClassExW(&wc);

	// Get monitor width and height
	int monitorWidth = GetSystemMetrics(0);
	int monitorHeight = GetSystemMetrics(1);

	// Window creation
	const HWND overlay = CreateWindowExW(
		WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE,
		wc.lpszClassName,
		L"Shader Overlay",
		WS_POPUP | WS_DISABLED,
		0, 0,
		monitorWidth, monitorHeight,
		nullptr,
		nullptr,
		wc.hInstance,
		nullptr
	);

	// Set window to be opaque
	SetLayeredWindowAttributes(overlay, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);

	// Set rendering to the entire screen
	{
		RECT clientArea{};
		GetClientRect(overlay, &clientArea);

		RECT windowArea{};
		GetWindowRect(overlay, &windowArea);

		POINT diff{};
		ClientToScreen(overlay, &diff);

		const MARGINS margins{
			windowArea.left + (diff.x - windowArea.left),
			windowArea.top + (diff.y - windowArea.top),
			windowArea.right,
			windowArea.bottom
		};

		DwmExtendFrameIntoClientArea(overlay, &margins);
	}

	windows.emplace(classname, overlay);

	ShowWindow(overlay, 0); // hide window
	ShowWindow(overlay, 8); // show window but don't activate

	return overlay;
}

// Use this api to handle window messages, defined by imgui
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Handle the messages from each window
LRESULT CALLBACK window_procedure(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(window, msg, wParam, lParam)) {
		return true;
	}

	if (msg == WM_DESTROY) {
		PostQuitMessage(0);
		return true;
	}

	return DefWindowProc(window, msg, wParam, lParam);
}

// Handle the messages from each window
LRESULT CALLBACK overlay_procedure(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(window, msg, wParam, lParam)) {
		return true;
	}

	return DefWindowProc(window, msg, wParam, lParam);
}
