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
#include "BinaryInjection.h"
#include <cstdlib>
#include <clocale>
#include <codecvt>
#include <cstring>

std::string toString(const std::wstring& str) {
	size_t length = WideCharToMultiByte(CP_UTF8, NULL, str.c_str(), (int) str.length(), nullptr, 0, NULL, NULL);

	std::string out(length, '\0');

	WideCharToMultiByte(CP_UTF8, NULL, str.c_str(), (int)str.length(), &out[0], (int)out.length(), NULL, NULL);
	return out;
}

std::wstring toWstring(const std::string& str) {
	size_t length = MultiByteToWideChar(CP_UTF8, NULL, str.c_str(), (int) str.length(), nullptr, 0);

	std::wstring out(length, L'\0');

	MultiByteToWideChar(CP_UTF8, NULL, str.c_str(), (int) str.length(), &out[0], (int) out.length());
	return out;
}

// Setup window config and create instance of window.
// Main application entry
INT APIENTRY WinMain(HINSTANCE instance, HINSTANCE, LPSTR cmd_line, INT cmd_show) {
#if _DEBUG // Open console when in debug mode
	AllocConsole();
	FILE* fDummy;
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
#endif // _DEBUG

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
	uint32_t funcCalls = 0;
	bool killInstrument = false;

	Profiler profile;
	char instrument_module[] = "-instrument_module";
	char instrument_target[] = "D3DSCache.dll";
	char instrument_module2[] = "-instrument_transitive";
	char instrument_target2[] = "d3d12.dll";
	char instrument_module3[] = "-instrument_transitive";
	char instrument_target3[] = "d3d11on12.dll";
	char instrument_module4[] = "-instrument_transitive";
	char instrument_target4[] = "example.exe";
	char instrument_module5[] = "-instrument_transitive";
	char instrument_target5[] = "d3d11.dll";
	char debug_events[] = "-trace_debug_events";
	char debug_entries[] = "-trace_module_entries";
	char debug_unwind[] = "-generate_unwind";

	char* line[] = {
	instrument_module, instrument_target,
	instrument_module2, instrument_target2,
	instrument_module3, instrument_target3,
	instrument_module4, instrument_target4,
	instrument_module5, instrument_target5,
	debug_events, debug_entries, debug_unwind };
	profile.Init(13, line);

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

		if (ImGui::IsKeyReleased(ImGuiKey_Semicolon)) {
			killInstrument = true;
		}

		// App logic
		bool processFound = false;

		processHandler.findAppProcesses();
		Process selectedProcess = processHandler.getProcess(toWstring(current_item));

		processFound = selectedProcess.id != 0;

		if (selectedProcess.name != L"default") {
			// Instrumentation
			// Currently crashes on second attempt to attach after hangs
			profile.startProfiling(selectedProcess.id, 1000);

			if (killInstrument && profile.isRunning()) {
				killInstrument = false;
				profile.stopProfilingAndExit();
			}
		}

		comboItems.clear();
		for (const Process& proc : processHandler.processes) {
			comboItems.emplace_back(toString(proc.name));
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
}
