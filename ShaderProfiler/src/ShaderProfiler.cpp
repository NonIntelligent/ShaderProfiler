#include <iostream>
#include <Windows.h>
#include <dwmapi.h>
#include <TlHelp32.h>
#include <vector>

#include <dxgi.h>
#include <d3d11.h>

#include <DbgHelp.h>
#include <memory.h>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include "Processing.h"
#include "ProfilerWindows.h"
#include "Renderer.h"
#include <cstdlib>
#include <clocale>
#include <codecvt>

// Setup window config and create instance of window.
// Main application entry
INT APIENTRY WinMain(HINSTANCE instance, HINSTANCE, PSTR, INT cmd_show) {
	// Create windows and overlay
	WindowManager windowManager(instance);
	HWND mainWindow = windowManager.createMainWindow(cmd_show);
	HWND overlay = windowManager.createOverlay();

	Renderer renderer;

	renderer.createGraphicsContext(mainWindow);
	renderer.createGraphicsContext(overlay);

	// Application running code
	bool running = true;
	bool showOverlay = true;
	std::string current_item = "Processes";
	ProcessHandler processHandler;
	std::vector<std::string> comboItems;

	while (running) {
		// Send messages to the windows procedure
		renderer.switchTarget(mainWindow);

		MSG msg;
		while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE | PM_QS_INPUT | PM_QS_SENDMESSAGE | PM_QS_POSTMESSAGE)) {

			bool translated = TranslateMessage(&msg);
			// Send to windows procedure
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT) {
				running = false;
			}
		}

		// Quit message may have been sent, destroying the window
		if (!running) break;


		//TODO setup input handling class or input event system handler
		//TODO allow overlay to handle keyboard inputs, to fix activating another window
		if (ImGui::IsKeyReleased(ImGuiKey_LeftBracket)) {
			showOverlay = !showOverlay;
			if (!showOverlay) {
				windowManager.hideWindow(overlay);
			}
			else {
				windowManager.showWindow(overlay, 4);
			}
		}

		// App logic
		bool processFound = false;

		processHandler.findAppProcesses();
		Process selectedProcess = processHandler.getProcess(L"Example.exe");

		processFound = selectedProcess.id != 0;

		comboItems.clear();
		for (const Process& proc : processHandler.processes) {
			char temp[64];
			WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, proc.name.c_str(), -1, temp, 64, NULL, NULL);
			comboItems.emplace_back(temp);
		}

		renderer.startFrame();

		renderer.drawText({ 250, 30 }, ImColor(0.f, 0.f, 0.f), "Rendering text to main Window");

		ImGui::Begin("second window");

		if (ImGui::BeginCombo("##label", current_item.c_str())) {
			for (const std::string& item : comboItems) {
				bool selected = current_item == item;
				if (ImGui::Selectable(item.c_str(), selected)) {
					current_item = item;
				}

				if (selected) {
					ImGui::SetItemDefaultFocus();
				}

			}

			ImGui::EndCombo();
		}

		ImGui::End();

		renderer.endFrame(mainWindow);

		renderer.presentFrame(mainWindow);


		if (showOverlay) {
			renderer.drawOverlay(overlay, selectedProcess);
		}

	}

    return 0;
}
