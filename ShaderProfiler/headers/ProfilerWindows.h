#pragma once
#include <Windows.h>
#include <unordered_map>
#include <string>

struct IDXGISwapChain;

struct Window {
	IDXGISwapChain* swapChain;
	HWND handle;
	std::wstring className;
};

class WindowManager {
public:
	std::unordered_map<std::wstring, HWND> windows;

public:
	WindowManager(HINSTANCE appInstance) : _instance(appInstance) {}
	~WindowManager();

	void viewWindow(int);
	void hideWindow();
	void updateWindows();
	HWND createMainWindow(int nShowCmd);
	HWND createOverlay();

private:
	HINSTANCE _instance;
};