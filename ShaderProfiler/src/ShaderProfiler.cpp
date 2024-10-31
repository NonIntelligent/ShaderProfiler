#include <iostream>
#include <Windows.h>
#include <dwmapi.h>
#include <TlHelp32.h>
#include <vector>

#include <dxgi.h>
#include <d3d11.h>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include "Processing.h"
#include "ProfilerWindows.h"
#include "Renderer.h"

// Setup window config and create instance of window.
// Main application entry
INT APIENTRY WinMain(HINSTANCE instance, HINSTANCE, PSTR, INT cmd_show) {
	// Create windows and overlay
	WindowManager windowManager(instance);
	HWND mainWindow = windowManager.createMainWindow(cmd_show);
	HWND overlay = windowManager.createOverlay();

	Renderer renderer;
	renderer.initPlatform(mainWindow);

	renderer.createSwapChainAndTarget(mainWindow);
	renderer.createSwapChainAndTarget(overlay);

	// Application running code
	bool running = true;
	bool iskeypressed = true;
	ProcessHandler processHandler;

	while (running) {

		// Send messages to the windows procedure
		MSG msg;
		while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT) {
				running = false;
			}
		}

		windowManager.updateWindows();

		// App logic
		bool processFound = false;

		processHandler.findAppProcesses();
		Process selectedProcess = processHandler.getProcess(L"Example.exe");

		processFound = selectedProcess.id != 0;

		renderer.startFrame();

		renderer.drawCircle({ 750, 500 }, 16.f, ImColor(1.f, 0.5f, 0.2f));
		renderer.drawText({ 750, 600 }, ImColor(1.f, 1.f, 1.f), std::to_string(selectedProcess.created).append(" creations"));
		renderer.drawText({ 750, 650 }, ImColor(1.f, 1.f, 1.f), std::to_string(selectedProcess.copied).append(" copies"));
		renderer.drawText({ 750, 700 }, ImColor(1.f, 1.f, 1.f), std::to_string(selectedProcess.deleted).append(" deletions"));
		ImGuiWindowFlags flags = 0;
		flags |= ImGuiWindowFlags_NoBackground;
		flags |= ImGuiWindowFlags_NoTitleBar;
		flags |= ImGuiWindowFlags_NoDecoration;
		flags |= ImGuiWindowFlags_NoMouseInputs;

		ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID,ImGui::GetMainViewport(), dockspace_flags);

		if (ImGui::IsKeyPressed(ImGuiKey_LeftBracket)) {
			iskeypressed = !iskeypressed;
		}

		if (iskeypressed) {
			float x = GetSystemMetrics(0);
			float y = GetSystemMetrics(1);
			ImGui::SetNextWindowSize({ x, y});
			ImGui::SetNextWindowPos({ 0, 0});

			if (ImGui::Begin("menu", &iskeypressed, flags)) {
				ImGui::TextColored(ImColor(1.f, 1.f, 0.f), "Rendering text to main Window");
			}

			ImGui::End();
		}

		renderer.drawText({ 250, 250 }, ImColor(0.f, 0.f, 0.f), "Rendering text to main Window");

		selectedProcess.created = 0;
		selectedProcess.copied = 0;
		selectedProcess.deleted = 0;

		if (processFound) {
			renderer.drawText({ 750, 550 }, ImColor(0.f, 1.f, 0.f), "Process Found!");
		} else {
			renderer.drawText({ 750, 550 }, ImColor(1.f, 0.f, 0.f), "Womp womp woomp!");
		}

		if (selectedProcess.id != 0 && selectedProcess.isAlive()) {
			renderer.drawText({ 800, 575 }, ImColor(0.f, 1.f, 0.f), "I AM ALIVE");
		} else {
			renderer.drawText({ 800, 575 }, ImColor(1.f, 0.f, 0.f), "Dead is I");
		}

		renderer.endFrame();

		renderer.presentFrame();
	}

    return 0;
}
