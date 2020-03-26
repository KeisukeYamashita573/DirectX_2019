#pragma once
#include <string>
#include <windows.h>
#include <vector>
#include <map>
#include <DirectXMath.h>

// PMD�w�b�_�[�\����
struct PMDHeader {
	char magic[3];
	float version;
	char modelName[20];
	char comment[256];
};

// PMD�}�e���A�����\����
struct PMDMaterial {
	DirectX::XMFLOAT3 diffuseColor;
	float alpha;
	float specularity;
	DirectX::XMFLOAT3 specularColor;
	DirectX::XMFLOAT3 mirrorColor;
	unsigned char toonIndex;
	unsigned char edgeFlag;
	unsigned int faceVertCount;
	char textureFileName[20];
};

struct PMDBoneInfo {
	char boneName[20];
	unsigned short parentBoneIndex;
	unsigned short tailBoneIndex;
	unsigned char boneType;
	unsigned short ikParentBoneIndex;
	DirectX::XMFLOAT3 boneHeadPos;
};

// ���_���\����
struct VerticesInfo {
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 normalVec;
	DirectX::XMFLOAT2 uv;
	unsigned short boneNum[2];
	unsigned char boneWeight;
	unsigned char edgeFlag;
};

class PMDManager
{
public:
	PMDManager();
	~PMDManager();
	// ���f�����[�h
	bool ModelRead(const std::string filename);
	// PMD���_���擾
	std::vector<unsigned char> GetPMDVertices();
	// PMD�C���f�b�N�X���擾
	std::vector<unsigned short> GetPMDIndices();
	// PMD�}�e���A�����擾
	std::vector<PMDMaterial> GetPMDMaterials();
	// PMD�{�[�����擾
	std::vector<PMDBoneInfo> GetPMDBones();

private:
	// PMD���_���z��
	std::vector<unsigned char> pmdVertices;
	// PMD�C���f�b�N�X���z��
	std::vector<unsigned short> pmdIndices;
	// PMD�}�e���A�����z��
	std::vector<PMDMaterial> pmdMaterials;
	// PMD�{�[�����z��
	std::vector<PMDBoneInfo> pmdBones;
};

