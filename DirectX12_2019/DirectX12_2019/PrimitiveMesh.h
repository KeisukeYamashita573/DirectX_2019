#pragma once
#include <DirectXMath.h>
#include <DirectXTex.h>
#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <wrl.h>
#include "Dx12Initer.h"

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"DirectXTex.lib")

using Microsoft::WRL::ComPtr;

class PrimitiveMesh
{
private:

public:
	PrimitiveMesh();
	~PrimitiveMesh();

	virtual ComPtr<ID3D12Resource> GetVBuffer() = 0;
	virtual D3D12_VERTEX_BUFFER_VIEW GetVBView() = 0;
	virtual ComPtr<ID3D12RootSignature> GetPrimRootSignature() = 0;
	virtual ComPtr<ID3D12PipelineState> GetPrimPipelineState() = 0;
	virtual PrimWVP GetMatrixWVP() = 0;
	virtual std::vector<D3D12_DESCRIPTOR_RANGE> GetDescTbls() = 0;
	virtual std::vector<D3D12_ROOT_PARAMETER> GetRootParams() = 0;

	virtual void CreatePrimRootsignature(const ComPtr<ID3D12Device> dev) = 0;
	virtual void CreatePrimPipelineState(const ComPtr<ID3D12Device> dev) = 0;

	virtual void Draw(ComPtr<ID3D12GraphicsCommandList> cmdlist) = 0;
};

