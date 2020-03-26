#include "Dx12Initer.h"
#include <cassert>
#include "App.h"
#include <exception>
#include <d3dCompiler.h>
#include <synchapi.h>
#include <string>
#include <algorithm>
#include <dxgidebug.h>
#include <d3d12sdklayers.h>
#include "PlaneMesh.h"
#include "PrimitiveMesh.h"
#include <Effekseer.h>
#include <EffekseerRendererDX12.h>
#include "imgui-master/imgui.h"
#include "imgui-master/examples/imgui_impl_dx12.h"
#include "imgui-master/examples/imgui_impl_win32.h"

#pragma comment(lib,"DirectXTex.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"LLGI.lib")
#pragma comment(lib,"Effekseer.lib")
#pragma comment(lib,"EffekseerRendererDX12.lib")

namespace {
	// ヒューチャーレベルの配列
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	Vertex Vertices[] = {
		{DirectX::XMFLOAT3(-1,-1,0),DirectX::XMFLOAT2(0,1) },
		{DirectX::XMFLOAT3(-1,1,0),DirectX::XMFLOAT2(0,0)},
		{DirectX::XMFLOAT3(1,-1,0),DirectX::XMFLOAT2(1,1) },
		{DirectX::XMFLOAT3(1,1,0),DirectX::XMFLOAT2(1,0)}
	};

	float Magnitude(const DirectX::XMFLOAT3& var) {
		return sqrt(var.x * var.x + var.y * var.y + var.z * var.z);
	}

	DirectX::XMFLOAT3 Normalize(const DirectX::XMFLOAT3& vec) {
		float mag = Magnitude(vec);
		DirectX::XMFLOAT3 vector(0, 0, 0);
		vector.x = vec.x / mag;
		vector.y = vec.y / mag;
		vector.z = vec.z / mag;
		return vector;
	}

	DirectX::XMFLOAT3 operator-(DirectX::XMFLOAT3& var, DirectX::XMFLOAT3& val) {
		return DirectX::XMFLOAT3(var.x - val.x, var.y - val.y, var.z - val.z);
	}

	size_t RoundupPowerOf2(size_t size) {
		size_t bit = 0x80000000;
		for (size_t i = 31; i >= 0; --i) {
			if (size & bit)break;
			bit >>= 1;
		}
		return bit + (bit % size);
	}

	EffekseerRenderer::Renderer* EfkRenderer;
	Effekseer::Manager* EfkManager;
	EffekseerRenderer::SingleFrameMemoryPool* efkMemoryPool;
	EffekseerRenderer::CommandList* efkCmdList;
	Effekseer::Effect* effect;
	Effekseer::Handle efkHandle = 0;
}

Dx12Initer::Dx12Initer()
{
#ifdef _DEBUG
	// デバッグレイヤを有効化
	ID3D12Debug* debugLayer = nullptr;
	auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
	debugLayer->EnableDebugLayer();
	debugLayer->Release();

#endif // _DEBUG
	// モデルのロード
	auto res =  pmdManager.ModelRead(modelPath);
	assert(res&&"PMDModelの読み込みに失敗しました。");

	/*res = pmxManager.LoadModel(pmxModelPath);
	assert(res && "PMXModelの読み込みに失敗しました。");*/

	res = vmdLoader.LoadVMD(vmdPath);
	assert(res && "VMDの読み込みに失敗しました。");

	camera = DirectX::XMFLOAT3(0, 20, -15.f);
	target = DirectX::XMFLOAT3(0, 10, 0);
	up = DirectX::XMFLOAT3(0, 1, 0);
	worldAngle = DirectX::XMFLOAT3(0, 0, 0);
	backColor.resize(4);
}


Dx12Initer::~Dx12Initer()
{
	// デスクリプターの解放
	if (rtvDescriptorHeap != nullptr) {
		rtvDescriptorHeap->Release();
	}
	if (dsvDescriptorHeap != nullptr) {
		dsvDescriptorHeap->Release();
	}
	if (rgstDescriptorHeap != nullptr) {
		rgstDescriptorHeap->Release();
	}
	if (matDescriptorHeap != nullptr) {
		matDescriptorHeap->Release();
	}
	if (boneHeap != nullptr) {
		boneHeap->Release();
	}
	if (rgbaRDescriptorHeap != nullptr) {
		rgbaRDescriptorHeap->Release();
	}
	if (rgbaSDescriptorHeap != nullptr) {
		rgbaSDescriptorHeap->Release();
	}
	if (spriteFontHeap != nullptr) {
		spriteFontHeap->Release();
	}
	if (pmxDescHeap != nullptr) {
		pmxDescHeap->Release();
	}
	if (shadowDSV != nullptr) {
		shadowDSV->Release();
	}
	if (shadowSRV != nullptr) {
		shadowSRV->Release();
	}
	if (primConstantHeap != nullptr) {
		primConstantHeap->Release();
	}
	if (ImguiDescriptorHeap != nullptr) {
		ImguiDescriptorHeap->Release();
	}
	if (EfkRenderer != nullptr) {
		EfkRenderer->Release();
	}
	if (EfkManager != nullptr) {
		EfkManager->Release();
	}
	if (efkMemoryPool != nullptr) {
		efkMemoryPool->Release();
	}
	if (efkCmdList != nullptr) {
		efkCmdList->Release();
	}
	if (effect != nullptr) {
		effect->Release();
	}
	TerminateImgui();
}

void Dx12Initer::Initialize()
{
	CreateFactory();
	CreateDevice();
	CreateSwapChain();
	CreateDH();
	CreateRTV();
	CreateCommand();
	CreateVBuffer();
	CreateIBuffer();
	CreateVertexBuffer(DirectX::XMFLOAT3(0, 0, 0), 200, 200);
	CreatePeraVBuffer();
	CreateRMCBV();
	// PMXの頂点、インデックス設定
	/*PmxCreateVBuffer();
	PmxCreateIBuffer();*/
	CreateCBV();
	// 床表示用
	CreatePrimConstantBuffer();
	CreateDSV();
	CreatePMDTexBuff();
	// PMXのテクスチャバッファ設定
	//CreatePmxTexBuffer();
	CreateBoneTree();
	CreateBoneCBV();
	CreateMatBuff();
	CreateMatView();
	// PMXのマテリアル設定
	//PmxCreateMaterial();
	Create1stResource();
	Create1stPathView();
	CreateImguiSRV();
	InitializeImgui(App::Instance().GetHwnd(), dev.Get(), DXGI_FORMAT_R8G8B8A8_UNORM, 2,
		ImguiDescriptorHeap, ImguiDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), ImguiDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	CreateShadowDescHeap();
	ShaderSetting();
	CreateSampler();
	CreateFence();
	CreateRootSignature();
	CreatePSO();
}

void Dx12Initer::Runnable()
{
	// ループ実行
	ThrowCmd();
}

D3D12_VIEWPORT Dx12Initer::GetViewPort() const
{
	return port;
}

ComPtr<ID3D12DescriptorHeap> Dx12Initer::CreateDescriptorHeapForSpriteFont()
{
	if (spriteFontHeap == nullptr) {
		D3D12_DESCRIPTOR_HEAP_DESC spriteFontHeapDesc = {};
		spriteFontHeapDesc.NodeMask = 0;
		spriteFontHeapDesc.NumDescriptors = 1;
		spriteFontHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		spriteFontHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		auto result = dev->CreateDescriptorHeap(&spriteFontHeapDesc, IID_PPV_ARGS(&spriteFontHeap));
		assert(result == S_OK);
	}
	return spriteFontHeap;
}

void Dx12Initer::CreateImguiSRV()
{
	D3D12_DESCRIPTOR_HEAP_DESC ImguiDescHeap = {};
	ImguiDescHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ImguiDescHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ImguiDescHeap.NumDescriptors = 1;
	ImguiDescHeap.NodeMask = 0;

	auto result = dev->CreateDescriptorHeap(&ImguiDescHeap, IID_PPV_ARGS(&ImguiDescriptorHeap));
	assert(result == S_OK);
}

void Dx12Initer::InitializeImgui(HWND hwnd, ID3D12Device* device, DXGI_FORMAT formatRtv, UINT buffCnt,ID3D12DescriptorHeap* heapSrv, D3D12_CPU_DESCRIPTOR_HANDLE hCpu, D3D12_GPU_DESCRIPTOR_HANDLE hGpu)
{
	IMGUI_CHECKVERSION();
	void* mHwnd = nullptr;
	ImGui::CreateContext();
	auto imguiResult = ImGui_ImplWin32_Init(hwnd);
	assert(imguiResult);
	imguiResult = ImGui_ImplDX12_Init(device, buffCnt, formatRtv,heapSrv, hCpu, hGpu);
	assert(imguiResult);
}

void Dx12Initer::TerminateImgui()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void Dx12Initer::CreateVBuffer()
{
	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	auto vertData = pmdManager.GetPMDVertices();
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = vertData.size();
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.SampleDesc.Count = 1;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	auto result = dev->CreateCommittedResource(&heapProp,
		D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&vertexBuffer));
	assert(result == S_OK);

	// 頂点バッファーのマップ
	vertMap = nullptr;
	result = vertexBuffer->Map(0, nullptr, (void**)& vertMap);
	std::copy(vertData.begin(), vertData.end(), vertMap);
	vertexBuffer->Unmap(0,nullptr);
	assert(result == S_OK);

	vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	// VerticesInfoのサイズが40なので38にするため－2する
	vbView.StrideInBytes = sizeof(VerticesInfo) - 2;
	vbView.SizeInBytes = (UINT)vertData.size();
}

void Dx12Initer::CreateIBuffer()
{
	auto indexData = pmdManager.GetPMDIndices();

	auto result = dev->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(indexData.size() * sizeof(unsigned short)),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indexBuffer));

	// インデックスバッファビューの設定
	ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = (UINT)(indexData.size() * sizeof(unsigned short));

	// インデックス情報をマップ
	unsigned short* indexMap = nullptr;
	result = indexBuffer->Map(0, nullptr, (void**)& indexMap);
	std::copy(indexData.begin(), indexData.end(), indexMap);
	indexBuffer->Unmap(0, nullptr);
	assert(result == S_OK);
}

void Dx12Initer::CreatePeraVBuffer()
{
	auto result = dev->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(Vertices)), D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&vertexPeraBuffer));
	assert(result == S_OK);

	Vertex* vertMap = nullptr;
	result = vertexPeraBuffer->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(Vertices), std::end(Vertices), vertMap);
	vertexPeraBuffer->Unmap(0, nullptr);
	assert(result == S_OK);

	peraVbView.BufferLocation = vertexPeraBuffer->GetGPUVirtualAddress();
	peraVbView.SizeInBytes = sizeof(Vertices);
	peraVbView.StrideInBytes = sizeof(Vertex);
}

void Dx12Initer::CreateDevice()
{
	// グラフィックカードを取得
	std::vector<IDXGIAdapter*> adapters;
	IDXGIAdapter* adapter = nullptr;
	for (auto i = 0; dxgi->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
		adapters.push_back(adapter);
	}

	// 取得したグラボからnVidiaのグラボを選ぶ
	for (auto adpt : adapters) {
		DXGI_ADAPTER_DESC adesc = {};
		// DXGI_ADAPTER_DESCの情報を取得
		adpt->GetDesc(&adesc);
		// 取得した情報の中から自身の名前を取得
		std::wstring strDesc = adesc.Description;
		// strDescにNVIDIAの文字が入っていたらadapterの変数の中身を変える
		if (strDesc.find(L"NVIDIA") != std::string::npos) {
			adapter = adpt;
		}

	}

	// デバイスの生成
	for (auto l : levels) {
		auto result = D3D12CreateDevice(adapter, l, IID_PPV_ARGS(&dev));
		if (SUCCEEDED(result)) {
			level = l;
			break;
		}
	}
#ifdef _DEBUG
	dev.As(&infoQueue);
	D3D12_MESSAGE_ID denyIds[] = {
		D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
	};
	D3D12_MESSAGE_SEVERITY severities[] = {
		D3D12_MESSAGE_SEVERITY_INFO
	};
	D3D12_INFO_QUEUE_FILTER filter = {};
	filter.DenyList.NumIDs = _countof(denyIds);
	filter.DenyList.pIDList = denyIds;
	filter.DenyList.NumSeverities = _countof(severities);
	filter.DenyList.pSeverityList = severities;
	infoQueue->PushStorageFilter(&filter);
	infoQueue->Release();
#endif // DEBUG
}

void Dx12Initer::CreateFactory()
{
	// ファクトリーの生成
	auto result = CreateDXGIFactory1(__uuidof(IDXGIFactory6), (void**)(&dxgi));
	assert(result == S_OK);
}

void Dx12Initer::CreateSwapChain()
{
	IDXGIFactory4* factory = nullptr;
	auto result = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
	assert(result == S_OK);
	// サンプルデスク初期化
	DXGI_SAMPLE_DESC sampleDesc = {};
	sampleDesc.Count = 1;
	sampleDesc.Quality = 0;

	// 画像サイズ取得
	auto wsize = App::Instance().GetWidnowInfo();

	// スワップチェインデスク初期化
	swapchainDesc = {};
	swapchainDesc.Width = wsize.width;
	swapchainDesc.Height = wsize.height;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc = sampleDesc;
	swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchainDesc.BufferCount = 2;
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	// コマンドキュー生成
	CreateCommandQueue();

	result = factory->CreateSwapChainForHwnd(cmdQue.Get(),
		App::Instance().GetHwnd(), &swapchainDesc, nullptr, nullptr, (IDXGISwapChain1**)(&swapchain));
	assert(result == S_OK);

	auto bbformat = DXGI_FORMAT_R8G8B8A8_UNORM;
	EfkRenderer = EffekseerRendererDX12::Create(dev.Get(),
		cmdQue.Get(),		// コマンドキュー
		2,					// スワップチェーンの数
		&bbformat,			// バッファのフォーマット
		1,					// レンダーターゲットの数
		false,				// デプス有効フラグ
		false,				// 反転デプス有効フラグ
		2000);				// 最大パーティクル数

	EfkManager = Effekseer::Manager::Create(10000);

	EfkManager->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);
	EfkManager->SetSpriteRenderer(EfkRenderer->CreateSpriteRenderer());
	EfkManager->SetRibbonRenderer(EfkRenderer->CreateRibbonRenderer());
	EfkManager->SetRingRenderer(EfkRenderer->CreateRingRenderer());
	EfkManager->SetTrackRenderer(EfkRenderer->CreateTrackRenderer());
	EfkManager->SetModelRenderer(EfkRenderer->CreateModelRenderer());

	EfkManager->SetTextureLoader(EfkRenderer->CreateTextureLoader());
	EfkManager->SetModelLoader(EfkRenderer->CreateModelLoader());

	efkMemoryPool = EffekseerRendererDX12::CreateSingleFrameMemoryPool(EfkRenderer);
	efkCmdList = EffekseerRendererDX12::CreateCommandList(EfkRenderer, efkMemoryPool);

	EfkRenderer->SetCommandList(efkCmdList);

	EfkRenderer->SetProjectionMatrix(Effekseer::Matrix44().PerspectiveFovLH(90.f / 180.f * 3.14f, 1280.f / 720.f, 1.0f, 100.0f));
	EfkRenderer->SetCameraMatrix(Effekseer::Matrix44().LookAtLH(Effekseer::Vector3D(0, 20, -15.f), Effekseer::Vector3D(0.f, 10.f, 0.f), Effekseer::Vector3D(0.f, 1.f, 0.f)));

	effect = Effekseer::Effect::Create(EfkManager, (const EFK_CHAR*)L"effect/sword.efk");


}

void Dx12Initer::CreateCommandQueue()
{
	// コマンドキューの作成
	D3D12_COMMAND_QUEUE_DESC cmdQDesc = {};
	cmdQDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQDesc.NodeMask = 0;
	cmdQDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	auto result = dev->CreateCommandQueue(&cmdQDesc, IID_PPV_ARGS(&cmdQue));
	assert(result == S_OK);
}

void Dx12Initer::CreateDH()
{
	// RTV用デスクリプターヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NodeMask = 0;
	rtvHeapDesc.NumDescriptors = 2;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	auto result = dev->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));
	assert(result == S_OK);
}

void Dx12Initer::CreateRTV()
{
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	swapchain->GetDesc(&swcDesc);
	// レンダーターゲットの数を取得, 数ぶん確保
	int renderTargetsNum = swcDesc.BufferCount;
	renderTargets.resize(renderTargetsNum);

	// デスクリプタ一つのサイズを取得
	heapSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	descriptorHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (int i = 0; i < renderTargetsNum; i++) {
		auto result = swapchain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
		assert(result == S_OK);
		dev->CreateRenderTargetView(renderTargets[i], nullptr, descriptorHandle);
		descriptorHandle.ptr += heapSize;
	}
}

void Dx12Initer::CreateCommand()
{
	// コマンドアロケーター、リストの作成
	dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator));
	dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator.Get(), nullptr, IID_PPV_ARGS(&cmdList));
}

void Dx12Initer::CreateCBV()
{
	// CBV用ヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NodeMask = 0;
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	auto result = dev->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&rgstDescriptorHeap));
	assert(result == S_OK);

	D3D12_HEAP_PROPERTIES cbvHeapProp = {};
	cbvHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	cbvHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	cbvHeapProp.CreationNodeMask = 1;
	cbvHeapProp.VisibleNodeMask = 1;
	cbvHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

	auto size = (sizeof(WVP) + 0xff) & ~0xff;

	result = dev->CreateCommittedResource(&cbvHeapProp, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&cbvMat));
	assert(result == S_OK);

	auto handle = rgstDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	cbvDesc.BufferLocation = cbvMat->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = (UINT)size;
	// 回転情報用CBV作成
	dev->CreateConstantBufferView(&cbvDesc, handle);

	result = cbvMat->Map(0, nullptr, (void**)& matrix);
	assert(result == S_OK);

	// 回転情報をの初期設定
	DirectX::XMFLOAT3 camera(0, 0, -1.5f);
	DirectX::XMFLOAT3 target(0, 0, 0);
	DirectX::XMFLOAT3 up(0, 1, 0);
	DirectX::XMFLOAT3 light(-5, 2, -2);
	DirectX::XMVECTOR vcamera = DirectX::XMLoadFloat3(&camera);
	DirectX::XMVECTOR vtarget = DirectX::XMLoadFloat3(&target);
	DirectX::XMVECTOR vup = DirectX::XMLoadFloat3(&up);
	DirectX::FXMVECTOR vlight = DirectX::XMLoadFloat3(&light);
	auto wsize = App::Instance().GetWidnowInfo();
	auto wmat = DirectX::XMMatrixRotationY(0.f);
	auto cmat = DirectX::XMMatrixLookAtLH(vcamera, vtarget, vup);
	auto pmat = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV2, 
		static_cast<float>(wsize.width) / static_cast<float>(wsize.height), 0.1f, 300.f);
	auto lightPos = DirectX::XMVectorScale(vlight,Magnitude(target - camera));
	matrix->lvp = DirectX::XMMatrixLookAtLH(lightPos, vtarget, vup) * DirectX::XMMatrixOrthographicLH(40, 40, 0.1f, 300.f);

	matrix->world = wmat;
	matrix->view = cmat;
	matrix->proj = pmat;
	matrix->eye = camera;
	wmat = wmat * cmat * pmat;
	matrix->wvp = wmat;
}

void Dx12Initer::CreateDSV()
{
	// DSV用デスクリプター作成
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	auto result = dev->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&dsvDescriptorHeap));
	assert(result == S_OK);

	auto wsize = App::Instance().GetWidnowInfo();

	D3D12_RESOURCE_DESC depthDesc = {};
	depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthDesc.Width = wsize.width;
	depthDesc.Height = wsize.height;
	depthDesc.DepthOrArraySize = 1;
	depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES depthHeapProp = {};
	depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	depthHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_CLEAR_VALUE depthClearValue = {};
	float clearColor[] = { 0.5f,0.5f,0.5f,1.f };
	depthClearValue.Color[0] = clearColor[0];
	depthClearValue.Color[1] = clearColor[1];
	depthClearValue.Color[2] = clearColor[2];
	depthClearValue.Color[3] = clearColor[3];
	depthClearValue.DepthStencil.Depth = 1.f;
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;

	

	result = dev->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE, &depthDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue, IID_PPV_ARGS(&depthBuffer));
	assert(result == S_OK);

	// DSV作成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.Texture2D.MipSlice = 0;
	dev->CreateDepthStencilView(depthBuffer.Get(), &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

void Dx12Initer::CreateMatBuff()
{
	int midx = 0;
	auto materialsBuff = pmdManager.GetPMDMaterials();
	auto size = (sizeof(MaterialInfo) + 0xff) & ~0xff;
	materialBuffer.resize(materialsBuff.size());

	// ヒープの設定
	D3D12_HEAP_PROPERTIES matHeapProp = {};
	matHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	matHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	matHeapProp.CreationNodeMask = 1;
	matHeapProp.VisibleNodeMask = 1;
	matHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

	std::vector<MaterialInfo> matsInfo;
	matsInfo.resize(materialsBuff.size());

	// 色情報などを設定
	for (auto i = 0; i < materialsBuff.size(); i++) {
		matsInfo[i].diffuse.x = materialsBuff[i].diffuseColor.x;
		matsInfo[i].diffuse.y = materialsBuff[i].diffuseColor.y;
		matsInfo[i].diffuse.z = materialsBuff[i].diffuseColor.z;
		matsInfo[i].diffuse.w = materialsBuff[i].alpha;

		matsInfo[i].specular.x = materialsBuff[i].specularColor.x;
		matsInfo[i].specular.y = materialsBuff[i].specularColor.y;
		matsInfo[i].specular.z = materialsBuff[i].specularColor.z;
		matsInfo[i].specular.w = materialsBuff[i].specularity;

		matsInfo[i].ambient.x = materialsBuff[i].mirrorColor.x;
		matsInfo[i].ambient.y = materialsBuff[i].mirrorColor.y;
		matsInfo[i].ambient.z = materialsBuff[i].mirrorColor.z;
	}

	// マテリアルを一つずつマップしていく
	for (auto& mbuff : materialBuffer) {
		auto result = dev->CreateCommittedResource(
			&matHeapProp,
			D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(size),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, IID_PPV_ARGS(&mbuff));
		assert(result == S_OK);
		MaterialInfo* mat = nullptr;
		result = mbuff->Map(0, nullptr, (void**)& mat);
		assert(result == S_OK);
		*mat = matsInfo[midx];
		mbuff->Unmap(0, nullptr);
		++midx;
	}
}

void Dx12Initer::CreateMatView()
{
	auto mats = pmdManager.GetPMDMaterials();
	D3D12_DESCRIPTOR_HEAP_DESC matHeapDesc = {};
	matHeapDesc.NodeMask = 0;
	matHeapDesc.NumDescriptors = static_cast<UINT>(mats.size()) * 5;
	matHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	matHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	auto result = dev->CreateDescriptorHeap(&matHeapDesc, IID_PPV_ARGS(&matDescriptorHeap));
	assert(result == S_OK);

	int idx = 0;
	auto size = (sizeof(MaterialInfo) + 0xff) & ~0xff;
	auto handle = matDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	auto increment = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	matDesc.SizeInBytes = (UINT)size;
	for (auto& m : mats) {
		// material用CBV作成
		matDesc.BufferLocation = materialBuffer[idx]->GetGPUVirtualAddress();
		dev->CreateConstantBufferView(&matDesc, handle);
		handle.ptr += increment;

		// 通常テクスチャ用SRV作成
		if (textureResource[idx] == nullptr) {
			auto whitetexture = texManager.CreateWhiteTex(dev);
			CreateSRVAndSetFormat(whitetexture, texSrvDesc, handle);
		}
		else {
			CreateSRVAndSetFormat(textureResource[idx].Get(), texSrvDesc, handle);
		}
		handle.ptr += increment;

		// sph用SRV作成
		if (sphResource[idx] == nullptr) {
			auto whitetexture = texManager.CreateWhiteTex(dev);
			CreateSRVAndSetFormat(whitetexture, texSrvDesc, handle);
		}
		else {
			CreateSRVAndSetFormat(sphResource[idx].Get(), texSrvDesc, handle);
		}
		handle.ptr += increment;

		// spa用SRV作成
		if (spaResource[idx] == nullptr) {
			auto blacktexture = texManager.CreateBlackTex(dev);
			CreateSRVAndSetFormat(blacktexture, texSrvDesc, handle);
		}
		else {
			CreateSRVAndSetFormat(spaResource[idx].Get(), texSrvDesc, handle);
		}
		handle.ptr += increment;

		// toon用SRV作成
		if (toonResource[idx] == nullptr) {
			auto gradationtexture = texManager.CreateGrayGradationTexture(dev);
			CreateSRVAndSetFormat(gradationtexture, texSrvDesc, handle);
		}
		else {
			CreateSRVAndSetFormat(toonResource[idx].Get(), texSrvDesc, handle);
		}
		handle.ptr += increment;

		++idx;
	}
}

void Dx12Initer::CreatePMDTexBuff()
{
	// マテリアル情報の取得
	auto pmdMats = pmdManager.GetPMDMaterials();
	// リソース管理用配列のリサイズ
	textureResource.resize(pmdMats.size());
	sphResource.resize(pmdMats.size());
	spaResource.resize(pmdMats.size());
	toonResource.resize(pmdMats.size());
	
	for (int i = 0; i < pmdMats.size(); i++) {
		std::string fileName = pmdMats[i].textureFileName;

		// Toon読み込み
		std::string toonFilePath = "toon/";
		char toonfileName[16];
		sprintf_s(toonfileName, "toon%02d.bmp", pmdMats[i].toonIndex + 1);
		toonFilePath += toonfileName;
		toonResource[i] = texManager.LoadTextureFromFile(dev,toonFilePath);

		// 通常テクスチャがない場合
		if (fileName.length() == 0) {
			textureResource[i] = nullptr;
		}
		if (std::count(fileName.begin(),fileName.end(),'*') > 0) {
			// 拡張子が"sph","spa"の場合の処理
			auto namepair = fileCont.SplitFileName(fileName);
			auto texFilePath = fileCont.GetPMDTexturePath(modelPath, fileName.c_str());
			if (fileCont.GetExtension(namepair.first) == "sph"){
				fileName = namepair.second;
				sphResource[i] = texManager.LoadTextureFromFile(dev, texFilePath);
			}
			else if (fileCont.GetExtension(namepair.first) == "spa") {
				fileName = namepair.second;
				spaResource[i] = texManager.LoadTextureFromFile(dev, texFilePath);
			}
			else {
				fileName = namepair.first;
			}
		}
		// テクスチャ名が一つの場合
		auto texFilePath = fileCont.GetPMDTexturePath(modelPath, fileName.c_str());

		if (fileCont.GetExtension(fileName) == "sph") {
			sphResource[i] = texManager.LoadTextureFromFile(dev, texFilePath);
		}
		else if (fileCont.GetExtension(fileName) == "spa") {
			spaResource[i] = texManager.LoadTextureFromFile(dev, texFilePath);
		}
		else {
			textureResource[i] = texManager.LoadTextureFromFile(dev, texFilePath, fileCont.GetExtension(fileName));
		}
	}
	
	// モデル用テクスチャSRV設定
	texSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	texSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	texSrvDesc.Texture2D.MipLevels = 1;
}

void Dx12Initer::CreateBoneTree()
{
	auto mbones = pmdManager.GetPMDBones();
	boneMats.resize(mbones.size());
	std::fill(boneMats.begin(), boneMats.end(), DirectX::XMMatrixIdentity());
	for (int idx = 0; idx < mbones.size(); idx++) {
		auto& b = mbones[idx];
		auto& boneNode = boneNodes[b.boneName];
		boneNode.boneIdx = idx;
		boneNode.startPos = b.boneHeadPos;
		boneNode.endPos = mbones[b.tailBoneIndex].boneHeadPos;
	}

	for (auto& b : boneNodes) {
		if (mbones[b.second.boneIdx].parentBoneIndex >= mbones.size())continue;
		auto parentName = mbones[mbones[b.second.boneIdx].parentBoneIndex].boneName;
		boneNodes[parentName].children.push_back(&b.second);
	}

	auto a = 0;
}

void Dx12Initer::CreateBoneCBV()
{
	D3D12_DESCRIPTOR_HEAP_DESC dh = {};
	dh.NodeMask = 0;
	dh.NumDescriptors = 1;
	dh.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	dh.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	auto result = dev->CreateDescriptorHeap(&dh, IID_PPV_ARGS(&boneHeap));

	DirectX::XMMATRIX matrix = DirectX::XMMatrixIdentity();
	auto size = (boneNodes.size() * sizeof(matrix) + 0xff) & ~0xff;

	result = dev->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&boneBuffer));
	assert(result == S_OK);

	D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
	desc.BufferLocation = boneBuffer->GetGPUVirtualAddress();
	desc.SizeInBytes = size;
	auto handle = boneHeap->GetCPUDescriptorHandleForHeapStart();
	dev->CreateConstantBufferView(&desc, handle);

	mappedMat = nullptr;
	boneMats.resize(boneNodes.size());
	std::fill(boneMats.begin(), boneMats.end(), DirectX::XMMatrixIdentity());
	boneBuffer->Map(0, nullptr, (void**)& mappedMat);

	/*auto Lelbow = boneNodes["左ひじ"];
	auto Lvec = DirectX::XMLoadFloat3(&Lelbow.startPos);
	auto Lmat = DirectX::XMMatrixTranslationFromVector(DirectX::XMVectorScale(Lvec,-1)) *
		DirectX::XMMatrixRotationZ(DirectX::XM_PIDIV2) * DirectX::XMMatrixTranslationFromVector(Lvec);
	RecursiveMatrixMultiply(Lelbow, Lmat);

	auto Relbow = boneNodes["右ひじ"];
	auto Rvec = DirectX::XMLoadFloat3(&Relbow.startPos);
	auto Rmat = DirectX::XMMatrixTranslationFromVector(DirectX::XMVectorScale(Rvec, -1)) *
		DirectX::XMMatrixRotationZ(-DirectX::XM_PIDIV2) * DirectX::XMMatrixTranslationFromVector(Rvec);
	RecursiveMatrixMultiply(Relbow, Rmat);*/
}

void Dx12Initer::Create1stPathView()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_DESCRIPTOR_HEAP_DESC RGBADescHeap = {};
	RGBADescHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	RGBADescHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	RGBADescHeap.NumDescriptors = 3;
	RGBADescHeap.NodeMask = 0;

	auto result = dev->CreateDescriptorHeap(&RGBADescHeap, IID_PPV_ARGS(&rgbaSDescriptorHeap));
	assert(result == S_OK);

	RGBADescHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	// レンダーターゲットはFlagsをShader_Visibleに設定できない
	RGBADescHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	result = dev->CreateDescriptorHeap(&RGBADescHeap, IID_PPV_ARGS(&rgbaRDescriptorHeap));
	assert(result == S_OK);

	auto handleRTV = rgbaRDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	auto handleSRV = rgbaSDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	auto handleIncRTV = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	auto handleIncSRV = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (auto& res : rgbaResource) {
		dev->CreateRenderTargetView(res.Get(), nullptr, handleRTV);
		dev->CreateShaderResourceView(res.Get(), &srvDesc, handleSRV);
		handleRTV.ptr += handleIncRTV;
		handleSRV.ptr += handleIncSRV;
	}
	// 縮小バッファ用
	/*for (auto& res : bloomBuffers) {
		dev->CreateRenderTargetView(res.Get(), nullptr, handleRTV);
		dev->CreateShaderResourceView(res.Get(), &srvDesc, handleSRV);
		handleRTV.ptr += handleIncRTV;
		handleSRV.ptr += handleIncSRV;
	}
	*/
}

void Dx12Initer::Create1stResource()
{
	auto wsize = App::Instance().GetWidnowInfo();
	std::vector<unsigned char> data(wsize.width * wsize.height * 4);
	std::fill(data.begin(), data.end(), 0xff);

	// ヒープ情報設定
	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	heapProp.CreationNodeMask = 1;
	heapProp.VisibleNodeMask = 1;

	// リソースデスクの設定
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Width = wsize.width;
	resDesc.Height = wsize.height;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	rgbaResource.resize(2);
	// リソースのコミット
	// マルチレンダーターゲットなので２つ
	for (auto& res : rgbaResource) {
		auto result = dev->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_RENDER_TARGET,
			nullptr, IID_PPV_ARGS(&res));
		assert(result == S_OK);
	}
}

void Dx12Initer::CreateSampler()
{
	// 通常のサンプラー設定
	samplerDesc[0].Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc[0].MinLOD = 0.f;
	samplerDesc[0].MipLODBias = 0.f;
	samplerDesc[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplerDesc[0].ShaderRegister = 0;
	samplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	samplerDesc[0].RegisterSpace = 0;
	samplerDesc[0].MaxAnisotropy = 0;
	samplerDesc[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

	// toon用
	samplerDesc[1] = samplerDesc[0];	// 変更点以外コピー
	samplerDesc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[1].ShaderRegister = 1;
}

void Dx12Initer::CreateFence()
{
	// フェンスの作成
	dev->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
}

void Dx12Initer::ThrowCmd()
{
	unsigned char keyState[256];
	GetKeyboardState(keyState);

	if (keyState[*"A"] & 0x80) {
		camera.x -= 1;
		target.x -= 1;
	}
	else if (keyState[*"D"] & 0x80) {
		camera.x += 1;
		target.x += 1;
	}

	if (keyState[*"W"] & 0x80) {
		camera.y += 1;
		target.y += 1;
	}
	else if (keyState[*"S"] & 0x80) {
		camera.y -= 1;
		target.y -= 1;
	}

	if (keyState[VK_UP] & 0x80) {
		camera.z += 1;
		target.z += 1;
	}
	else if (keyState[VK_DOWN] & 0x80) {
		camera.z -= 1;
		target.z -= 1;
	}

	if (keyState[*"J"] & 0x80) {
		worldAngle.y -= 0.05;
	}
	else if (keyState[*"L"] & 0x80) {
		worldAngle.y += 0.05;
	}

	if (keyState[*"I"] & 0x80) {
		worldAngle.x += 0.05;
	}
	else if (keyState[*"K"] & 0x80) {
		worldAngle.x -= 0.05;
	}

	DirectX::XMVECTOR vcamera = DirectX::XMLoadFloat3(&camera);
	DirectX::XMVECTOR vtarget = DirectX::XMLoadFloat3(&target);
	DirectX::XMVECTOR vup = DirectX::XMLoadFloat3(&up);
	DirectX::XMFLOAT3 light(-15, 35, -15);
	DirectX::FXMVECTOR vlight = DirectX::XMLoadFloat3(&Normalize(light));
	App& app = App::Instance();
	auto wsize = app.GetWidnowInfo();

	// 影描画
	auto result = cmdAllocator->Reset();
	cmdList->Reset(cmdAllocator.Get(), shadowPipelineState.Get());
	auto shadowIndex = swapchain->GetCurrentBackBufferIndex();
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(shadowBuffer.Get(), D3D12_RESOURCE_STATE_GENERIC_READ,
		D3D12_RESOURCE_STATE_DEPTH_WRITE));

	D3D12_VIEWPORT shadowPort = {};
	shadowPort.TopLeftX = 0;
	shadowPort.TopLeftY = 0;
	shadowPort.Width = 256;
	shadowPort.Height = 256;
	shadowPort.MinDepth = 0.f;
	shadowPort.MaxDepth = 1.f;

	RECT scissorShadowRect = {};
	scissorShadowRect.left = 0;
	scissorShadowRect.top = 0;
	scissorShadowRect.right = shadowPort.Width;
	scissorShadowRect.bottom = shadowPort.Height;

	cmdList->SetPipelineState(shadowPipelineState.Get());
	cmdList->SetGraphicsRootSignature(shadowRootSignature.Get());
	cmdList->RSSetViewports(1, &shadowPort);
	cmdList->RSSetScissorRects(1, &scissorShadowRect);
	cmdList->IASetVertexBuffers(0, 1, &vbView);
	cmdList->IASetIndexBuffer(&ibView);
	cmdList->OMSetRenderTargets(0, nullptr, false, &shadowDSV->GetCPUDescriptorHandleForHeapStart());
	cmdList->ClearDepthStencilView(shadowDSV->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
	cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	cmdList->SetDescriptorHeaps(1, &rgstDescriptorHeap);
	cmdList->SetGraphicsRootDescriptorTable(0, rgstDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	cmdList->SetDescriptorHeaps(1, &boneHeap);
	cmdList->SetGraphicsRootDescriptorTable(2, boneHeap->GetGPUDescriptorHandleForHeapStart());
	auto handle = matDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	for (auto i = 0; i < pmdManager.GetPMDMaterials().size(); i++) {
		cmdList->SetDescriptorHeaps(1, &matDescriptorHeap);
		handle.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 4;
	}

	auto shadowMat = DirectX::XMMatrixRotationY(angle) * DirectX::XMMatrixRotationY(worldAngle.y) * DirectX::XMMatrixRotationX(worldAngle.x);
	auto shadowCmat = DirectX::XMMatrixLookAtLH(vcamera, vtarget, vup);
	auto shadowPmat = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV2,
		static_cast<float>(wsize.width) / static_cast<float>(wsize.height), 0.1f, 300.f);
	// 結果をマップする
	matrix->world = shadowMat;
	matrix->view = shadowCmat;
	matrix->proj = shadowPmat;
	matrix->eye = camera;
	shadowMat = shadowMat * shadowCmat * shadowPmat;
	auto p = DirectX::XMFLOAT4(0, 1, 0, 0);
	auto l = DirectX::XMFLOAT4(-1, 1, -1, 0);
	auto shadowmat = DirectX::XMMatrixShadow(DirectX::XMLoadFloat4(&p), DirectX::XMLoadFloat4(&l));
	matrix->shadowMat = DirectX::XMMatrixRotationY(angle) * shadowmat;
	for (int i = 0; i < value; i += 2) {
		DirectX::XMVECTOR vec;
		vec = DirectX::XMLoadFloat3(&DirectX::XMFLOAT3((i % 10) * 10, 0, (i / 10) * 10));

		matrix->instancePos[i] = DirectX::XMMatrixTranslationFromVector(vec);
		matrix->instancePos[i + 1] = DirectX::XMMatrixTranslationFromVector(vec);
	}
	matrix->wvp = shadowMat;

	cmdList->DrawInstanced(0, 1, 0, 0);

	cmdList->SetDescriptorHeaps(1, &shadowSRV);
	cmdList->SetGraphicsRootDescriptorTable(3, shadowSRV->GetGPUDescriptorHandleForHeapStart());

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(shadowBuffer.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_GENERIC_READ));
	cmdList->Close();
	ExecuteCmdList();
	Wait();

	// 床表示
	result = cmdAllocator->Reset();
	cmdList->Reset(cmdAllocator.Get(), primPipelineState.Get());

	auto planeIndex = swapchain->GetCurrentBackBufferIndex();
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[planeIndex], D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET));

	// viewPortの設定
	D3D12_VIEWPORT primPort = {};
	primPort.TopLeftX = 0;
	primPort.TopLeftY = 0;
	primPort.Width = (FLOAT)App::Instance().GetWidnowInfo().width;
	primPort.Height = (FLOAT)App::Instance().GetWidnowInfo().height;
	primPort.MinDepth = 0.0f;
	primPort.MaxDepth = 1.0f;

	// Rectの設定
	RECT scissorPrimRect = {};
	scissorPrimRect.left = 0;
	scissorPrimRect.top = 0;
	scissorPrimRect.right = App::Instance().GetWidnowInfo().width;
	scissorPrimRect.bottom = App::Instance().GetWidnowInfo().height;

	// コマンドリストに情報をセット
	cmdList->SetPipelineState(primPipelineState.Get());
	cmdList->SetGraphicsRootSignature(primRootSignature.Get());
	cmdList->RSSetViewports(1, &primPort);
	cmdList->RSSetScissorRects(1, &scissorPrimRect);
	cmdList->IASetVertexBuffers(0, 1, &primVbView);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	auto primDescHandle = rgbaRDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	float PrimColor[] = { 1.f,0.5f,0.5f,1.f };
	cmdList->OMSetRenderTargets(1, &primDescHandle, false, &dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	cmdList->ClearRenderTargetView(primDescHandle, backColor.data(), 0, nullptr);

	auto lightScale = DirectX::XMVectorScale(vlight, Magnitude(target - camera));
	auto lvp = DirectX::XMMatrixLookAtLH(lightScale, vtarget, vup) * DirectX::XMMatrixOrthographicLH(40, 40, 0.1f, 300.f);

	// 行列計算
	primMappedMatrix->world = DirectX::XMMatrixIdentity() * DirectX::XMMatrixRotationY(worldAngle.y) * DirectX::XMMatrixRotationX(worldAngle.x);
	primMappedMatrix->viewProj = DirectX::XMMatrixLookAtLH(vcamera, vtarget, vup) *
		DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV2, static_cast<float>(wsize.width) / static_cast<float>(wsize.height), 0.1f, 300.f);

	cmdList->SetDescriptorHeaps(1, &primConstantHeap);
	cmdList->SetGraphicsRootDescriptorTable(0, primConstantHeap->GetGPUDescriptorHandleForHeapStart());
	if (activeFloorFlg) {
		cmdList->DrawInstanced(4, 1, 0, 0);
	}
	
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[planeIndex], D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT));

	// コマンドのクローズ
	result = cmdList->Close();

	// コマンドリスト実行
	ExecuteCmdList();
	// 待機処理
	Wait();


	// モデル処理
	auto heapStart = rgbaRDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	auto incSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvs[3];
	uint32_t Initoffset = 0;
	for (auto& hdl : rtvs) {
		hdl.InitOffsetted(heapStart, Initoffset);
		Initoffset += incSize;
	}
	// クリアカラー設定
	std::vector<FLOAT> clearColor = { 0.5f,0.5f,0.5f,1.f };
	std::vector<FLOAT> clearNormalColor = { 0.f,0.f,0.f,1.f };
	//float clearColor[] = { 1.f,0.f,0.f,1.f };
	// コマンドリセット
	result = cmdAllocator->Reset();
	cmdList->Reset(cmdAllocator.Get(), pipelineState.Get());
	// インデックスの取得
	auto bbIndex = swapchain->GetCurrentBackBufferIndex();
	// リソースバリアーを書き込める状態に設定
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[bbIndex], D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET));

	// ビューポート系設定
	SetViewPorts();

	// コマンドリスト系の設定
	cmdList->OMSetRenderTargets(2, rtvs, false, &dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	cmdList->ClearRenderTargetView(rtvs[1], clearNormalColor.data(), 0, nullptr);
	
	SetCmdList(rtvs, clearColor);

	auto wmat = DirectX::XMMatrixRotationY(angle) * DirectX::XMMatrixRotationY(worldAngle.y) * DirectX::XMMatrixRotationX(worldAngle.x);
	auto cmat = DirectX::XMMatrixLookAtLH(vcamera, vtarget, vup);
	auto pmat = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV2,
		static_cast<float>(wsize.width) / static_cast<float>(wsize.height), 0.1f, 300.f);
	// 結果をマップする
	matrix->world = wmat;
	matrix->view = cmat;
	matrix->proj = pmat;
	matrix->eye = camera;
	wmat = wmat * cmat * pmat;
	p = DirectX::XMFLOAT4(0, 1, 0, 0);
	l = DirectX::XMFLOAT4(-1, 1, -1, 0);
	shadowmat = DirectX::XMMatrixShadow(DirectX::XMLoadFloat4(&p), DirectX::XMLoadFloat4(&l));
	matrix->shadowMat = DirectX::XMMatrixRotationY(angle) * shadowmat;
	for (int i = 0; i < value; i += 2) {
		DirectX::XMVECTOR vec;
		vec = DirectX::XMLoadFloat3(&DirectX::XMFLOAT3((i % 20) * 10, 0, (i / 20) * 10));

		matrix->instancePos[i] = DirectX::XMMatrixTranslationFromVector(vec);
		matrix->instancePos[i + 1] = DirectX::XMMatrixTranslationFromVector(vec);
	}
	matrix->wvp = wmat;

	//angle += 0.05f;

	// モデルの描画
	unsigned int offset = 0;
	unsigned int idxCnt = 0;

	auto mathandle = matDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	auto increment = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
	ID3D12DescriptorHeap* matDescHeaps[] = { matDescriptorHeap };
	cmdList->SetDescriptorHeaps(1, matDescHeaps);
	for (auto& m : pmdManager.GetPMDMaterials()) {
		// デスクリプターテーブルの切り替え
		cmdList->SetGraphicsRootDescriptorTable(1, mathandle);
		idxCnt = m.faceVertCount;
		if (activeModelFlg) {
			cmdList->DrawIndexedInstanced(idxCnt, value * 2, offset, 0, 0);
		}
		mathandle.ptr += increment;
		offset += idxCnt;
	}
	 
	if (keyState[VK_SPACE] & 0x80) {
		if (EfkManager->Exists(efkHandle)) {
			EfkManager->StopEffect(efkHandle);
		}

		efkHandle = EfkManager->Play(effect, Effekseer::Vector3D(0, 10, 0));
	}

	EfkManager->Update();
	efkMemoryPool->NewFrame();
	EffekseerRendererDX12::BeginCommandList(efkCmdList, cmdList.Get());
	EfkRenderer->BeginRendering();
	EfkManager->Draw();
	EfkRenderer->EndRendering();
	EffekseerRendererDX12::EndCommandList(efkCmdList);

	// PMX表示用
	//auto mathandle = pmxDescHeap->GetGPUDescriptorHandleForHeapStart();
	//auto increment = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 2;
	//ID3D12DescriptorHeap* matDescHeaps[] = { pmxDescHeap };
	//cmdList->SetDescriptorHeaps(1, matDescHeaps);
	//for (auto& m : pmxManager.GetPMXMaterials()) {
	//	 //デスクリプターテーブルの切り替え
	//	cmdList->SetGraphicsRootDescriptorTable(1, mathandle);
	//	idxCnt = m.matSurfaceCnt;
	//	cmdList->DrawIndexedInstanced(idxCnt, 1, offset, 0, 0);
	//	mathandle.ptr += increment;
	//	offset += idxCnt;
	//}	

	// ボーン処理
	frame++;
	std::fill(boneMats.begin(), boneMats.end(), DirectX::XMMatrixIdentity());
	
	static auto ModellastTime = GetTickCount();
	if (static_cast<float>(GetTickCount() - ModellastTime) > vmdLoader.Duration() * 33.33333f) {
		ModellastTime = GetTickCount();
	}
	
	MotionUpdate(static_cast<float>(GetTickCount() - ModellastTime) / 33.33333f);
	std::copy(boneMats.begin(), boneMats.end(), mappedMat);

	cmdList->SetDescriptorHeaps(1, &spriteFontHeap);
	App::Instance().GetSpriteBatch()->Begin(cmdList.Get());
	App::Instance().GetSpriteFont()->DrawString(App::Instance().GetSpriteBatch(), L"げぼかわ", DirectX::XMFLOAT2(552, 102), DirectX::Colors::Black);
	App::Instance().GetSpriteFont()->DrawString(App::Instance().GetSpriteBatch(), L"げぼかわ", DirectX::XMFLOAT2(550, 100), DirectX::Colors::Orange);
	App::Instance().GetSpriteFont()->DrawString(App::Instance().GetSpriteBatch(), L"クソダサ", DirectX::XMFLOAT2(552, 152), DirectX::Colors::Black);
	App::Instance().GetSpriteFont()->DrawString(App::Instance().GetSpriteBatch(), L"クソダサ", DirectX::XMFLOAT2(550, 150), DirectX::Colors::Orange);
	App::Instance().GetSpriteFont()->DrawString(App::Instance().GetSpriteBatch(), L"クソ〇ねゴミカスーーーー", DirectX::XMFLOAT2(552, 202), DirectX::Colors::Black);
	App::Instance().GetSpriteFont()->DrawString(App::Instance().GetSpriteBatch(), L"クソ〇ねゴミカスーーーー", DirectX::XMFLOAT2(550, 200), DirectX::Colors::Red);
	App::Instance().GetSpriteBatch()->End();

	// バリアーの再設定
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[bbIndex], D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT));

	// コマンドのクローズ
	result = cmdList->Close();

	// コマンドリスト実行
	ExecuteCmdList();
	// 待機処理
	Wait();

	// ペラの処理
	cmdAllocator->Reset();
	cmdList->Reset(cmdAllocator.Get(), peraPipelineState.Get());

	D3D12_VIEWPORT view = {};
	view.TopLeftX = 0;
	view.TopLeftY = 0;
	view.Width = (FLOAT)App::Instance().GetWidnowInfo().width;
	view.Height = (FLOAT)App::Instance().GetWidnowInfo().height;
	view.MinDepth = 0.0f;
	view.MaxDepth = 1.0f;

	// Rectの設定
	RECT rec = {};
	rec.left = 0;
	rec.top = 0;
	rec.right = App::Instance().GetWidnowInfo().width;
	rec.bottom = App::Instance().GetWidnowInfo().height;

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	auto idx = swapchain->GetCurrentBackBufferIndex();
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[idx] , D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	cmdList->SetGraphicsRootSignature(rootPeraSignature.Get());
	cmdList->RSSetViewports(1, &view);
	cmdList->RSSetScissorRects(1, &rec);

	cmdList->IASetVertexBuffers(0, 2, &peraVbView);
	cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	cmdList->SetPipelineState(peraPipelineState.Get());
	auto peraDescH = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	auto peraHeapSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	peraDescH.ptr += peraHeapSize * idx;
	float peraColor[] = { 1.0f,0.5f,0.5f,1.f };
	cmdList->OMSetRenderTargets(1, &peraDescH, false, &dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	cmdList->ClearRenderTargetView(peraDescH, peraColor, 0, nullptr);
	cmdList->SetDescriptorHeaps(1, &rgbaSDescriptorHeap);
	cmdList->SetGraphicsRootDescriptorTable(0, rgbaSDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	cmdList->SetDescriptorHeaps(1, &rmMoveDescHeap);
	cmdList->SetGraphicsRootDescriptorTable(1, rmMoveDescHeap->GetGPUDescriptorHandleForHeapStart());

	movement->pos.x += 0.5f;
	movement->pos.y -= 0.5f;

	cmdList->DrawInstanced(4, 1, 0, 0);

	cmdList->SetDescriptorHeaps(1, &ImguiDescriptorHeap);
	ImGui::Begin("Information");
	ImGui::Text("Hello,ImGui world");
	ImGui::Separator();
	ImGui::Text("DebugWindow");
	ImGui::Checkbox("Active", &activeFlg);
	ImGui::Separator();
	ImGui::Text("DrawFloor");
	ImGui::Checkbox("ActiveFloor", &activeFloorFlg);
	ImGui::Separator();
	ImGui::Text("DrawModel");
	ImGui::Checkbox("ActiveModel", &activeModelFlg);
	ImGui::Separator();
	ImGui::Text("Framerate(avg) %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::Separator();
	ImGui::SliderInt("Slider", &value, minValue, maxValue);
	ImGui::Separator();
	ImGui::Text("InstanceCountControllerButton");
	if (ImGui::Button("Decrement")) {
		if (value > 1) {
			value--;
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Increment")) {
		if (value < 100) {
			value++;
		}
	}
	ImGui::ColorPicker4("ColorPicker", backColor.data());
	ImGui::ColorEdit4("Color", backColor.data(), ImGuiColorEditFlags_None);
	ImGui::End();
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList.Get());

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[idx], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	cmdList->Close();
	
	ExecuteCmdList();
	Wait();

	// ペラ1の処理
	cmdAllocator->Reset();
	cmdList->Reset(cmdAllocator.Get(), peraPipelineState.Get());

	D3D12_VIEWPORT view2 = {};
	view2.TopLeftX = 0;
	view2.TopLeftY = 0;
	view2.Width = (FLOAT)App::Instance().GetWidnowInfo().width / 4;
	view2.Height = (FLOAT)App::Instance().GetWidnowInfo().height / 4;
	view2.MinDepth = 0.0f;
	view2.MaxDepth = 1.0f;

	// Rectの設定
	RECT rec2 = {};
	rec2.left = 0;
	rec2.top = 0;
	rec2.right = App::Instance().GetWidnowInfo().width / 4;
	rec2.bottom = App::Instance().GetWidnowInfo().height / 4;

	auto idx2 = swapchain->GetCurrentBackBufferIndex();
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[idx2], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	cmdList->SetGraphicsRootSignature(rootPeraSignature.Get());
	cmdList->RSSetViewports(1, &view2);
	cmdList->RSSetScissorRects(1, &rec2);
	cmdList->IASetVertexBuffers(0, 1, &peraVbView);
	cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	cmdList->SetPipelineState(peraPipelineState.Get());
	auto pera2DescH = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	auto pera2HeapSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	pera2DescH.ptr += pera2HeapSize * idx2;
	float pera2Color[] = { 1.0f,0.5f,0.5f,1.f };
	cmdList->OMSetRenderTargets(1, &pera2DescH, false, &dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	//cmdList->ClearRenderTargetView(pera2DescH, pera2Color, 0, nullptr);
	auto rgbaHandle = rgbaSDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	auto inc = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cmdList->SetDescriptorHeaps(1, &rgbaSDescriptorHeap);
	rgbaHandle.ptr += inc;
	cmdList->SetGraphicsRootDescriptorTable(0, rgbaHandle);
	if (activeFlg) {
		cmdList->DrawInstanced(4, 1, 0, 0);
	}
	
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[idx2], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	cmdList->Close();

	ExecuteCmdList();
	Wait();

	// ペラ2の処理
	cmdAllocator->Reset();
	cmdList->Reset(cmdAllocator.Get(), peraPipelineState.Get());

	D3D12_VIEWPORT view3 = {};
	view3.TopLeftX = 0;
	view3.TopLeftY = (FLOAT)App::Instance().GetWidnowInfo().height / 4;
	view3.Width = (FLOAT)App::Instance().GetWidnowInfo().width / 4;
	view3.Height = (FLOAT)App::Instance().GetWidnowInfo().height / 4;
	view3.MinDepth = 0.0f;
	view3.MaxDepth = 1.0f;

	// Rectの設定
	RECT rec3 = {};
	rec3.left = 0;
	rec3.top = App::Instance().GetWidnowInfo().height / 4;
	rec3.right = App::Instance().GetWidnowInfo().width / 4;
	rec3.bottom = rec3.top + App::Instance().GetWidnowInfo().height / 4;

	auto idx3 = swapchain->GetCurrentBackBufferIndex();
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[idx3], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	cmdList->SetGraphicsRootSignature(rootPeraSignature.Get());
	cmdList->RSSetViewports(1, &view3);
	cmdList->RSSetScissorRects(1, &rec3);
	cmdList->IASetVertexBuffers(0, 1, &peraVbView);
	cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	cmdList->SetPipelineState(peraPipelineState.Get());
	auto pera3DescH = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	auto pera3HeapSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	pera3DescH.ptr += pera3HeapSize * idx3;
	float pera3Color[] = { 1.0f,0.5f,0.5f,1.f };
	cmdList->OMSetRenderTargets(1, &pera3DescH, false, &dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	//cmdList->ClearRenderTargetView(pera2DescH, pera2Color, 0, nullptr);
	auto rgbaHandle2 = rgbaSDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	auto inc2 = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cmdList->SetDescriptorHeaps(1, &rgbaSDescriptorHeap);
	rgbaHandle2.ptr += inc2 * 2;
	cmdList->SetGraphicsRootDescriptorTable(0, rgbaHandle2);
	if (activeFlg) {
		//cmdList->DrawInstanced(4, 1, 0, 0);
	}

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[idx3], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	cmdList->Close();

	ExecuteCmdList();
	Wait();
	swapchain->Present(0, 0);
}

void Dx12Initer::SetViewPorts()
{
	// viewPortの設定
	port.TopLeftX = 0;
	port.TopLeftY = 0;
	port.Width = (FLOAT)App::Instance().GetWidnowInfo().width;
	port.Height = (FLOAT)App::Instance().GetWidnowInfo().height;
	port.MinDepth = 0.0f;
	port.MaxDepth = 1.0f;

	// Rectの設定
	RECT scissorRect = {};
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = App::Instance().GetWidnowInfo().width;
	scissorRect.bottom = App::Instance().GetWidnowInfo().height;
	
	// コマンドリストにviewPortとRectのセット
	cmdList->RSSetViewports(1, &port);
	cmdList->RSSetScissorRects(1, &scissorRect);
}

void Dx12Initer::SetCmdList(D3D12_CPU_DESCRIPTOR_HANDLE handle[], const std::vector<FLOAT>& color)
{
	// ルートシグネチャーのセット
	cmdList->SetGraphicsRootSignature(rootSignature.Get());
	//デスクリプタヒープのセット（回転行列）
	cmdList->SetDescriptorHeaps(1, &rgstDescriptorHeap);
	// デスクリプターテーブルのセット
	cmdList->SetGraphicsRootDescriptorTable(0, rgstDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	// ボーン用デスクリプターヒープセット
	cmdList->SetDescriptorHeaps(1, &boneHeap);
	// ボーン用デスクリプターテーブルセット
	cmdList->SetGraphicsRootDescriptorTable(2, boneHeap->GetGPUDescriptorHandleForHeapStart());

	// 頂点バッファーのセット
	cmdList->IASetVertexBuffers(0, 1, &vbView);
	// PMXの頂点バッファセット
	//cmdList->IASetVertexBuffers(0, 1, &pmxVbView);
	// インデックスバッファのセット
	cmdList->IASetIndexBuffer(&ibView);
	// PMXのインデックスバッファセット
	//cmdList->IASetIndexBuffer(&pmxIbView);

	// トポロジーの設定
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// レンダーターゲット設定
	//cmdList->OMSetRenderTargets(2, handle, false, &dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	// レンダーターゲットクリア
	// デプスステンシルのクリア
	cmdList->ClearDepthStencilView(dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH,
		1.f, static_cast<UINT8>(0.f), static_cast<UINT>(0.f), nullptr);
	// パイプラインステートのセット
	cmdList->SetPipelineState(pipelineState.Get());
}

void Dx12Initer::ShaderSetting()
{
	// シェーダーコンパイル
	auto result = D3DCompileFromFile(L"VertexShader.hlsl", nullptr, nullptr, "vs", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vertexShader, nullptr);
	assert(result == S_OK);
	result = D3DCompileFromFile(L"VertexShader.hlsl", nullptr, nullptr, "ps", "ps_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pixelShader, nullptr);
	assert(result == S_OK);
	result = D3DCompileFromFile(L"VertexShader.hlsl", nullptr, nullptr, "shadowVs", "vs_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &shadowVsBlob, nullptr);
	assert(result == S_OK);

	// ペラポリ用シェーダコンパイル
	result = D3DCompileFromFile(L"pera.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "vs", "vs_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vertexPeraShader, nullptr);
	assert(result == S_OK);

	result = D3DCompileFromFile(L"pera.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "ps", "ps_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pixelPeraShader, nullptr);
	assert(result == S_OK);

	ID3DBlob* primError = nullptr;
	result = D3DCompileFromFile(L"Primitive.hlsl", nullptr, nullptr, "vs", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &primVsBlob, &primError);
	assert(result == S_OK);
	result = D3DCompileFromFile(L"Primitive.hlsl", nullptr, nullptr, "ps", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &primPsBlob, &primError);
	assert(result == S_OK);

	descTblRange.resize(5);
	rootparam.resize(4);

	// デスクリプターテーブルレンジの設定
	descTblRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descTblRange[0].BaseShaderRegister = 0;
	descTblRange[0].NumDescriptors = 1;
	descTblRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	descTblRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descTblRange[1].BaseShaderRegister = 1;
	descTblRange[1].NumDescriptors = 1;
	descTblRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	descTblRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descTblRange[2].BaseShaderRegister = 0;
	descTblRange[2].NumDescriptors = 4;
	descTblRange[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	descTblRange[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descTblRange[3].BaseShaderRegister = 2;
	descTblRange[3].NumDescriptors = 1;
	descTblRange[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	descTblRange[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descTblRange[4].BaseShaderRegister = 4;
	descTblRange[4].NumDescriptors = 1;
	descTblRange[4].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;


	// ルートパラメーターの設定
	rootparam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[0].DescriptorTable.NumDescriptorRanges = 1;
	rootparam[0].DescriptorTable.pDescriptorRanges = &descTblRange[0];
	rootparam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootparam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[1].DescriptorTable.NumDescriptorRanges = 2;
	rootparam[1].DescriptorTable.pDescriptorRanges = &descTblRange[1];
	rootparam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootparam[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[2].DescriptorTable.NumDescriptorRanges = 1;
	rootparam[2].DescriptorTable.pDescriptorRanges = &descTblRange[3];
	rootparam[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootparam[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[3].DescriptorTable.NumDescriptorRanges = 1;
	rootparam[3].DescriptorTable.pDescriptorRanges = &descTblRange[4];
	rootparam[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	descTblPeraRange.resize(2);
	peraRootparam.resize(2);

	descTblPeraRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descTblPeraRange[0].NumDescriptors = 3;
	descTblPeraRange[0].BaseShaderRegister = 0;
	descTblPeraRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	descTblPeraRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descTblPeraRange[1].NumDescriptors = 1;
	descTblPeraRange[1].BaseShaderRegister = 0;
	descTblPeraRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	/*descTblPeraRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descTblPeraRange[1].NumDescriptors = 1;
	descTblPeraRange[1].BaseShaderRegister = 1;
	descTblPeraRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;*/

	peraRootparam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	peraRootparam[0].DescriptorTable.NumDescriptorRanges = 1;
	peraRootparam[0].DescriptorTable.pDescriptorRanges = &descTblPeraRange[0];
	peraRootparam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	peraRootparam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	peraRootparam[1].DescriptorTable.NumDescriptorRanges = 1;
	peraRootparam[1].DescriptorTable.pDescriptorRanges = &descTblPeraRange[1];
	peraRootparam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	/*peraRootparam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	peraRootparam[1].DescriptorTable.NumDescriptorRanges = 1;
	peraRootparam[1].DescriptorTable.pDescriptorRanges = &descTblPeraRange[1];
	peraRootparam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;*/
	
	primDescTbl.resize(2);
	primRootParam.resize(2);

	primDescTbl[0].NumDescriptors = 1;
	primDescTbl[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	primDescTbl[0].BaseShaderRegister = 0;
	primDescTbl[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	primDescTbl[1].NumDescriptors = 1;
	primDescTbl[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	primDescTbl[1].BaseShaderRegister = 0;
	primDescTbl[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	primRootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	primRootParam[0].DescriptorTable.NumDescriptorRanges = 1;
	primRootParam[0].DescriptorTable.pDescriptorRanges = primDescTbl.data();
	primRootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	primRootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	primRootParam[1].DescriptorTable.NumDescriptorRanges = 1;
	primRootParam[1].DescriptorTable.pDescriptorRanges = &primDescTbl[1];
	primRootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	CreatePeraRootSignature(peraRootparam);
	CreatePeraPSO();
	CreatePrimRootSignature();
	CreatePrimPSO();
	CreateShadowRootSignature(rootparam);
	CreateShadowPSO();
}

void Dx12Initer::CreateRootSignature()
{
	// シグネチャー生成
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.NumParameters = static_cast<UINT>(rootparam.size());
	rsd.pParameters = rootparam.data();
	rsd.pStaticSamplers = &samplerDesc[0];
	rsd.NumStaticSamplers = 2;

	auto result = D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);

	// ルートシグネチャー生成
	result = dev->CreateRootSignature(0, signature->GetBufferPointer(),
		signature->GetBufferSize(),IID_PPV_ARGS(&rootSignature));
	assert(result == S_OK);
}
void Dx12Initer::CreatePSO()
{
	// 入力レイアウト設定
	D3D12_INPUT_ELEMENT_DESC ieDesc[] = {
	{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	{"BONENO",0,DXGI_FORMAT_R16G16_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	{"WEIGHT",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}};

	D3D12_RENDER_TARGET_BLEND_DESC rtbDesc;
	rtbDesc.BlendEnable = true;
	rtbDesc.LogicOpEnable = false;
	rtbDesc.SrcBlend = D3D12_BLEND_ONE;
	rtbDesc.DestBlend = D3D12_BLEND_ZERO;
	rtbDesc.BlendOp = D3D12_BLEND_OP_ADD;
	rtbDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	rtbDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	rtbDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	rtbDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	rtbDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	D3D12_BLEND_DESC blendDesc;
	blendDesc.AlphaToCoverageEnable = true;
	blendDesc.IndependentBlendEnable = false;
	for (int i = 0; i < _countof(blendDesc.RenderTarget); i++) {
		blendDesc.RenderTarget[i] = rtbDesc;
	}

	// パイプラインステート設定
	gpsDesc.pRootSignature = rootSignature.Get();
	gpsDesc.InputLayout.pInputElementDescs = ieDesc;
	gpsDesc.InputLayout.NumElements = _countof(ieDesc);
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);
	gpsDesc.NumRenderTargets = 3;
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gpsDesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gpsDesc.RTVFormats[2] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gpsDesc.DepthStencilState.DepthEnable = true;
	gpsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	gpsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	gpsDesc.DepthStencilState.StencilEnable = false;
	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	gpsDesc.BlendState = blendDesc;
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.SampleDesc.Quality = 0;
	gpsDesc.SampleMask = 0xffffff;
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	auto result = dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&pipelineState));
	assert(result == S_OK);
}

void Dx12Initer::CreatePeraRootSignature(const std::vector<D3D12_ROOT_PARAMETER>& par)
{
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.NumParameters = par.size();
	rsd.pParameters = par.data();

	D3D12_STATIC_SAMPLER_DESC peraSamplerDesc = {};
	peraSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	peraSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	peraSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	peraSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	peraSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	peraSamplerDesc.MinLOD = 0.f;
	peraSamplerDesc.MipLODBias = 0.f;
	peraSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	peraSamplerDesc.ShaderRegister = 0;
	peraSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	peraSamplerDesc.RegisterSpace = 0;
	peraSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

	rsd.pStaticSamplers = &peraSamplerDesc;
	rsd.NumStaticSamplers = 1;

	ID3DBlob* rootSignatureBlob = nullptr;
	ID3DBlob* error = nullptr;
	
	auto result = D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, &error);
	assert(result == S_OK);

	result = dev->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootPeraSignature));
	assert(result == S_OK);
}

void Dx12Initer::CreatePeraPSO()
{
	D3D12_INPUT_ELEMENT_DESC peraLayoutDescs[] = {
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(vertexPeraShader);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(pixelPeraShader);

	gpsDesc.DepthStencilState.DepthEnable = false;
	gpsDesc.DepthStencilState.StencilEnable = false;
	gpsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_NEVER;
	gpsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	gpsDesc.NumRenderTargets = 1;
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gpsDesc.pRootSignature = rootPeraSignature.Get();
	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.SampleDesc.Quality = 0;
	gpsDesc.SampleMask = UINT_MAX;
	gpsDesc.InputLayout.pInputElementDescs = peraLayoutDescs;
	gpsDesc.InputLayout.NumElements = _countof(peraLayoutDescs);
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	auto result = dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&peraPipelineState));
	assert(result == S_OK);

}

void Dx12Initer::CreateRMCBV()
{
	// DescriptorHeapの設定
	D3D12_DESCRIPTOR_HEAP_DESC rmCBVHeapDesc = {};
	rmCBVHeapDesc.NodeMask = 0;
	rmCBVHeapDesc.NumDescriptors = 1;
	rmCBVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	rmCBVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	// DescriptorHeap生成
	auto result = dev->CreateDescriptorHeap(&rmCBVHeapDesc, IID_PPV_ARGS(&rmMoveDescHeap));
	assert(result == S_OK);

	// リソースの割り当てに必要な情報を設定
	D3D12_HEAP_PROPERTIES rmCBVHeapProp = {};
	rmCBVHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	rmCBVHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	rmCBVHeapProp.CreationNodeMask = 1;
	rmCBVHeapProp.VisibleNodeMask = 1;
	rmCBVHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

	// リソースにセットするオブジェクトのサイズを取得
	auto size = (sizeof(RMMovementInfo) + 0xff) & ~0xff;

	// リソースとヒープの割り当て
	result = dev->CreateCommittedResource(&rmCBVHeapProp, D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&rmMoveResource));
	assert(result == S_OK);
	auto handle = rmMoveDescHeap->GetCPUDescriptorHandleForHeapStart();

	// CBV作成時に必要な物を設定
	rmCBVDesc.BufferLocation = rmMoveResource->GetGPUVirtualAddress();
	rmCBVDesc.SizeInBytes = (UINT)size;
	// レイマーチングオブジェクト移動用CBV作成
	dev->CreateConstantBufferView(&rmCBVDesc, handle);

	result = rmMoveResource->Map(0, nullptr, (void**)&movement);
	assert(result == S_OK);

	movement->pos = DirectX::XMFLOAT3(0, 0, 0);
}

void Dx12Initer::CreateShadowPSO()
{
	D3D12_INPUT_ELEMENT_DESC ieDesc[] = {
	{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	{"BONENO",0,DXGI_FORMAT_R16G16_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	{"WEIGHT",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0} };

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(shadowVsBlob);
	gpsDesc.DepthStencilState.DepthEnable = true;
	gpsDesc.DepthStencilState.StencilEnable = false;
	gpsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
	gpsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	gpsDesc.pRootSignature = shadowRootSignature.Get();

	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.SampleDesc.Quality = 0;
	gpsDesc.SampleMask = UINT_MAX;

	gpsDesc.InputLayout.pInputElementDescs = ieDesc;
	gpsDesc.InputLayout.NumElements = _countof(ieDesc);
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	auto result = dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&shadowPipelineState));
	assert(result == S_OK);
}

void Dx12Initer::CreateShadowRootSignature(const std::vector<D3D12_ROOT_PARAMETER> param)
{
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.NumParameters = param.size();
	rsd.pParameters = param.data();

	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.MinLOD = 0.f;
	samplerDesc.MipLODBias = 0.f;
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	samplerDesc.ShaderRegister = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	samplerDesc.RegisterSpace = 0;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

	rsd.pStaticSamplers = &samplerDesc;
	rsd.NumStaticSamplers = 1;

	ID3DBlob* rootSignatureBlob = nullptr;
	ID3DBlob* error = nullptr;
	auto result = D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, &error);
	assert(result == S_OK);
	result = dev->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&shadowRootSignature));
	assert(result == S_OK);
}

void Dx12Initer::CreateShadowDescHeap()
{
	App& app = App::Instance();
	auto wsize = app.GetWidnowInfo();
	auto size = RoundupPowerOf2(max(wsize.width, wsize.height));

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 1;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NodeMask = 0;
	auto result = dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&shadowDSV));
	assert(result == S_OK);

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	result = dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&shadowSRV));

	D3D12_HEAP_PROPERTIES heapProp = {};
	D3D12_RESOURCE_DESC resourceDesc = {};
	D3D12_CLEAR_VALUE clearValue = {};

	heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProp.CreationNodeMask = 0;
	heapProp.VisibleNodeMask = 0;

	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Width = size;
	resourceDesc.Height = size;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil.Depth = 1.f;
	clearValue.DepthStencil.Stencil = 0.f;

	result = dev->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, &clearValue,
		IID_PPV_ARGS(&shadowBuffer));
	assert(result == S_OK);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.Texture2D.MipSlice = 0;
	dev->CreateDepthStencilView(shadowBuffer.Get(), &dsvDesc, shadowDSV->GetCPUDescriptorHandleForHeapStart());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	dev->CreateShaderResourceView(shadowBuffer.Get(), &srvDesc, shadowSRV->GetCPUDescriptorHandleForHeapStart());
}

void Dx12Initer::CreateVertexBuffer(const DirectX::XMFLOAT3& pos, float width, float depth)
{
	PrimVertices = { {
		{PrimVertex(DirectX::XMFLOAT3(pos.x - width / 2,pos.y,pos.z - depth / 2),DirectX::XMFLOAT3(0,1,0),DirectX::XMFLOAT2(0,0))},
		{PrimVertex(DirectX::XMFLOAT3(pos.x - width / 2, pos.y,pos.z + depth / 2),DirectX::XMFLOAT3(0,1,0),DirectX::XMFLOAT2(0,1))},
		{PrimVertex(DirectX::XMFLOAT3(pos.x + width / 2, pos.y, pos.z - depth / 2),DirectX::XMFLOAT3(0,1,0),DirectX::XMFLOAT2(1,0))},
		{PrimVertex(DirectX::XMFLOAT3(pos.x + width / 2, pos.y, pos.z + depth / 2),DirectX::XMFLOAT3(0,1,0),DirectX::XMFLOAT2(1,1))}
	} };

	auto result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(PrimVertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&primVBuffer));
	assert(result == S_OK);

	primVbView.BufferLocation = primVBuffer->GetGPUVirtualAddress();
	primVbView.SizeInBytes = sizeof(PrimVertices);
	primVbView.StrideInBytes = sizeof(PrimVertex);

	D3D12_RANGE range = {};
	PrimVertex* primVertBuffPtr = nullptr;
	result = primVBuffer->Map(0, &range, (void**)& primVertBuffPtr);
	assert(result == S_OK);
	std::copy(PrimVertices.begin(), PrimVertices.end(), primVertBuffPtr);
	primVBuffer->Unmap(0, &range);
}

void Dx12Initer::CreatePrimPSO()
{
	D3D12_INPUT_ELEMENT_DESC primLayoutDesc[] = {
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"COLOR",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(primVsBlob);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(primPsBlob);
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
	gpsDesc.InputLayout.pInputElementDescs = primLayoutDesc;
	gpsDesc.InputLayout.NumElements = _countof(primLayoutDesc);
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	auto result = dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&primPipelineState));
	assert(result == S_OK);
}

void Dx12Initer::CreatePrimRootSignature()
{
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.NumParameters = primRootParam.size();
	rsd.pParameters = primRootParam.data();

	D3D12_STATIC_SAMPLER_DESC primSamplerDesc = {};
	primSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	primSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	primSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	primSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	primSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	primSamplerDesc.MinLOD = 0.f;
	primSamplerDesc.MipLODBias = 0.f;
	primSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	primSamplerDesc.ShaderRegister = 0;
	primSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	primSamplerDesc.RegisterSpace = 0;
	primSamplerDesc.MaxAnisotropy = 0;
	primSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

	rsd.pStaticSamplers = &primSamplerDesc;
	rsd.NumStaticSamplers = 1;

	ID3DBlob* rootSignatureBlob = nullptr;
	ID3DBlob* error = nullptr;

	auto result = D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, &error);
	assert(result == S_OK);
	result = dev->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&primRootSignature));
	assert(result == S_OK);
}

void Dx12Initer::CreatePrimConstantBuffer()
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
	wvp.lvp = DirectX::XMMatrixLookAtLH(lightPos, vtarget, vup) * DirectX::XMMatrixOrthographicLH(40, 40, 0.1f, 300.f);

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
	primMappedMatrix->lvp = wvp.lvp;
}

void Dx12Initer::ExecuteCmdList()
{
	// コマンドエクスキュート
	ID3D12CommandList* cmdLists[] = { cmdList.Get() };
	cmdQue->ExecuteCommandLists(1, cmdLists);
	++fenceValue;
	cmdQue->Signal(fence.Get(), fenceValue);
}

void Dx12Initer::Wait()
{
	// 待ち
	while (fence->GetCompletedValue() < fenceValue) {
		;
	}
}

void Dx12Initer::CreateSRVAndSetFormat(ID3D12Resource* resource, D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc, D3D12_CPU_DESCRIPTOR_HANDLE hdl)
{
	srvdesc.Format = resource->GetDesc().Format;
	dev->CreateShaderResourceView(resource, &srvdesc, hdl);
}

void Dx12Initer::RecursiveMatrixMultiply(BoneNode& node, DirectX::XMMATRIX& inmat)
{
	boneMats[node.boneIdx] *= inmat;
	for (auto& cnode : node.children) {

		RecursiveMatrixMultiply(*cnode,boneMats[node.boneIdx]);
	}
}

void Dx12Initer::PmxCreateVBuffer()
{
	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	auto pmxVertData = pmxManager.GetPMXVertices();
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = pmxVertData.size() * sizeof(VertexInfo);
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	auto result = dev->CreateCommittedResource(&heapProp,
		D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&pmxVertBuffer));
	assert(result == S_OK);

	// 頂点バッファのマップ
	VertexInfo* pmxVertMap = nullptr;
	result = pmxVertBuffer->Map(0, nullptr, (void**)& pmxVertMap);
	std::copy(pmxVertData.begin(), pmxVertData.end(), pmxVertMap);
	pmxVertBuffer->Unmap(0, nullptr);
	assert(result == S_OK);

	pmxVbView.BufferLocation = pmxVertBuffer->GetGPUVirtualAddress();
	pmxVbView.StrideInBytes = sizeof(VertexInfo);
	pmxVbView.SizeInBytes = pmxVertData.size() * sizeof(VertexInfo);
}

void Dx12Initer::PmxCreateIBuffer()
{
	auto pmxIdxDatas = pmxManager.GetPMXIndices();

	auto result = dev->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(pmxIdxDatas.size() * sizeof(unsigned short)),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pmxIndexBuffer));
	assert(result == S_OK);

	pmxIbView.BufferLocation = pmxIndexBuffer->GetGPUVirtualAddress();
	pmxIbView.Format = DXGI_FORMAT_R16_UINT;
	pmxIbView.SizeInBytes = (UINT)(pmxIdxDatas.size() * sizeof(unsigned short));

	unsigned short* pmxIdxMap = nullptr;
	result = pmxIndexBuffer->Map(0, nullptr, (void**)& pmxIdxMap);
	assert(result == S_OK);
	std::copy(pmxIdxDatas.begin(), pmxIdxDatas.end(), pmxIdxMap);
	pmxIndexBuffer->Unmap(0, nullptr);
}

void Dx12Initer::PmxCreateMaterial()
{
	int idx = 0;
	auto pmxMaterialBuffs = pmxManager.GetPMXMaterials();
	auto size = (sizeof(MaterialInfo) + 0xff) & ~0xff;
	PmxMaterialBuffers.resize(pmxMaterialBuffs.size());

	D3D12_HEAP_PROPERTIES pmxMatHeapProp = {};
	pmxMatHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	pmxMatHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	pmxMatHeapProp.CreationNodeMask = 1;
	pmxMatHeapProp.VisibleNodeMask = 1;
	pmxMatHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

	std::vector<MaterialInfo> pmxMatsInfo;
	pmxMatsInfo.resize(pmxMaterialBuffs.size());

	for (auto i = 0; i < pmxMaterialBuffs.size(); i++) {
		pmxMatsInfo[i].diffuse = pmxMaterialBuffs[i].diffuse;

		pmxMatsInfo[i].specular.x = pmxMaterialBuffs[i].specular.x;
		pmxMatsInfo[i].specular.y = pmxMaterialBuffs[i].specular.y;
		pmxMatsInfo[i].specular.z = pmxMaterialBuffs[i].specular.z;
		pmxMatsInfo[i].specular.w = pmxMaterialBuffs[i].specularVal;

		pmxMatsInfo[i].ambient.x = pmxMaterialBuffs[i].ambient.x;
		pmxMatsInfo[i].ambient.y = pmxMaterialBuffs[i].ambient.y;
		pmxMatsInfo[i].ambient.z = pmxMaterialBuffs[i].ambient.z;
	}

	for (auto& pmxMBuff : PmxMaterialBuffers) {
		auto result = dev->CreateCommittedResource(
			&pmxMatHeapProp, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(size), D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, IID_PPV_ARGS(&pmxMBuff));
		assert(result == S_OK);
		MaterialInfo* pmxMat = nullptr;
		result = pmxMBuff->Map(0, nullptr, (void**)& pmxMat);
		assert(result == S_OK);
		*pmxMat = pmxMatsInfo[idx];
		pmxMBuff->Unmap(0, nullptr);
		++idx;
	}

	D3D12_DESCRIPTOR_HEAP_DESC pmxMatHeapDesc = {};
	pmxMatHeapDesc.NodeMask = 0;
	pmxMatHeapDesc.NumDescriptors = pmxMaterialBuffs.size() * 2;
	pmxMatHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	pmxMatHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	auto result = dev->CreateDescriptorHeap(&pmxMatHeapDesc, IID_PPV_ARGS(&pmxDescHeap));
	assert(result == S_OK);

	idx = 0;
	auto handle = pmxDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto increment = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CONSTANT_BUFFER_VIEW_DESC pmxMatDesc = {};
	pmxMatDesc.SizeInBytes = size;
	for (auto& mat : pmxMaterialBuffs) {
		pmxMatDesc.BufferLocation = PmxMaterialBuffers[idx]->GetGPUVirtualAddress();
		dev->CreateConstantBufferView(&pmxMatDesc, handle);
		handle.ptr += increment;

		if (PmxTextureResources[idx] == nullptr) {
			auto whitetexture = texManager.CreateWhiteTex(dev);
			CreateSRVAndSetFormat(whitetexture, pmxSrvDesc, handle);
		}
		else {
			CreateSRVAndSetFormat(PmxTextureResources[idx].Get(), pmxSrvDesc, handle);
		}
		

		handle.ptr += increment;
		++idx;
	}
}

void Dx12Initer::CreatePmxTexBuffer()
{
	auto pmxMats = pmxManager.GetPMXMaterials();
	PmxTextureResources.resize(pmxMats.size());

	for (int i = 0; i < pmxMats.size(); i++) {
		std::string fileName = texManager.GetStringFromWstring(pmxManager.GetPMXTexturePath()[pmxMats[i].normalTextureIdx].texturePath);

		if (fileName.length() == 0) {
			PmxTextureResources[i] = nullptr;
		}

		/*auto texFilePath = fileCont.GetPMDTexturePath(pmxModelPath, fileName.c_str());
		PmxTextureResources[i] = texManager.LoadTextureFromFile(dev, texFilePath, fileCont.GetExtension((char*)fileName.c_str()));*/
		
		
	}

	pmxSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	pmxSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	pmxSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	pmxSrvDesc.Texture2D.MipLevels = 1;
}

void Dx12Initer::RotateBone(const char* bonename, const DirectX::XMFLOAT4& q, const DirectX::XMFLOAT3& location)
{
	auto boneit = boneNodes.find(bonename);
	if (boneit == boneNodes.end())return;
	auto& bonenode = boneit->second;
	auto loc = DirectX::XMLoadFloat3(&location);
	auto vec = DirectX::XMLoadFloat3(&bonenode.startPos);
	auto quaternioin = DirectX::XMLoadFloat4(&q);
	boneMats[bonenode.boneIdx] = DirectX::XMMatrixTranslationFromVector(DirectX::XMVectorScale(vec, -1)) *
		DirectX::XMMatrixRotationQuaternion(quaternioin) *
		DirectX::XMMatrixTranslationFromVector(vec) * DirectX::XMMatrixTranslationFromVector(loc);
}

void Dx12Initer::RotateBoneB(const char* bonename, const DirectX::XMFLOAT4& q, const DirectX::XMFLOAT4& q2, const DirectX::XMFLOAT3& location , const DirectX::XMFLOAT3& location2 , float t)
{
	auto boneit = boneNodes.find(bonename);
	if (boneit == boneNodes.end())return;
	auto& bonenode = boneit->second;
	auto loc = DirectX::XMLoadFloat3(&location);
	auto loc2 = DirectX::XMLoadFloat3(&location2);
	auto vec = DirectX::XMLoadFloat3(&bonenode.startPos);
	auto quaternion = DirectX::XMLoadFloat4(&q);
	auto quaternion2 = DirectX::XMLoadFloat4(&q2);
	boneMats[bonenode.boneIdx] = DirectX::XMMatrixTranslationFromVector(DirectX::XMVectorScale(vec, -1)) *
		DirectX::XMMatrixRotationQuaternion(DirectX::XMQuaternionSlerp(quaternion, quaternion2, t)) *
		DirectX::XMMatrixTranslationFromVector(vec) * DirectX::XMMatrixTranslationFromVector(DirectX::XMVectorLerp(loc,loc2,t));
}

void Dx12Initer::MotionUpdate(int frameno)
{
	for (auto& boneAnim : vmdLoader.GetKeyFrameData()) {
		auto& keyf = boneAnim.second;
		std::sort(keyf.begin(), keyf.end(), [](KeyFrame& a, KeyFrame& b) {return a.frameNo < b.frameNo; });
		auto frameIt = std::find_if(keyf.rbegin(), keyf.rend(), [frameno](const KeyFrame& k) {return k.frameNo <= frameno; });
		if (frameIt == keyf.rend())continue;
		auto nextIt = frameIt.base();
		if (nextIt == keyf.end()) {
			RotateBone(boneAnim.first.c_str(), frameIt->quaternion,frameIt->position);
		}
		else {
			float a = frameIt->frameNo;
			float b = nextIt->frameNo;
			float t = (static_cast<float>(frameno) - a) / (b - a);
			t = GetBezierYValueFromXWithBisection(t, nextIt->bz1, nextIt->bz2);
			RotateBoneB(boneAnim.first.c_str(), frameIt->quaternion, nextIt->quaternion,frameIt->position,nextIt->position, t);
		}
	}

	DirectX::XMMATRIX rootmat = DirectX::XMMatrixIdentity();
	RecursiveMatrixMultiply(boneNodes["センター"], rootmat);
}

float Dx12Initer::GetBezierYValueFromXWithBisection(float x, const DirectX::XMFLOAT2& a, const DirectX::XMFLOAT2& b, const unsigned int limit)
{
	if (a.x == a.y && b.x == b.y)return x;
	float t = x;
	float k0 = 1 + 3 * a.x - 3 * b.x;
	float k1 = 3 * b.x - 6 * a.x;
	float k2 = 3 * a.x;

	const float epsilon = 0.0005f;

	for (int i = 0; i < limit; i++) {
		float r = (1 - t);
		float ft = (t * t * t) * k0 + t * t * k1 + t * k2 - x;
		if (ft <= epsilon && ft >= -epsilon)break;
		float fdt = (3 * t * t * k0 + 2 * t * k1 + k2);
		if (fdt == 0)break;
		t = t - ft / fdt;
	}
	float r = (1 - t);
	return 3 * r * r * t * a.y + 3 * r * t * t * b.y + t * t * t;
}
