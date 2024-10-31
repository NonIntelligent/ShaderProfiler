#pragma once
#include <Windows.h>
#include <string>
#include <unordered_map>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11RenderTargetView;
struct IDXGISwapChain;
struct IDXGIFactory;
enum D3D_FEATURE_LEVEL;
using ImU32 = unsigned int;
struct ImVec2;

class Renderer {
public:
	Renderer();
	~Renderer();
public:
	void createSwapChainAndTarget(HWND window);
	void switchTarget(HWND window);
	void startFrame();
	void endFrame();
	void presentFrame();

	void initPlatform(HWND mainWindow);

	void drawGraph(ImVec2 pos);
	void drawText(ImVec2 pos, ImU32 colour, const std::string &text);
	void drawCircle(ImVec2 pos, float radius, ImU32 colour);
private:
	ID3D11Device* _device = nullptr;
	ID3D11DeviceContext* _deviceContext = nullptr;
	D3D_FEATURE_LEVEL _level = {};
	IDXGIFactory* _factory = nullptr;

	std::unordered_map<HWND, IDXGISwapChain*> windowMap;
	std::unordered_map<IDXGISwapChain*, ID3D11RenderTargetView*> renderMaps;
private:
};

