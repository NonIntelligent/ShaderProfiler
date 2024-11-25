#pragma once
#include <Windows.h>
#include <unordered_map>
#include <string>

struct IDXGISwapChain;

struct Window {
	HWND handle;
	std::wstring className;
	int width, height;
};

class WindowManager {
public:
	std::unordered_map<std::wstring, HWND> windows;

public:
	WindowManager(HINSTANCE appInstance) : _instance(appInstance) {}
	~WindowManager();

	void showWindow(HWND window, int nShowCmd);
	void hideWindow(HWND window);
	HWND createMainWindow(int nShowCmd);
	HWND createOverlay();

private:
	HINSTANCE _instance;
};