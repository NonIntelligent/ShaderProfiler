#pragma once

struct ID3D11RenderTargetView;
struct IDXGISwapChain;
struct ImGuiContext;

struct Graphics {
	IDXGISwapChain* swapChain = nullptr;
	ID3D11RenderTargetView* target = nullptr;
	ImGuiContext* context = nullptr;
};