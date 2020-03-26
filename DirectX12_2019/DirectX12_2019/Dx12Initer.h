#pragma once
#include <DirectXMath.h>
#include <DirectXTex.h>
#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <vector>
#include <wrl.h>
#include <map>
#include <List>
#include <array>
#include "PMDManager.h"
#include "PMXManager.h"
#include "TextureManager.h"
#include "FileNaneController.h"
#include "VMDLoader.h"

using Microsoft::WRL::ComPtr;

class PrimitiveMesh;

// 頂点情報構造体
struct Vertex {
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT2 uv;
	//DirectX::XMFLOAT3 normal;
};

struct PrimVertex {
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 uv;

	PrimVertex() {
		pos = DirectX::XMFLOAT3(0, 0, 0);
		normal = DirectX::XMFLOAT3(0, 0, 0);
		uv = DirectX::XMFLOAT2(0, 0);
	}

	PrimVertex(DirectX::XMFLOAT3 p, DirectX::XMFLOAT3 n, DirectX::XMFLOAT2 u) {
		pos = p;
		normal = n;
		uv = u;
	}

	PrimVertex(float x, float y, float z, float nx, float ny, float nz, float u, float v) {
		pos.x = x;
		pos.y = y;
		pos.z = z;
		normal.x = nx;
		normal.y = ny;
		normal.z = nz;
		uv.x = u;
		uv.y = v;
	}
};

// WVP情報構造体
struct WVP {
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX proj;
	DirectX::XMMATRIX wvp;
	DirectX::XMMATRIX lvp;
	DirectX::XMFLOAT3 eye;
	DirectX::XMMATRIX shadowMat;
	DirectX::XMMATRIX instancePos[1000];
};

struct PrimWVP {
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX viewProj;
	DirectX::XMMATRIX lvp;
};

// ツリー構築用ボーンノード情報
struct BoneNode {
	int boneIdx;
	DirectX::XMFLOAT3 startPos;
	DirectX::XMFLOAT3 endPos;
	std::vector<BoneNode*> children;
};

// マテリアル情報構造体
struct MaterialInfo {
	DirectX::XMFLOAT4 diffuse;
	DirectX::XMFLOAT4 specular;
	DirectX::XMFLOAT4 ambient;
};

struct RMMovementInfo {
	DirectX::XMFLOAT3 pos;
};

class Dx12Initer
{
public:
	Dx12Initer();
	~Dx12Initer();
	// 初期化関数
	void Initialize();
	// ループ
	void Runnable();

	// フォント関係
	D3D12_VIEWPORT GetViewPort()const;
	ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeapForSpriteFont();
	ComPtr<ID3D12Device> GetDevice() {
		return dev;
	}
	ComPtr<ID3D12CommandQueue> GetCmdQue() {
		return cmdQue;
	}
	ComPtr<ID3D12GraphicsCommandList> GetCmdList() {
		return cmdList;
	}
	// 待機関数
	void Wait();
private:
	// 読み込むモデルのパス
	//std::string modelPath = "model/Model/巡音ルカ.pmd";
	std::string modelPath = "model/miku/初音ミク.pmd";
	//std::string pmxModelPath = "model/物述有栖/物述有栖/物述有栖.pmx";
	std::string vmdPath = "モーション/ヤゴコロダンス.vmd";

	// デバイス変数
	ComPtr<ID3D12Device> dev = nullptr;

	// infoQueue変数
	ComPtr<ID3D12InfoQueue> infoQueue;

	// コマンド関係
	// コマンドアロケーター
	ComPtr<ID3D12CommandAllocator> cmdAllocator = nullptr;
	// コマンドリスト
	ComPtr<ID3D12GraphicsCommandList> cmdList = nullptr;
	// コマンドキュー
	ComPtr<ID3D12CommandQueue> cmdQue = nullptr;
	// 基本的なDXGI関係
	ComPtr<IDXGIFactory6> dxgi = nullptr;
	IDXGISwapChain4* swapchain = nullptr;
	// フェンス
	ComPtr<ID3D12Fence> fence = nullptr;
	UINT64 fenceValue = 0;
	// フィーチャーレベル
	D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_12_1;
	// スワップチェインデスク
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc;
	// デスクリプターヒープ用ポインタ変数
	ID3D12DescriptorHeap* rtvDescriptorHeap = nullptr;
	ID3D12DescriptorHeap* dsvDescriptorHeap = nullptr;
	ID3D12DescriptorHeap* rgstDescriptorHeap = nullptr;
	ID3D12DescriptorHeap* matDescriptorHeap = nullptr;
	ID3D12DescriptorHeap* rgbaRDescriptorHeap = nullptr;
	ID3D12DescriptorHeap* rgbaSDescriptorHeap = nullptr;
	// レンダーターゲット用配列
	std::vector<ID3D12Resource*> renderTargets;
	// ヒープサイズ用変数
	UINT heapSize = 0;
	// リソースバリア用変数
	D3D12_RESOURCE_BARRIER BarrierDesc = {};
	// デスクリプタハンドル用変数
	D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = {};
	// リソース配列
	ComPtr<ID3D12Resource> vertexBuffer = nullptr;
	ComPtr<ID3D12Resource> vertexPeraBuffer;
	ID3D12Resource* indexBuffer = nullptr;
	ComPtr<ID3D12Resource> textureBuffer = nullptr;

	// 頂点バッファービュー
	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	D3D12_VERTEX_BUFFER_VIEW peraVbView = {};
	// インデックスバッファービュー
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	// コンスタントバッファービュー
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	// マテリアルバッファービュー
	D3D12_CONSTANT_BUFFER_VIEW_DESC matDesc = {};
	// シェーダーコンパイル時使用変数
	ID3DBlob* vertexShader = nullptr;
	ID3DBlob* pixelShader = nullptr;

	ID3DBlob* vertexPeraShader = nullptr;
	ID3DBlob* pixelPeraShader = nullptr;

	ID3DBlob* primVsBlob = nullptr;
	ID3DBlob* primPsBlob = nullptr;

	// ルートシグネチャー用ポインタ変数
	ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	ComPtr<ID3D12RootSignature> rootPeraSignature = nullptr;
	// シグネチャ用変数
	ID3DBlob* signature = nullptr;
	// エラー情報格納用変数
	ID3DBlob* error = nullptr;
	// パイプラインステート情報設定用変数
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
	// パイプラインステート変数
	ComPtr<ID3D12PipelineState> pipelineState = nullptr;
	ComPtr<ID3D12PipelineState> peraPipelineState = nullptr;

	// サンプラー情報設定用構造体配列
	D3D12_STATIC_SAMPLER_DESC samplerDesc[2] = {};
	// レンジ
	std::vector<D3D12_DESCRIPTOR_RANGE> descTblRange;
	// ルートパラメーター
	std::vector<D3D12_ROOT_PARAMETER> rootparam;

	// ペラレンジ
	std::vector<D3D12_DESCRIPTOR_RANGE> descTblPeraRange;
	// ペラルートパラメーター
	std::vector<D3D12_ROOT_PARAMETER> peraRootparam;

	// 回転情報転送用配列
	ComPtr<ID3D12Resource> cbvMat;
	std::shared_ptr<WVP> matrix;
	// 各クラス変数
	PMDManager pmdManager;
	PMXManager pmxManager;
	TextureManager texManager;
	FileNaneController fileCont;
	VMDLoader vmdLoader;
	// デプス用バッファー
	ComPtr<ID3D12Resource> depthBuffer = nullptr;
	
	// PMDリソース情報用配列
	std::vector<ComPtr<ID3D12Resource>> materialBuffer;
	std::vector<ComPtr<ID3D12Resource>> textureResource;
	std::vector<ComPtr<ID3D12Resource>> sphResource;
	std::vector<ComPtr<ID3D12Resource>> spaResource;
	std::vector<ComPtr<ID3D12Resource>> toonResource;
	std::vector<ComPtr<ID3D12Resource>> rgbaResource;
	// PMDテクスチャ用SRVDesc
	D3D12_SHADER_RESOURCE_VIEW_DESC texSrvDesc = {};
	// 回転度数用変数
	float angle = 0.f;

	// ボーン関係変数
	// ボーン行列転送用配列
	std::vector<DirectX::XMMATRIX> boneMats;
	// ボーンマップ
	std::map<std::string, BoneNode> boneNodes;
	// ボーンバッファ
	ComPtr<ID3D12Resource> boneBuffer;
	ID3D12DescriptorHeap* boneHeap;
	DirectX::XMMATRIX* mappedMat;
	int frame = 0;

	// PMX用
	ComPtr<ID3D12Resource> pmxVertBuffer = nullptr;
	ComPtr<ID3D12Resource> pmxIndexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW pmxVbView = {};
	D3D12_INDEX_BUFFER_VIEW pmxIbView = {};
	std::vector<ComPtr<ID3D12Resource>> PmxMaterialBuffers;
	ID3D12DescriptorHeap* pmxDescHeap = nullptr;
	std::vector<ComPtr<ID3D12Resource>> PmxTextureResources;
	D3D12_SHADER_RESOURCE_VIEW_DESC pmxSrvDesc = {};
	std::vector<std::string> LoadedTexture;

	// 影表示用
	ComPtr<ID3D12PipelineState> shadowPipelineState = nullptr;
	ComPtr<ID3D12RootSignature> shadowRootSignature = nullptr;
	ComPtr<ID3D12Resource> shadowBuffer = nullptr;
	ID3D12DescriptorHeap* shadowDSV = nullptr;
	ID3D12DescriptorHeap* shadowSRV = nullptr;
	ID3DBlob* shadowVsBlob = nullptr;

	// 床表示用
	//PrimitiveMesh* primMesh;
	ComPtr<ID3D12Resource> primVBuffer;
	D3D12_VERTEX_BUFFER_VIEW primVbView;
	ID3D12DescriptorHeap* primConstantHeap = nullptr;
	ComPtr<ID3D12Resource> primCBuffer;
	PrimWVP* primMappedMatrix = nullptr;
	std::array<PrimVertex, 4> PrimVertices;

	ComPtr<ID3D12RootSignature> primRootSignature;
	ComPtr<ID3D12PipelineState> primPipelineState;
	std::vector<D3D12_DESCRIPTOR_RANGE> primDescTbl;
	std::vector<D3D12_ROOT_PARAMETER> primRootParam;

	DirectX::XMFLOAT3 camera;
	DirectX::XMFLOAT3 target;
	DirectX::XMFLOAT3 up;
	DirectX::XMFLOAT3 worldAngle;

	unsigned char* vertMap = nullptr;

	// Imgui初期化
	int value = 2;
	int minValue = 1;
	int maxValue = 1000;
	bool activeFlg = true;
	bool activeFloorFlg = true;
	bool activeModelFlg = true;
	std::vector<float> backColor;
	ID3D12DescriptorHeap* ImguiDescriptorHeap = nullptr;
	// Imgui用関数
	void CreateImguiSRV();
	void InitializeImgui(HWND hwnd, ID3D12Device* device, DXGI_FORMAT formatRtv, UINT buffCnt,ID3D12DescriptorHeap* heaPSrv, 
		D3D12_CPU_DESCRIPTOR_HANDLE hCpu, D3D12_GPU_DESCRIPTOR_HANDLE hGpu);
	void TerminateImgui();

	// ブルーム表示用

	//フォント用
	D3D12_VIEWPORT port = {};
	ID3D12DescriptorHeap* spriteFontHeap = nullptr;

	// レイマーチング移動用バッファー等
	std::shared_ptr<RMMovementInfo> movement = nullptr;
	ID3D12DescriptorHeap* rmMoveDescHeap = nullptr;
	ComPtr<ID3D12Resource> rmMoveResource = nullptr;
	D3D12_CONSTANT_BUFFER_VIEW_DESC rmCBVDesc = {};

	// 頂点、インデックスバッファ生成
	void CreateVBuffer();
	void CreateIBuffer();
	// デバイス生成
	void CreateDevice();
	// ファクトリー生成
	void CreateFactory();
	// スワップチェイン生成
	void CreateSwapChain();
	// コマンドキュー生成
	void CreateCommandQueue();
	// RTV用デスクリプターヒープ生成
	void CreateDH();
	// RTV生成
	void CreateRTV();
	// コマンド（Queue以外）の生成
	void CreateCommand();
	// CBV生成
	void CreateCBV();
	// DSV生成
	void CreateDSV();
	// マテリアル用バッファー生成
	void CreateMatBuff();
	// マテリアル用ビュー生成
	void CreateMatView();
	// PMDテクスチャバッファー生成
	void CreatePMDTexBuff();
	// ツリー設定
	void CreateBoneTree();
	// ボーン用コンスタントバッファ作成
	void CreateBoneCBV();
	// サンプラー生成
	void CreateSampler();
	// フェンス作成
	void CreateFence();
	void ThrowCmd();
	// ビューポート設定
	void SetViewPorts();
	// コマンドリスト設定
	void SetCmdList(D3D12_CPU_DESCRIPTOR_HANDLE handle[], const std::vector<FLOAT>& color);
	// シェーダー設定用関数
	void ShaderSetting();
	// ルートシグネチャ生成
	void CreateRootSignature();
	// パイプラインステートオブジェクト生成
	void CreatePSO();
	// コマンド実行用関数
	void ExecuteCmdList();
	

	// ペラポリ用
	// 最初のパスに使用するビューの作成
	void Create1stPathView();
	void Create1stResource();
	// 頂点バッファー生成
	void CreatePeraVBuffer();
	// ペラルートシグネチャ生成
	void CreatePeraRootSignature(const std::vector<D3D12_ROOT_PARAMETER>& par);
	// ペラパイプラインステートオブジェクト生成
	void CreatePeraPSO();
	void CreateRMCBV();

	// 影用関数
	void CreateShadowPSO();
	void CreateShadowRootSignature(const std::vector<D3D12_ROOT_PARAMETER> param);
	void CreateShadowDescHeap();

	// 床用関数
	void CreateVertexBuffer(const DirectX::XMFLOAT3& pos, float width, float depth);
	void CreatePrimPSO();
	void CreatePrimRootSignature();
	void CreatePrimConstantBuffer();

	// SRVの作成及びフォーマットの設定
	void CreateSRVAndSetFormat(ID3D12Resource* resource, D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc, D3D12_CPU_DESCRIPTOR_HANDLE hdl);
	// 回転情報を子に伝える
	void RecursiveMatrixMultiply(BoneNode& node, DirectX::XMMATRIX& inmat);

	void PmxCreateVBuffer();
	void PmxCreateIBuffer();
	void PmxCreateMaterial();
	void CreatePmxTexBuffer();
	void RotateBone(const char* bonename, const DirectX::XMFLOAT4& q, const DirectX::XMFLOAT3& location);
	void RotateBoneB(const char* bonename, const DirectX::XMFLOAT4& q, const DirectX::XMFLOAT4& q2, const DirectX::XMFLOAT3& location, const DirectX::XMFLOAT3& location2, float t);
	void MotionUpdate(int frameno);
	float GetBezierYValueFromXWithBisection(float x, const DirectX::XMFLOAT2& a, const DirectX::XMFLOAT2& b, const unsigned int limit = 16U);
};

