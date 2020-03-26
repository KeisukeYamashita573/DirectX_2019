#pragma once
#include "PrimitiveMesh.h"
#include <vector>

using Microsoft::WRL::ComPtr;

class PlaneMesh :
	public PrimitiveMesh
{
private:
	ComPtr<ID3D12Resource> vBuffer;
	ComPtr<ID3D12RootSignature> primRootSignature;
	ComPtr<ID3D12PipelineState> primPipelineState;
	ComPtr<ID3DBlob> primVSShader;
	ComPtr<ID3DBlob> primPSShader;
	ID3D12DescriptorHeap* primConstantHeap = nullptr;
	ComPtr<ID3D12Resource> primCBuffer;
	D3D12_VERTEX_BUFFER_VIEW vbView;
	std::vector<D3D12_DESCRIPTOR_RANGE> primDescTblRange;
	std::vector<D3D12_ROOT_PARAMETER> primRootParam;
	PrimWVP* primMappedMatrix = nullptr;

	void CreatePrimDescHeapCBV(const ComPtr<ID3D12Device> dev);

public:
	PlaneMesh(ID3D12Device* dev, const DirectX::XMFLOAT3& pos, float width, float depth);
	~PlaneMesh();

	ComPtr<ID3D12Resource> GetVBuffer() override;
	D3D12_VERTEX_BUFFER_VIEW GetVBView() override;
	ComPtr<ID3D12RootSignature> GetPrimRootSignature() override;
	ComPtr<ID3D12PipelineState> GetPrimPipelineState() override;
	PrimWVP GetMatrixWVP() override;
	std::vector<D3D12_DESCRIPTOR_RANGE> GetDescTbls() override;
	std::vector<D3D12_ROOT_PARAMETER> GetRootParams()override;

	void CreatePrimRootsignature(const ComPtr<ID3D12Device> dev) override;
	void CreatePrimPipelineState(const ComPtr<ID3D12Device> dev) override;

	void Draw(ComPtr<ID3D12GraphicsCommandList> cmdlist) override;
};

