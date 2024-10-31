#include "Renderer.h"
#include <dxgi.h>
#include <d3d11.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

// Features supported
constexpr D3D_FEATURE_LEVEL levels[2]{
	D3D_FEATURE_LEVEL_11_0,
	D3D_FEATURE_LEVEL_10_0
};

Renderer::Renderer() {
	D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0U,
		levels,
		2U,
		D3D11_SDK_VERSION,
		&_device,
		&_level,
		&_deviceContext
	);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	ImGui::StyleColorsDark();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	_factory = nullptr;

	CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&_factory);

	ImGui_ImplDX11_Init(_device, _deviceContext);
}

Renderer::~Renderer() {
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	for (auto& [sc, target] : renderMaps) {
		sc->Release();
		target->Release();
	}

	if (_deviceContext) _deviceContext->Release();

	if (_device) _device->Release();

	_factory->Release();
}

void Renderer::createSwapChainAndTarget(HWND window) {
	// TODO Generate error exception
	if (!_device) {
		return;
	}

	// Swap chain config and render context for the window
	DXGI_SWAP_CHAIN_DESC desc{};
	desc.BufferDesc.RefreshRate.Numerator = 0U;
	desc.BufferDesc.RefreshRate.Denominator = 0U;
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1U;
	desc.SampleDesc.Quality = 0U;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = 1U;
	desc.OutputWindow = window;
	desc.Windowed = TRUE;
	desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	desc.Flags = 0U;

	// Create buffers and targets for the window
	IDXGISwapChain* swapChain = nullptr;
	ID3D11RenderTargetView* target = nullptr;
	ID3D11Texture2D* backBuffer = nullptr;

	_factory->CreateSwapChain(_device, &desc, &swapChain);
	swapChain->GetBuffer(0U, IID_PPV_ARGS(&backBuffer));
	_device->CreateRenderTargetView(backBuffer, nullptr, &target);

	backBuffer->Release();

	// Assign mappings for window rendering
	windowMap[window] = swapChain;
	renderMaps[swapChain] = target;
}

void Renderer::switchTarget(HWND window) {
	_deviceContext->OMSetRenderTargets(1U, &renderMaps[windowMap[window]], nullptr);
}

void Renderer::startFrame() {
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void Renderer::endFrame() {
	// End of frame, render and clear buffers 
	ImGui::Render();

	constexpr float colour[4]{ 0.f,0.f,0.f,0.f };

	for (auto const &it: renderMaps) {
		_deviceContext->OMSetRenderTargets(1U, &it.second, nullptr);
		_deviceContext->ClearRenderTargetView(it.second, colour);
	}

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	ImGuiIO& io = ImGui::GetIO();

	// Update and render additional platform windows
	// TODO error check to see if window exists
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

void Renderer::presentFrame() {
	for (auto const& it : renderMaps) {
		it.first->Present(0U, 0U);
	}
}

void Renderer::initPlatform(HWND mainWindow) {
	ImGui_ImplWin32_Init(mainWindow);
}

void Renderer::drawText(ImVec2 pos, ImU32 colour, const std::string &text) {
	ImGui::GetBackgroundDrawList()->AddText(pos, colour, text.c_str());

}

void Renderer::drawCircle(ImVec2 pos, float radius, ImU32 colour) {
	ImGui::GetBackgroundDrawList()->AddCircleFilled(pos, radius, colour);
}
