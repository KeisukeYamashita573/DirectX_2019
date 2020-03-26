#include "TextureManager.h"
#include <string>

TextureManager::TextureManager()
{
}

TextureManager::~TextureManager()
{
}

ID3D12Resource* TextureManager::CreateWhiteTex(ComPtr<ID3D12Device>& dev)
{
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	texHeapProp.CreationNodeMask = 0;
	texHeapProp.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Width = 4;
	resDesc.Height = 4;
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.MipLevels = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ID3D12Resource* whiteBuff = nullptr;
	auto result = dev->CreateCommittedResource(
		&texHeapProp, D3D12_HEAP_FLAG_NONE,
		&resDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr, IID_PPV_ARGS(&whiteBuff));
	if (FAILED(result)) {
		return nullptr;
	}

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0xff);
	result = whiteBuff->WriteToSubresource(0, nullptr, data.data(), 4 * 4, data.size());
	return whiteBuff;
}

ID3D12Resource* TextureManager::CreateBlackTex(ComPtr<ID3D12Device>& dev)
{
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	texHeapProp.CreationNodeMask = 0;
	texHeapProp.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Width = 4;
	resDesc.Height = 4;
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.MipLevels = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ID3D12Resource* blackBuff = nullptr;
	auto result = dev->CreateCommittedResource(
		&texHeapProp, D3D12_HEAP_FLAG_NONE,
		&resDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr, IID_PPV_ARGS(&blackBuff));
	if (FAILED(result)) {
		return nullptr;
	}

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0x00);
	result = blackBuff->WriteToSubresource(0, nullptr, data.data(), 4 * 4, data.size());
	return blackBuff;
}

ID3D12Resource* TextureManager::CreateGrayGradationTexture(ComPtr<ID3D12Device>& dev)
{
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	texHeapProp.CreationNodeMask = 0;
	texHeapProp.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Width = 4;
	resDesc.Height = 256;
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.MipLevels = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ID3D12Resource* gradBuff = nullptr;
	auto result = dev->CreateCommittedResource(
		&texHeapProp, D3D12_HEAP_FLAG_NONE,
		&resDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr, IID_PPV_ARGS(&gradBuff));
	if (FAILED(result)) {
		return nullptr;
	}

	std::vector<unsigned int> data(4 * 256);
	auto it = data.begin();
	unsigned int c = 0xff;
	for (; it != data.end(); it += 4) {
		auto col = (c << 0xff) | (c << 16) | (c << 8) | c;
		std::fill(it, it + 4, col);
		--c;
	}
	result = gradBuff->WriteToSubresource(0, nullptr, data.data(), 4 * sizeof(unsigned int), sizeof(unsigned int) * data.size());
	return gradBuff;
}

ID3D12Resource* TextureManager::LoadTextureFromFile(ComPtr<ID3D12Device>& dev, const std::string& texpath, const std::string& ext)
{
	DirectX::TexMetadata metaData = {};
	DirectX::ScratchImage scratchImage = {};

	auto result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	auto it = toonResTable.find(texpath);
	if (it != toonResTable.end()) {
		return it->second.Get();
	}

	result = DirectX::LoadFromWICFile(GetWstringFromString(texpath).c_str(),
		DirectX::WIC_FLAGS_NONE, &metaData, scratchImage);
	if (FAILED(result)) {
		return nullptr;
	}

	auto img = scratchImage.GetImage(0, 0, 0);

	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	texHeapProp.CreationNodeMask = 1;
	texHeapProp.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = metaData.format;
	resDesc.Width = metaData.width;
	resDesc.Height = metaData.height;
	resDesc.DepthOrArraySize = metaData.arraySize;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.MipLevels = metaData.mipLevels;
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metaData.dimension);
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ID3D12Resource* texbuff = nullptr;
	result = dev->CreateCommittedResource(&texHeapProp, D3D12_HEAP_FLAG_NONE,
		&resDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, IID_PPV_ARGS(&texbuff));
	if (FAILED(result)) {
		return nullptr;
	}

	result = texbuff->WriteToSubresource(0, nullptr, img->pixels,
		img->rowPitch, img->slicePitch);
	if (FAILED(result)) {
		return nullptr;
	}

	toonResTable[texpath] = texbuff;
	return texbuff;
}

std::wstring TextureManager::GetWstringFromString(const std::string& str)
{
	auto num1 = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(), -1, nullptr, 0);

	std::wstring wstr;
	wstr.resize(num1);

	auto num2 = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(), -1, &wstr[0], num1);
	assert(num1 == num2);
	return wstr;
}

std::string TextureManager::GetStringFromWstring(const std::wstring& str)
{
	auto num = WideCharToMultiByte(CP_OEMCP, 0, str.c_str(), -1, nullptr, 0, nullptr, nullptr);

	char* tmpStr = new char[num];
	WideCharToMultiByte(CP_OEMCP, 0, str.c_str(), -1, tmpStr, num, nullptr, nullptr);
	std::string resStr = tmpStr;

	return resStr;
}
