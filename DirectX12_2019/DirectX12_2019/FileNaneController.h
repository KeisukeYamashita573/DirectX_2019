#pragma once
#include <string>

// �t�@�C��������p�N���X
class FileNaneController
{
private:

public:
	FileNaneController();
	~FileNaneController();

	// ���f���p�e�N�X�`���̃p�X���擾����֐�
	std::string GetPMDTexturePath(const std::string& path, const char* texpath);
	std::wstring GetPMXTexturePath(const std::wstring& path, const std::wstring& texpath);
	// ��A�����ꂽ�t�@�C�����𕪊�����pair�ŕԂ�
	std::pair<std::string, std::string> SplitFileName(const std::string& fileName, const char splitter = '*');
	// �t�@�C���̊g���q���擾����
	std::string GetExtension(const std::string& fileName);
};

