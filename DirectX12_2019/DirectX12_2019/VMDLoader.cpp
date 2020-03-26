#include "VMDLoader.h"

float VMDLoader::Duration()
{
	auto framemax = 0.f;
	for (auto& anim : animationData) {
		for (int i = 0; i < anim.second.size(); i++) {
			framemax = max(framemax, anim.second[i].frameNo);
		}
	}
	return framemax;
}

VMDLoader::VMDLoader()
{
}

VMDLoader::~VMDLoader()
{
}

bool VMDLoader::LoadVMD(std::string fileName)
{
	FILE* fp = nullptr;
	fopen_s(&fp, fileName.c_str(), "rb");
	fseek(fp, 50.0f, SEEK_SET);

	// モーション数読み込み
	unsigned int motionCnt = 0;
	if (fread_s(&motionCnt, sizeof(motionCnt), sizeof(motionCnt), 1, fp) == 0) {
		return false;
	}

	// モーションデータ読み込み
	motionData.resize(motionCnt);
	for (auto i = 0; i < motionCnt; i++) {
		// ボーン名読み込み
		if (fread_s(&motionData[i].boneName, sizeof(motionData[i].boneName), sizeof(motionData[i].boneName), 1, fp) == 0) {
			return false;
		}
		auto sz = sizeof(motionData[i].flameNo) + sizeof(motionData[i].location) + sizeof(motionData[i].rotation) + sizeof(motionData[i].interpolation);
		if (fread_s(&motionData[i].flameNo, sz, sz, 1, fp) == 0) {
			return false;
		}
	}
	
	for (auto& data : motionData) {
		animationData[data.boneName].emplace_back(KeyFrame(data.flameNo,data.location, data.rotation, data.interpolation[48], data.interpolation[52], data.interpolation[56], data.interpolation[60]));
	}

	auto a = 0;

	return true;
}

std::map<std::string,std::vector<KeyFrame>> VMDLoader::GetKeyFrameData()
{
	return animationData;
}
