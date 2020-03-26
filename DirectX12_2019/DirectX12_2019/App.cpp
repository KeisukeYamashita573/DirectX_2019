#include "App.h"
#include <iostream>
#include "imgui-master/imgui.h"
#include "imgui-master/examples/imgui_impl_dx12.h"
#include "imgui-master/examples/imgui_impl_win32.h"

#pragma comment(lib,"dxguid.lib")

constexpr int WindowWidth = 1280;
constexpr int WindowHeight = 720;

void App::CreateGraphicsMemory(ComPtr<ID3D12Device> device)
{
	gMemory = new DirectX::GraphicsMemory(device.Get());
}

void App::CreateSpriteBatch(ComPtr<ID3D12Device> device, ComPtr<ID3D12CommandQueue> cmdque)
{
	DirectX::ResourceUploadBatch resUploadBatch(device.Get());
	resUploadBatch.Begin();
	DirectX::RenderTargetState rtState(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT);
	DirectX::SpriteBatchPipelineStateDescription pd(rtState);
	spriteBatch = new DirectX::SpriteBatch(device.Get(), resUploadBatch, pd);

	heapForSpriteFont = initer.CreateDescriptorHeapForSpriteFont().Get();
	spriteFont = new DirectX::SpriteFont(device.Get(), resUploadBatch, L"font/meiryo.spritefont",
		heapForSpriteFont->GetCPUDescriptorHandleForHeapStart(), heapForSpriteFont->GetGPUDescriptorHandleForHeapStart());
	D3D12_VIEWPORT port = {};
	port.TopLeftX = 0;
	port.TopLeftY = 0;
	port.Width = WindowWidth;
	port.Height = WindowHeight;
	port.MinDepth = 0.0f;
	port.MaxDepth = 1.0f;
	auto future = resUploadBatch.End(cmdque.Get());
	initer.Wait();
	future.wait();
	spriteBatch->SetViewport(port);
}

App::App()
{
	wInfo.width = WindowWidth;
	wInfo.height = WindowHeight;
}

LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

LRESULT WidnowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
		return TRUE;
	}
	if (msg == WM_DESTROY) {
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void App::InitializeWindow() {
	wnd.cbSize = sizeof(WNDCLASSEX);
	wnd.lpfnWndProc = (WNDPROC)WidnowProcedure;
	wnd.lpszClassName = "1701386_山下圭介";
	wnd.hInstance = GetModuleHandle(nullptr);
	RegisterClassEx(&wnd);

	RECT wrc = { 0,0,
	WindowWidth,WindowHeight };
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	hwnd = CreateWindow(wnd.lpszClassName,
		"1701386_山下圭介", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		wrc.right - wrc.left,
		wrc.bottom - wrc.top,
		nullptr, nullptr,
		wnd.hInstance,
		nullptr);
	
	if (hwnd == nullptr) {
		LPVOID mssageBuff = nullptr;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&mssageBuff, 0, nullptr);
		OutputDebugString((TCHAR*)mssageBuff);
		std::cout << (TCHAR*)mssageBuff << std::endl;
		LocalFree(mssageBuff);
	}
}

void App::Initialize()
{
	InitializeWindow();
	initer.Initialize();
	CreateGraphicsMemory(initer.GetDevice());
	CreateSpriteBatch(initer.GetDevice(),initer.GetCmdQue());
}

void App::Run()
{
	// ウィンドウ表示
	ShowWindow(hwnd, SW_SHOW);
	MSG msg = {};
	// メインループ
	while (true) {
		// peekMessageはメッセージの読み込み
		// 読み込んだメッセージをmsg構造体に入れる
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);	// 必要な情報を全て取ってくる
			DispatchMessage(&msg);	// 自分の子のウィンドウにメッセージを飛ばす
		}
		// 終了メッセージが来た場合の処理
		if (msg.message == WM_QUIT) {
			break;
		}
		initer.Runnable();
	}
}

void App::Terminate()
{
	UnregisterClass(wnd.lpszClassName, wnd.hInstance);
}

App::~App()
{
	if (gMemory != nullptr) {
		free(gMemory);
	}
	if (spriteFont != nullptr) {
		free(spriteFont);
	}
	if (spriteBatch != nullptr) {
		free(spriteBatch);
	}
	if (heapForSpriteFont != nullptr) {
		heapForSpriteFont->Release();
	}
}
