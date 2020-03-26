#pragma once
#include <vector>
#include <string>
#include <windows.h>
#include <DirectXMath.h>

struct PMXHeader {
	char magic[4];
	float version;
	char dataSize;
	std::vector<char> sizeData;
};

struct PMXInfo {
	int byteSizeM;
	std::wstring modelName;
	int byteSizeME;
	std::wstring modelNameE;
	int byteSizeC;
	std::wstring comment;
	int byteSizeCE;
	std::wstring commentE;
};

struct VertexInfo {
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 uv;
	std::vector<DirectX::XMFLOAT4> addUV;
	char weightType;
	std::vector<char> boneRefIdx;
	std::vector<float> boneWeight;
	std::vector<DirectX::XMFLOAT3> sdefValue;
	float edgeDiameter;
};

struct TexturePathInfo {
	int texturePathSize;
	std::wstring texturePath;
};

struct PMXMaterialInfo {
	int matNameSize;
	std::wstring matName;
	int matNameSizeE;
	std::wstring matNameE;
	DirectX::XMFLOAT4 diffuse;
	DirectX::XMFLOAT3 specular;
	float specularVal;
	DirectX::XMFLOAT3 ambient;
	unsigned char drawFlag;
	DirectX::XMFLOAT4 edgeColor;
	float edgeSize;
	int normalTextureIdx;
	int spTextureIdx;
	unsigned char sphereMode;
	unsigned char shereToonFlg;
	int toonTexRefIdx;
	unsigned char shereToonTexRefIdx;
	int memoSize;
	std::wstring memo;
	int matSurfaceCnt;
};

struct IKLinkInfo {
	int linkBoneIdx;
	char limitedAngle;
	DirectX::XMFLOAT3 underLimitAngle;
	DirectX::XMFLOAT3 upperLimitAngle;
};

struct BoneInfo {
	int boneNameSize;
	std::wstring boneName;
	int boneNameSizeE;
	std::wstring boneNameE;
	DirectX::XMFLOAT3 position;
	int parentBoneIdx;
	int transformHierarchy;
	short boneFlag;
	DirectX::XMFLOAT3 posOffset;
	int connectBoneIdx;
	int grantParentBoneIdx;		// �t�^�e�{�[���̃{�[��Index
	float grantValue;			// �t�^��
	DirectX::XMFLOAT3 axisDirVec;
	DirectX::XMFLOAT3 DirVecXAxis;		// X���̕����x�N�g��
	DirectX::XMFLOAT3 DirVecZAxis;		// Z���̕����x�N�g��
	int keyValue;
	int IKTargetBoneIdx;
	int IKRoopCnt;
	float IKLimAngle;
	int IKLinkCnt;
	std::vector<IKLinkInfo> IKLinkInfos;
	bool rotationFlag;
	bool moveFlag;
	bool drawFlag;
	bool controlFlag;
};

struct VertecMorph {
	unsigned int VertIndex;
	DirectX::XMFLOAT3 posOffset;
};

struct UVMorph {
	unsigned int VertIndex;
	DirectX::XMFLOAT4 uvOffset;
};

struct BoneMorph {
	unsigned int boneIdx;
	DirectX::XMFLOAT3 movementVal;
	DirectX::XMFLOAT4 rotationVal;
};

struct MaterialMorph {
	unsigned int materialIdx;
	unsigned char offsetCalculation;
	DirectX::XMFLOAT4 diffuse;
	DirectX::XMFLOAT3 specular;
	float specularVal;
	DirectX::XMFLOAT3 ambient;
	DirectX::XMFLOAT4 edgeColor;
	float edgeSize;
	DirectX::XMFLOAT4 textureVal;
	DirectX::XMFLOAT4 sphereTextureVal;
	DirectX::XMFLOAT4 toonTextureVal;
};

struct GroupMorph {
	unsigned int morphIdx;
	float MorphVal;
};

struct MorphInfo {
	int morphNameSize;
	std::wstring morphName;
	int morphNameSizeE;
	std::wstring morphNameE;
	unsigned char controlPanel;
	unsigned char morphType;
	unsigned int morphOffsetCnt;
	std::vector<VertecMorph> vertexMorphs;
	std::vector<UVMorph> uvMorphs;
	std::vector<UVMorph> addUvMorphs;
	std::vector<BoneMorph> boneMorphs;
	std::vector<MaterialMorph> materialMorphs;
	std::vector<GroupMorph> groupMorphs;
};

class PMXManager
{
private:
	std::vector<std::string> toonFileNames = {
		"toon01.bmp","toon02.bmp","toon03.bmp","toon04.bmp","toon05.bmp",
		"toon06.bmp","toon07.bmp","toon08.bmp","toon09.bmp","toon10.bmp",
	};

	// PMD���_���z��
	std::vector<VertexInfo> vertInfo;
	// PMD�C���f�b�N�X���z��
	std::vector<unsigned int> SurfaceList;
	// PMD�}�e���A�����z��
	std::vector<PMXMaterialInfo> pmxMatInfo;
	// �e�N�X�`���p�X�Ǘ��p�z��
	std::vector<TexturePathInfo> texPathInfo;
	// �{�[�����z��
	std::vector<BoneInfo> boneInfos;
	// ���[�t���z��
	std::vector<MorphInfo> mInfos;

public:
	PMXManager();
	~PMXManager();

	bool LoadModel(std::string fileName);

	// PMX���_���擾
	std::vector<VertexInfo> GetPMXVertices();
	// PMX�C���f�b�N�X���擾
	std::vector<unsigned int> GetPMXIndices();
	// PMX�}�e���A�����擾
	std::vector<PMXMaterialInfo> GetPMXMaterials();
	// PMX�e�N�X�`���p�X�擾
	std::vector<TexturePathInfo> GetPMXTexturePath();
	// �{�[�����擾
	std::vector<BoneInfo> GetPMXBoneInfo();
	// ���[�t���擾
	std::vector<MorphInfo> GetPMXMorphInfo();
	// �g�D�[�����擾
	std::string GetToonFileName(int idx);
};

