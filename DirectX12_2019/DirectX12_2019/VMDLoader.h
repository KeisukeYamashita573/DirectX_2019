#pragma once
#include <string>
#include <map>
#include <vector>
#include <windows.h>
#include <DirectXMath.h>

struct MotionData {
	char boneName[15];
	unsigned int flameNo;
	DirectX::XMFLOAT3 location;
	DirectX::XMFLOAT4 rotation;
	unsigned char interpolation[64];
};

struct KeyFrame {
	KeyFrame() : frameNo(0),position(0,0,0),quaternion(0,0,0,0){};
	KeyFrame(int frame_no, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT4 rot, const unsigned char ax, const unsigned char ay, const unsigned char bx, const unsigned char by) : frameNo(frame_no),position(pos),quaternion(rot) {
		bz1 = DirectX::XMFLOAT2(static_cast<float>(ax) / 127.f, static_cast<float>(ay) / 127.f);
		bz2 = DirectX::XMFLOAT2(static_cast<float>(bx) / 127.f, static_cast<float>(by) / 127.f);
	};
	int frameNo;
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT4 quaternion;
	DirectX::XMFLOAT2 bz1;
	DirectX::XMFLOAT2 bz2;
};

class VMDLoader
{
private:
	std::vector<MotionData> motionData;
	std::map<std::string,std::vector<KeyFrame>> animationData;
public:
	VMDLoader();
	~VMDLoader();

	bool LoadVMD(std::string fileName);
	float Duration();

	std::map<std::string,std::vector<KeyFrame>> GetKeyFrameData();
};

