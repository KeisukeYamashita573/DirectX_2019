#pragma once
#include <windows.h>
#include "Dx12Initer.h"
#include <SpriteFont.h>
#include <spritebatch.h>
#include <ResourceUploadBatch.h>
#include <memory>

struct WindowInfo {
	int width;
	int height;
}; 

class App
{
public:
	
	static App& Instance() {
		static App instance;
		return instance;
	}
	~App();

	void InitializeWindow();
	void Initialize();
	void Run();
	void Terminate();
	HWND GetHwnd() {
		return hwnd;
	};

	WindowInfo GetWidnowInfo() {
		return wInfo;
	}

	// フォント関係
	void CreateGraphicsMemory(ComPtr<ID3D12Device> device);
	void CreateSpriteBatch(ComPtr<ID3D12Device> device, ComPtr<ID3D12CommandQueue> cmdque);
	DirectX::GraphicsMemory* GetGraphicsMemory() {
		return gMemory;
	}
	DirectX::SpriteFont* GetSpriteFont() {
		return spriteFont;
	}

	DirectX::SpriteBatch* GetSpriteBatch() {
		return spriteBatch;
	}

	ID3D12DescriptorHeap* GetSpriteFontHeap() {
		return heapForSpriteFont;
	}
private:
	App();
	App(const App&) {};
	void operator=(const App&) {};

	HWND hwnd;
	WNDCLASSEX wnd = {};
	Dx12Initer initer;
	WindowInfo wInfo;

	// フォント関係
	DirectX::GraphicsMemory* gMemory = nullptr;
	DirectX::SpriteFont* spriteFont = nullptr;
	DirectX::SpriteBatch* spriteBatch = nullptr;
	ID3D12DescriptorHeap* heapForSpriteFont = nullptr;
};

