#pragma once
#include <d3d12.h>
#include "d3dx12.h"
#include <DirectXTex.h>
#include <wrl.h>
#include <map>

#pragma comment(lib,"DirectXTex.lib")

using namespace Microsoft::WRL;

class TextureManager
{
private:
	std::map<std::string, ComPtr<ID3D12Resource>> toonResTable;
public:
	TextureManager();
	~TextureManager();

	ID3D12Resource* CreateWhiteTex(ComPtr<ID3D12Device>& dev);
	ID3D12Resource* CreateBlackTex(ComPtr<ID3D12Device>& dev);
	ID3D12Resource* CreateGrayGradationTexture(ComPtr<ID3D12Device>& dev);
	std::wstring GetWstringFromString(const std::string& str);
	std::string GetStringFromWstring(const std::wstring& str);
	ID3D12Resource* LoadTextureFromFile(ComPtr<ID3D12Device>& dev, const std::string& texpath, const std::string& ext = "png");
};

