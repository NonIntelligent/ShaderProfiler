#include "Renderer.h"
#include "Processes.h"
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

	CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&_factory);
}

Renderer::~Renderer() {
	for (auto& [sc, graphics] : graphicsMap) {
		graphics.swapChain->Release();
		graphics.target->Release();
		ImGui::SetCurrentContext(graphics.context);
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext(graphics.context);
	}

	if (_factory) _factory->Release();

	if (_deviceContext) _deviceContext->Release();

	if (_device) _device->Release();
}

void Renderer::createGraphicsContext(HWND window) {

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
	Graphics graphics;
	ID3D11Texture2D* backBuffer = nullptr;

	_factory->CreateSwapChain(_device, &desc, &graphics.swapChain);
	graphics.swapChain->GetBuffer(0U, IID_PPV_ARGS(&backBuffer));

	if (backBuffer) {
		_device->CreateRenderTargetView(backBuffer, nullptr, &graphics.target);
	}

	backBuffer->Release();

	graphics.context = ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui::SetCurrentContext(graphics.context);
	initContext(window);

	// Assign mappings for window rendering
	graphicsMap[window] = graphics;

}

void Renderer::switchTarget(HWND window) {
	ImGui::SetCurrentContext(graphicsMap[window].context);
	_deviceContext->OMSetRenderTargets(1U, &graphicsMap[window].target, nullptr);
}

void Renderer::startFrame() {
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void Renderer::endFrame(HWND window) {
	// End of frame, render and clear buffers 
	ImGui::Render();

	constexpr float colour[4]{ 0.f,0.f,0.f,0.f };

	_deviceContext->ClearRenderTargetView(graphicsMap[window].target, colour);

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void Renderer::presentFrame(HWND window, UINT sync, UINT flags) {
	graphicsMap[window].swapChain->Present(sync, flags);
}

void Renderer::drawOverlay(HWND overlay, const Process &app) {
	switchTarget(overlay);
	startFrame();

	const ImGuiIO& io = ImGui::GetIO();

	drawCircle({ 750, 500 }, 16.f, ImColor(1.f, 0.5f, 0.2f));
	drawText({ 750, 600 }, ImColor(0.f, 1.f, 0.f), std::to_string(Process::created).append(" creations"));
	drawText({ 750, 650 }, ImColor(1.f, 0.5f, 0.f), std::to_string(Process::copied).append(" copies"));
	drawText({ 750, 700 }, ImColor(1.f, 0.f, 0.f), std::to_string(Process::deleted).append(" deletions"));
	drawText({ 750, 750 }, ImColor(0.5f, 0.5f, 0.8f), std::to_string(1.0f / io.DeltaTime).append(" fps"));
	RECT rect;
	GetWindowRect(overlay, &rect);
	ImGui::GetBackgroundDrawList()->AddRect({ 0,0 }, { (float)rect.right - rect.left, (float)rect.bottom - rect.top }, ImColor(0.5f, 0.f, 0.5f),0.f,0,4.f);

	Process::created = 0;
	Process::copied = 0;
	Process::deleted = 0;

	bool processFound = app.id != 0;

	if (processFound) {
		drawText({ 750, 550 }, ImColor(0.f, 1.f, 0.f), "Process Found!");
	} else {
		drawText({ 750, 550 }, ImColor(1.f, 0.f, 0.f), "Womp womp woomp!");
	}

	if (processFound && app.isAlive()) {
		drawText({ 800, 575 }, ImColor(0.f, 1.f, 0.f), "I AM ALIVE");
	} else {
		drawText({ 800, 575 }, ImColor(1.f, 0.f, 0.f), "Dead is I");
	}

	endFrame(overlay);
	presentFrame(overlay);
}

void Renderer::drawEmptyFrame(HWND window) {
	switchTarget(window);
	startFrame();
	endFrame(window);
	presentFrame(window);
}

void Renderer::drawText(ImVec2 pos, ImU32 colour, const std::string &text) {
	ImGui::GetBackgroundDrawList()->AddText(pos, colour, text.c_str());

}

void Renderer::drawCircle(ImVec2 pos, float radius, ImU32 colour) {
	ImGui::GetBackgroundDrawList()->AddCircleFilled(pos, radius, colour);
}

void Renderer::setImGuiContext(HWND window) {
	ImGui::SetCurrentContext(graphicsMap[window].context);
}

void Renderer::initContext(HWND hwnd) {
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(_device, _deviceContext);
}