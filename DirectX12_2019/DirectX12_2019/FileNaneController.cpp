#include "FileNaneController.h"
#include <windows.h>

FileNaneController::FileNaneController()
{
}

FileNaneController::~FileNaneController()
{
}

std::string FileNaneController::GetPMDTexturePath(const std::string& path, const char* texpath)
{
	// '/'�𕶎���̌�납�猟��
	int pathIndex1 = static_cast<int>(path.rfind('/'));
	// '\\'�𕶎���̌�납�猟��
	int pathIndex2 = static_cast<int>(path.rfind('\\'));
	// ���������ق����g��
	auto pathIndex = max(pathIndex1, pathIndex2);
	// �C���f�b�N�X���g���ĕ�����𕪊�
	auto folderPath = path.substr(0, pathIndex + (size_t)1);
	return folderPath + texpath;
}

std::wstring FileNaneController::GetPMXTexturePath(const std::wstring& path, const std::wstring& texpath)
{
	// '/'�𕶎���̌�납�猟��
	int pathIndex1 = static_cast<int>(path.rfind('/'));
	// '\\'�𕶎���̌�납�猟��
	int pathIndex2 = static_cast<int>(path.rfind('\\'));
	// ���������ق����g��
	auto pathIndex = max(pathIndex1, pathIndex2);
	// �C���f�b�N�X���g���ĕ�����𕪊�
	auto folderPath = path.substr(0, pathIndex + (size_t)1);
	return folderPath + texpath.c_str();
}

std::pair<std::string, std::string> FileNaneController::SplitFileName(const std::string& fileName, const char splitter)
{
	// �����t�@�C�����𕪊�����
	auto idx = fileName.rfind(splitter);
	std::pair < std::string, std::string> ret;
	ret.first = fileName.substr(0, idx);
	ret.second = fileName.substr(idx + 1, fileName.length() - idx - 1);
	return ret;
}

std::string FileNaneController::GetExtension(const std::string& fileName)
{
	// �g���q���擾
	auto idx = fileName.rfind('.');
	return fileName.substr(idx + 1, fileName.length() - idx - 1);

}
