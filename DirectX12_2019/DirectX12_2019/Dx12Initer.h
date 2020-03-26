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

// ���_���\����
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

// WVP���\����
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

// �c���[�\�z�p�{�[���m�[�h���
struct BoneNode {
	int boneIdx;
	DirectX::XMFLOAT3 startPos;
	DirectX::XMFLOAT3 endPos;
	std::vector<BoneNode*> children;
};

// �}�e���A�����\����
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
	// �������֐�
	void Initialize();
	// ���[�v
	void Runnable();

	// �t�H���g�֌W
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
	// �ҋ@�֐�
	void Wait();
private:
	// �ǂݍ��ރ��f���̃p�X
	//std::string modelPath = "model/Model/�������J.pmd";
	std::string modelPath = "model/miku/�����~�N.pmd";
	//std::string pmxModelPath = "model/���q�L��/���q�L��/���q�L��.pmx";
	std::string vmdPath = "���[�V����/���S�R���_���X.vmd";

	// �f�o�C�X�ϐ�
	ComPtr<ID3D12Device> dev = nullptr;

	// infoQueue�ϐ�
	ComPtr<ID3D12InfoQueue> infoQueue;

	// �R�}���h�֌W
	// �R�}���h�A���P�[�^�[
	ComPtr<ID3D12CommandAllocator> cmdAllocator = nullptr;
	// �R�}���h���X�g
	ComPtr<ID3D12GraphicsCommandList> cmdList = nullptr;
	// �R�}���h�L���[
	ComPtr<ID3D12CommandQueue> cmdQue = nullptr;
	// ��{�I��DXGI�֌W
	ComPtr<IDXGIFactory6> dxgi = nullptr;
	IDXGISwapChain4* swapchain = nullptr;
	// �t�F���X
	ComPtr<ID3D12Fence> fence = nullptr;
	UINT64 fenceValue = 0;
	// �t�B�[�`���[���x��
	D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_12_1;
	// �X���b�v�`�F�C���f�X�N
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc;
	// �f�X�N���v�^�[�q�[�v�p�|�C���^�ϐ�
	ID3D12DescriptorHeap* rtvDescriptorHeap = nullptr;
	ID3D12DescriptorHeap* dsvDescriptorHeap = nullptr;
	ID3D12DescriptorHeap* rgstDescriptorHeap = nullptr;
	ID3D12DescriptorHeap* matDescriptorHeap = nullptr;
	ID3D12DescriptorHeap* rgbaRDescriptorHeap = nullptr;
	ID3D12DescriptorHeap* rgbaSDescriptorHeap = nullptr;
	// �����_�[�^�[�Q�b�g�p�z��
	std::vector<ID3D12Resource*> renderTargets;
	// �q�[�v�T�C�Y�p�ϐ�
	UINT heapSize = 0;
	// ���\�[�X�o���A�p�ϐ�
	D3D12_RESOURCE_BARRIER BarrierDesc = {};
	// �f�X�N���v�^�n���h���p�ϐ�
	D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = {};
	// ���\�[�X�z��
	ComPtr<ID3D12Resource> vertexBuffer = nullptr;
	ComPtr<ID3D12Resource> vertexPeraBuffer;
	ID3D12Resource* indexBuffer = nullptr;
	ComPtr<ID3D12Resource> textureBuffer = nullptr;

	// ���_�o�b�t�@�[�r���[
	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	D3D12_VERTEX_BUFFER_VIEW peraVbView = {};
	// �C���f�b�N�X�o�b�t�@�[�r���[
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	// �R���X�^���g�o�b�t�@�[�r���[
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	// �}�e���A���o�b�t�@�[�r���[
	D3D12_CONSTANT_BUFFER_VIEW_DESC matDesc = {};
	// �V�F�[�_�[�R���p�C�����g�p�ϐ�
	ID3DBlob* vertexShader = nullptr;
	ID3DBlob* pixelShader = nullptr;

	ID3DBlob* vertexPeraShader = nullptr;
	ID3DBlob* pixelPeraShader = nullptr;

	ID3DBlob* primVsBlob = nullptr;
	ID3DBlob* primPsBlob = nullptr;

	// ���[�g�V�O�l�`���[�p�|�C���^�ϐ�
	ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	ComPtr<ID3D12RootSignature> rootPeraSignature = nullptr;
	// �V�O�l�`���p�ϐ�
	ID3DBlob* signature = nullptr;
	// �G���[���i�[�p�ϐ�
	ID3DBlob* error = nullptr;
	// �p�C�v���C���X�e�[�g���ݒ�p�ϐ�
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
	// �p�C�v���C���X�e�[�g�ϐ�
	ComPtr<ID3D12PipelineState> pipelineState = nullptr;
	ComPtr<ID3D12PipelineState> peraPipelineState = nullptr;

	// �T���v���[���ݒ�p�\���̔z��
	D3D12_STATIC_SAMPLER_DESC samplerDesc[2] = {};
	// �����W
	std::vector<D3D12_DESCRIPTOR_RANGE> descTblRange;
	// ���[�g�p�����[�^�[
	std::vector<D3D12_ROOT_PARAMETER> rootparam;

	// �y�������W
	std::vector<D3D12_DESCRIPTOR_RANGE> descTblPeraRange;
	// �y�����[�g�p�����[�^�[
	std::vector<D3D12_ROOT_PARAMETER> peraRootparam;

	// ��]���]���p�z��
	ComPtr<ID3D12Resource> cbvMat;
	std::shared_ptr<WVP> matrix;
	// �e�N���X�ϐ�
	PMDManager pmdManager;
	PMXManager pmxManager;
	TextureManager texManager;
	FileNaneController fileCont;
	VMDLoader vmdLoader;
	// �f�v�X�p�o�b�t�@�[
	ComPtr<ID3D12Resource> depthBuffer = nullptr;
	
	// PMD���\�[�X���p�z��
	std::vector<ComPtr<ID3D12Resource>> materialBuffer;
	std::vector<ComPtr<ID3D12Resource>> textureResource;
	std::vector<ComPtr<ID3D12Resource>> sphResource;
	std::vector<ComPtr<ID3D12Resource>> spaResource;
	std::vector<ComPtr<ID3D12Resource>> toonResource;
	std::vector<ComPtr<ID3D12Resource>> rgbaResource;
	// PMD�e�N�X�`���pSRVDesc
	D3D12_SHADER_RESOURCE_VIEW_DESC texSrvDesc = {};
	// ��]�x���p�ϐ�
	float angle = 0.f;

	// �{�[���֌W�ϐ�
	// �{�[���s��]���p�z��
	std::vector<DirectX::XMMATRIX> boneMats;
	// �{�[���}�b�v
	std::map<std::string, BoneNode> boneNodes;
	// �{�[���o�b�t�@
	ComPtr<ID3D12Resource> boneBuffer;
	ID3D12DescriptorHeap* boneHeap;
	DirectX::XMMATRIX* mappedMat;
	int frame = 0;

	// PMX�p
	ComPtr<ID3D12Resource> pmxVertBuffer = nullptr;
	ComPtr<ID3D12Resource> pmxIndexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW pmxVbView = {};
	D3D12_INDEX_BUFFER_VIEW pmxIbView = {};
	std::vector<ComPtr<ID3D12Resource>> PmxMaterialBuffers;
	ID3D12DescriptorHeap* pmxDescHeap = nullptr;
	std::vector<ComPtr<ID3D12Resource>> PmxTextureResources;
	D3D12_SHADER_RESOURCE_VIEW_DESC pmxSrvDesc = {};
	std::vector<std::string> LoadedTexture;

	// �e�\���p
	ComPtr<ID3D12PipelineState> shadowPipelineState = nullptr;
	ComPtr<ID3D12RootSignature> shadowRootSignature = nullptr;
	ComPtr<ID3D12Resource> shadowBuffer = nullptr;
	ID3D12DescriptorHeap* shadowDSV = nullptr;
	ID3D12DescriptorHeap* shadowSRV = nullptr;
	ID3DBlob* shadowVsBlob = nullptr;

	// ���\���p
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

	// Imgui������
	int value = 2;
	int minValue = 1;
	int maxValue = 1000;
	bool activeFlg = true;
	bool activeFloorFlg = true;
	bool activeModelFlg = true;
	std::vector<float> backColor;
	ID3D12DescriptorHeap* ImguiDescriptorHeap = nullptr;
	// Imgui�p�֐�
	void CreateImguiSRV();
	void InitializeImgui(HWND hwnd, ID3D12Device* device, DXGI_FORMAT formatRtv, UINT buffCnt,ID3D12DescriptorHeap* heaPSrv, 
		D3D12_CPU_DESCRIPTOR_HANDLE hCpu, D3D12_GPU_DESCRIPTOR_HANDLE hGpu);
	void TerminateImgui();

	// �u���[���\���p

	//�t�H���g�p
	D3D12_VIEWPORT port = {};
	ID3D12DescriptorHeap* spriteFontHeap = nullptr;

	// ���C�}�[�`���O�ړ��p�o�b�t�@�[��
	std::shared_ptr<RMMovementInfo> movement = nullptr;
	ID3D12DescriptorHeap* rmMoveDescHeap = nullptr;
	ComPtr<ID3D12Resource> rmMoveResource = nullptr;
	D3D12_CONSTANT_BUFFER_VIEW_DESC rmCBVDesc = {};

	// ���_�A�C���f�b�N�X�o�b�t�@����
	void CreateVBuffer();
	void CreateIBuffer();
	// �f�o�C�X����
	void CreateDevice();
	// �t�@�N�g���[����
	void CreateFactory();
	// �X���b�v�`�F�C������
	void CreateSwapChain();
	// �R�}���h�L���[����
	void CreateCommandQueue();
	// RTV�p�f�X�N���v�^�[�q�[�v����
	void CreateDH();
	// RTV����
	void CreateRTV();
	// �R�}���h�iQueue�ȊO�j�̐���
	void CreateCommand();
	// CBV����
	void CreateCBV();
	// DSV����
	void CreateDSV();
	// �}�e���A���p�o�b�t�@�[����
	void CreateMatBuff();
	// �}�e���A���p�r���[����
	void CreateMatView();
	// PMD�e�N�X�`���o�b�t�@�[����
	void CreatePMDTexBuff();
	// �c���[�ݒ�
	void CreateBoneTree();
	// �{�[���p�R���X�^���g�o�b�t�@�쐬
	void CreateBoneCBV();
	// �T���v���[����
	void CreateSampler();
	// �t�F���X�쐬
	void CreateFence();
	void ThrowCmd();
	// �r���[�|�[�g�ݒ�
	void SetViewPorts();
	// �R�}���h���X�g�ݒ�
	void SetCmdList(D3D12_CPU_DESCRIPTOR_HANDLE handle[], const std::vector<FLOAT>& color);
	// �V�F�[�_�[�ݒ�p�֐�
	void ShaderSetting();
	// ���[�g�V�O�l�`������
	void CreateRootSignature();
	// �p�C�v���C���X�e�[�g�I�u�W�F�N�g����
	void CreatePSO();
	// �R�}���h���s�p�֐�
	void ExecuteCmdList();
	

	// �y���|���p
	// �ŏ��̃p�X�Ɏg�p����r���[�̍쐬
	void Create1stPathView();
	void Create1stResource();
	// ���_�o�b�t�@�[����
	void CreatePeraVBuffer();
	// �y�����[�g�V�O�l�`������
	void CreatePeraRootSignature(const std::vector<D3D12_ROOT_PARAMETER>& par);
	// �y���p�C�v���C���X�e�[�g�I�u�W�F�N�g����
	void CreatePeraPSO();
	void CreateRMCBV();

	// �e�p�֐�
	void CreateShadowPSO();
	void CreateShadowRootSignature(const std::vector<D3D12_ROOT_PARAMETER> param);
	void CreateShadowDescHeap();

	// ���p�֐�
	void CreateVertexBuffer(const DirectX::XMFLOAT3& pos, float width, float depth);
	void CreatePrimPSO();
	void CreatePrimRootSignature();
	void CreatePrimConstantBuffer();

	// SRV�̍쐬�y�уt�H�[�}�b�g�̐ݒ�
	void CreateSRVAndSetFormat(ID3D12Resource* resource, D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc, D3D12_CPU_DESCRIPTOR_HANDLE hdl);
	// ��]�����q�ɓ`����
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

