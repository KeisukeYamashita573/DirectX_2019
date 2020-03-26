#include "PMDManager.h"
#include <stdio.h>
#include <cassert>


PMDManager::PMDManager()
{
}

PMDManager::~PMDManager()
{

}

bool PMDManager::ModelRead(const std::string filename)
{
	// �w�b�_�[�ǂݍ���
	PMDHeader pmdHeader;
	FILE* fp = nullptr;
	fopen_s(&fp, filename.c_str(), "rb");
	if (fread_s(pmdHeader.magic, sizeof(pmdHeader.magic), sizeof(pmdHeader.magic), 1, fp) == 0) {
		return false;
	}
	if (fread_s(&pmdHeader.version, sizeof(pmdHeader) - sizeof(pmdHeader.magic) - 1,
		sizeof(pmdHeader) - sizeof(pmdHeader.magic) - 1, 1, fp) == 0) {
		return false;
	}

	// ���_���ǂݍ���
	unsigned int vertCnt;
	if (fread_s(&vertCnt, sizeof(vertCnt), sizeof(vertCnt), 1, fp) == 0) {
		return false;
	}

	// ���_���ǂݍ���
	const unsigned int vertStride = 38;
	pmdVertices.resize((SIZE_T)vertCnt * vertStride);
	fread_s(pmdVertices.data(), pmdVertices.size(), pmdVertices.size(), 1, fp);

	// �C���f�b�N�X���ǂݍ���
	unsigned int indicesCnt = 0;
	if (fread_s(&indicesCnt, sizeof(indicesCnt), sizeof(indicesCnt), 1, fp) == 0) {
		return false;
	}

	// �C���f�b�N�X���ǂݍ���
	pmdIndices.resize(indicesCnt);
	if (fread_s(pmdIndices.data(), pmdIndices.size() * sizeof(unsigned short), sizeof(unsigned short), pmdIndices.size(), fp) == 0) {
		return false;
	}

	// �}�e���A�����ǂݍ���
	unsigned int materialCnt = 0.f;
	if (fread_s(&materialCnt, sizeof(materialCnt), sizeof(materialCnt), 1, fp) == 0) {
		return false;
	}

	// �}�e���A�����ǂݍ���
	pmdMaterials.resize(materialCnt);
	for (auto& material : pmdMaterials) {
		if (fread_s(&material, 46,46, 1, fp) == 0) {
			return false;
		}
		if (fread_s(&material.faceVertCount, 24,24, 1, fp) == 0) {
			return false;
		}
	}

	// �{�[�����ǂݍ���
	unsigned short boneCount = 0;
	if (fread_s(&boneCount, sizeof(boneCount), sizeof(boneCount), 1, fp) == 0) {
		return false;
	}

	pmdBones.resize(boneCount);
	for (auto& bone : pmdBones) {
		if (fread_s(&bone, 24, 24, 1, fp) == 0) {
			return false;
		}
		if (fread_s(&bone.boneType, sizeof(bone.boneType), sizeof(bone.boneType), 1, fp) == 0) {
			return false;
		}
		if (fread_s(&bone.ikParentBoneIndex, sizeof(bone.ikParentBoneIndex) + sizeof(bone.boneHeadPos), sizeof(bone.ikParentBoneIndex) + sizeof(bone.boneHeadPos), 1, fp) == 0) {
			return false;
		}
	}

	auto a = 0;

	return true;
}

std::vector<unsigned char> PMDManager::GetPMDVertices()
{
	return pmdVertices;
}

std::vector<unsigned short> PMDManager::GetPMDIndices()
{
	return pmdIndices;
}

std::vector<PMDMaterial> PMDManager::GetPMDMaterials()
{
	return pmdMaterials;
}

std::vector<PMDBoneInfo> PMDManager::GetPMDBones()
{
	return pmdBones;
}
