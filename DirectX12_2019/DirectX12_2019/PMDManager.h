#pragma once
#include <string>
#include <windows.h>
#include <vector>
#include <map>
#include <DirectXMath.h>

// PMDヘッダー構造体
struct PMDHeader {
	char magic[3];
	float version;
	char modelName[20];
	char comment[256];
};

// PMDマテリアル情報構造体
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

// 頂点情報構造体
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
	// モデルロード
	bool ModelRead(const std::string filename);
	// PMD頂点情報取得
	std::vector<unsigned char> GetPMDVertices();
	// PMDインデックス情報取得
	std::vector<unsigned short> GetPMDIndices();
	// PMDマテリアル情報取得
	std::vector<PMDMaterial> GetPMDMaterials();
	// PMDボーン情報取得
	std::vector<PMDBoneInfo> GetPMDBones();

private:
	// PMD頂点情報配列
	std::vector<unsigned char> pmdVertices;
	// PMDインデックス情報配列
	std::vector<unsigned short> pmdIndices;
	// PMDマテリアル情報配列
	std::vector<PMDMaterial> pmdMaterials;
	// PMDボーン情報配列
	std::vector<PMDBoneInfo> pmdBones;
};

