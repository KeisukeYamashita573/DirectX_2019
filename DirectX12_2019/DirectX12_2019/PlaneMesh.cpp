#include "PlaneMesh.h"
#include "App.h"
#include <array>
#include <d3dcompiler.h>

#pragma comment(lib,"d3dcompiler.lib")

namespace {
	float Magnitude(const DirectX::XMFLOAT3& var)
	{
		return sqrt(var.x * var.x + var.y * var.y + var.z * var.z);
	}

	DirectX::XMFLOAT3 operator-(DirectX::XMFLOAT3& var, DirectX::XMFLOAT3& val)
	{
		return DirectX::XMFLOAT3(var.x - val.x, var.y - val.y, var.z - val.z);
	}
}

PlaneMesh::PlaneMesh(ID3D12Device* dev, const DirectX::XMFLOAT3& pos, float width, float depth)
{
	std::array<PrimVertex, 4> vertices = {
		{PrimVertex(DirectX::XMFLOAT3(-100,0.4f,-100),DirectX::XMFLOAT3(0,1,0),DirectX::XMFLOAT2(0,0)),
		PrimVertex(DirectX::XMFLOAT3(-100,0.4f,100),DirectX::XMFLOAT3(0,1,0),DirectX::XMFLOAT2(0,1)),
		PrimVertex(DirectX::XMFLOAT3(100,0.4f,-100),DirectX::XMFLOAT3(0,1,0),DirectX::XMFLOAT2(1,0)),
		PrimVertex(DirectX::XMFLOAT3(100,0.4f,100),DirectX::XMFLOAT3(0,1,0),DirectX::XMFLOAT2(1,1)),
		}
	};

	auto result = dev->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)), D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&vBuffer));
	assert(result == S_OK);

	PrimVertex* primVertMap = nullptr;
	result = vBuffer->Map(0, nullptr, (void**)& primVertMap);
	std::copy(std::begin(vertices), std::end(vertices), primVertMap);
	vBuffer->Unmap(0, nullptr);
	assert(result == S_OK);

	vbView.BufferLocation = vBuffer->GetGPUVirtualAddress();
	vbView.SizeInBytes = sizeof(vertices);
	vbView.StrideInBytes = sizeof(PrimVertex);

	// シェーダーコンパイル
	ID3DBlob* primError = nullptr;
	result = D3DCompileFromFile(L"Primitive.hlsl", nullptr, nullptr, "vs", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &primVSShader, &primError);
	assert(result == S_OK);
	result = D3DCompileFromFile(L"Primitive.hlsl", nullptr, nullptr, "ps", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &primPSShader, &primError);
	assert(result == S_OK);

	// デスクリプターテーブルとルートシグネチャーの設定
	primDescTblRange.resize(1);
	primRootParam.resize(1);

	primDescTblRange[0].NumDescriptors = 1;
	primDescTblRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	primDescTblRange[0].BaseShaderRegister = 0;
	primDescTblRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	/*primDescTblRange[1].NumDescriptors = 1;
	primDescTblRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	primDescTblRange[1].BaseShaderRegister = 0;
	primDescTblRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;*/

	primRootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	primRootParam[0].DescriptorTable.NumDescriptorRanges = 1;
	primRootParam[0].DescriptorTable.pDescriptorRanges = primDescTblRange.data();
	primRootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	/*primRootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	primRootParam[1].DescriptorTable.NumDescriptorRanges = 1;
	primRootParam[1].DescriptorTable.pDescriptorRanges = &primDescTblRange[1];
	primRootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;*/

	CreatePrimDescHeapCBV(dev);
}

PlaneMesh::~PlaneMesh()
{
	if (primConstantHeap != nullptr) {
		primConstantHeap->Release();
	}
	/*if (primMappedMatrix != nullptr) {
		free(primMappedMatrix);
	}*/
}

ComPtr<ID3D12Resource> PlaneMesh::GetVBuffer()
{
	return vBuffer;
}

D3D12_VERTEX_BUFFER_VIEW PlaneMesh::GetVBView()
{
	return vbView;
}

ComPtr<ID3D12RootSignature> PlaneMesh::GetPrimRootSignature()
{
	return primRootSignature;
}

ComPtr<ID3D12PipelineState> PlaneMesh::GetPrimPipelineState()
{
	return primPipelineState;
}

PrimWVP PlaneMesh::GetMatrixWVP()
{
	return *primMappedMatrix;
}

std::vector<D3D12_DESCRIPTOR_RANGE> PlaneMesh::GetDescTbls()
{
	return primDescTblRange;
}

std::vector<D3D12_ROOT_PARAMETER> PlaneMesh::GetRootParams()
{
	return primRootParam;
}

void PlaneMesh::CreatePrimRootsignature(const ComPtr<ID3D12Device> dev)
{
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.NumParameters = primRootParam.size();
	rsd.pParameters = primRootParam.data();

	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplerDesc.ShaderRegister = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	samplerDesc.RegisterSpace = 0;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

	rsd.pStaticSamplers = &samplerDesc;
	rsd.NumParameters = 1;

	ID3DBlob* rootSignatureBlob = nullptr;
	ID3DBlob* error = nullptr;

	auto result = D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, &error);
	assert(result == S_OK);
	result = dev->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&primRootSignature));
	assert(result == S_OK);

}

void PlaneMesh::CreatePrimPipelineState(const ComPtr<ID3D12Device> dev)
{
	D3D12_INPUT_ELEMENT_DESC PrimLeyoutDesc[] = {
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"COLOR",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(primVSShader.Get());
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(primPSShader.Get());
	gpsDesc.DepthStencilState.DepthEnable = false;
	gpsDesc.DepthStencilState.StencilEnable = false;
	gpsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_NEVER;
	gpsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	gpsDesc.NumRenderTargets = 1;
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	gpsDesc.pRootSignature = primRootSignature.Get();

	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.SampleDesc.Quality = 0;
	gpsDesc.SampleMask = UINT_MAX;

	gpsDesc.InputLayout.pInputElementDescs = PrimLeyoutDesc;
	gpsDesc.InputLayout.NumElements = _countof(PrimLeyoutDesc);
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	auto result = dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&primPipelineState));
	assert(result == S_OK);
}

void PlaneMesh::CreatePrimDescHeapCBV(const ComPtr<ID3D12Device> dev)
{
	App& app = App::Instance();
	auto wsize = app.GetWidnowInfo();
	auto size = sizeof(PrimWVP);
	size = (size + 0xff) & ~0xff;

	DirectX::XMFLOAT3 eye(0, 15, -15.f);
	DirectX::XMFLOAT3 target(0, 10, 0);
	DirectX::XMFLOAT3 up(0, 1, 0);
	DirectX::XMVECTOR veye = DirectX::XMLoadFloat3(&eye);
	DirectX::XMVECTOR vtarget = DirectX::XMLoadFloat3(&target);
	DirectX::XMVECTOR vup = DirectX::XMLoadFloat3(&up);
	DirectX::XMFLOAT3 lightEye(1, -1, 1);
	DirectX::XMVECTOR vlightEye = DirectX::XMLoadFloat3(&lightEye);
	auto angle = 0.f;
	PrimWVP wvp = {};
	wvp.world = DirectX::XMMatrixRotationY(angle);
	auto lightPos = DirectX::XMVectorScale(vlightEye, Magnitude(target - eye));
	wvp.viewProj = DirectX::XMMatrixLookAtLH(lightPos, vtarget, vup) * 
		DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV2, static_cast<float>(wsize.width) / static_cast<float>(wsize.height), 0.1f, 300.f);

	// バッファ作成
	D3D12_DESCRIPTOR_HEAP_DESC primDescHeap = {};
	primDescHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	primDescHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	primDescHeap.NumDescriptors = 1;
	primDescHeap.NodeMask = 0;
	auto result = dev->CreateDescriptorHeap(&primDescHeap, IID_PPV_ARGS(&primConstantHeap));
	assert(result == S_OK);

	result = dev->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size), D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&primCBuffer));
	assert(result == S_OK);

	D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
	desc.BufferLocation = primCBuffer->GetGPUVirtualAddress();
	desc.SizeInBytes = size;
	auto handle = primConstantHeap->GetCPUDescriptorHandleForHeapStart();
	dev->CreateConstantBufferView(&desc, handle);

	result = primCBuffer->Map(0, nullptr, (void**)& primMappedMatrix);
	assert(result == S_OK);
	primMappedMatrix->world = wvp.world;
	primMappedMatrix->viewProj = wvp.viewProj;
}

void PlaneMesh::Draw(ComPtr<ID3D12GraphicsCommandList> cmdlist)
{
	cmdlist->SetPipelineState(primPipelineState.Get());
	cmdlist->SetGraphicsRootSignature(primRootSignature.Get());
	cmdlist->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	cmdlist->IASetVertexBuffers(0, 1, &vbView);
	cmdlist->SetDescriptorHeaps(1, &primConstantHeap);
	cmdlist->SetGraphicsRootDescriptorTable(0, primConstantHeap->GetGPUDescriptorHandleForHeapStart());
	cmdlist->DrawInstanced(4, 1, 0, 0);
}