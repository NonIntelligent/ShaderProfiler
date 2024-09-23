#include <iostream>
#include <Windows.h>
#include <dwmapi.h>
#include <dxgi.h>
#include <d3d11.h>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

// Use this api to handle window messages, defined by imgui
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void ShutdownApp();
void ReleaseD3D11(IDXGISwapChain* swap, ID3D11DeviceContext* context, ID3D11Device* device, ID3D11RenderTargetView* target);

// Handle the messages from the window
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


// Setup window config and create intance of window.
INT APIENTRY WinMain(HINSTANCE instance, HINSTANCE, PSTR, INT cmd_show) {
	// Setup window class config
	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = window_procedure;
	wc.hInstance = instance;
	wc.lpszClassName = L"Shader Overlay Class";

	RegisterClassExW(&wc);

	// Get monitor width and height
	int monitorWidth = GetSystemMetrics(0);
	int monitorHeight = GetSystemMetrics(1);

	// Window creation
	const HWND window = CreateWindowExW(
		WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
		wc.lpszClassName,
		L"Shader Overlay",
		WS_POPUP,
		0, 0,
		monitorWidth, monitorHeight,
		nullptr,
		nullptr,
		wc.hInstance,
		nullptr
	);

	// Set window to be opaque (LWA_ALPHA tells the window to use the 3rd parameter value)
	SetLayeredWindowAttributes(window, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);

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

	// Swap chain config
	DXGI_SWAP_CHAIN_DESC sc{};
	sc.BufferDesc.RefreshRate.Numerator = 60U;
	sc.BufferDesc.RefreshRate.Denominator = 1U;
	sc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sc.SampleDesc.Count = 1U;
	sc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sc.BufferCount = 2U;
	sc.OutputWindow = window;
	sc.Windowed = TRUE;
	sc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// Features supported
	constexpr D3D_FEATURE_LEVEL levels[2]{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_0
	};

	ID3D11Device* device(nullptr);
	ID3D11DeviceContext* deviceContext{ nullptr };
	IDXGISwapChain* swapChain{ nullptr };
	ID3D11RenderTargetView* renderTargetView{ nullptr };
	D3D_FEATURE_LEVEL level{};

	D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0U,
		levels,
		2U,
		D3D11_SDK_VERSION,
		&sc,
		&swapChain,
		&device,
		&level,
		&deviceContext
	);

	ID3D11Texture2D* backBuffer{ nullptr };
	swapChain->GetBuffer(0U, IID_PPV_ARGS(&backBuffer));

	device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
	backBuffer->Release();

	ShowWindow(window, cmd_show);
	UpdateWindow(window);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(device, deviceContext);

	// Application running code
	bool running = true;

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
		// Program Loop
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();

		ImGui::NewFrame();

		ImGui::GetBackgroundDrawList()->AddCircleFilled({ 750,500 }, 16.f, ImColor(1.f, 0.5f, 0.2f));

		// Rendering
		ImGui::Render();

		constexpr float colour[4]{ 0.f,0.f,0.f,0.f };
		deviceContext->OMSetRenderTargets(1U, &renderTargetView, nullptr);
		deviceContext->ClearRenderTargetView(renderTargetView, colour);

		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		// Present frames without syncing
		swapChain->Present(0U, 0U);
	}

	ShutdownApp();
	ReleaseD3D11(swapChain, deviceContext, device, renderTargetView);
	DestroyWindow(window);
	UnregisterClassW(wc.lpszClassName, instance);
    return 0;
}

void ShutdownApp() {
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void ReleaseD3D11(IDXGISwapChain* swap, ID3D11DeviceContext* context, ID3D11Device* device, ID3D11RenderTargetView* target) {
	if (swap) swap->Release();

	if (context) context->Release();

	if (device) device->Release();

	if (target) target->Release();
}
