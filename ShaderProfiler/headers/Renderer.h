#pragma once
#include <Windows.h>
#include <string>
#include <unordered_map>

#include "Graphics.h"

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11RenderTargetView;
struct IDXGISwapChain;
struct IDXGIFactory;
enum D3D_FEATURE_LEVEL;
using ImU32 = unsigned int;
struct ImVec2;

struct Process;

class Renderer {
public:
	Renderer();
	~Renderer();
public:
	void createGraphicsContext(HWND window);
	void switchTarget(HWND window);
	void startFrame();
	void endFrame(HWND window);
	void presentFrame(HWND window, UINT sync = 0U, UINT flags = 0U);

	void drawOverlay(HWND overlay, const Process &app);

	void drawEmptyFrame(HWND window);
	void clearTarget(HWND window);

	void drawGraph(ImVec2 pos);
	void drawText(ImVec2 pos, ImU32 colour, const std::string &text);
	void drawCircle(ImVec2 pos, float radius, ImU32 colour);

	void setImGuiContext(HWND window);
private:
	ID3D11Device* _device = nullptr;
	ID3D11DeviceContext* _deviceContext = nullptr;
	D3D_FEATURE_LEVEL _level = {};
	IDXGIFactory* _factory = nullptr;

	std::unordered_map<HWND, Graphics> graphicsMap;
private:
	void initContext(HWND hwnd);
};

