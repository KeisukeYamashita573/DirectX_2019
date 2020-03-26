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
	// '/'を文字列の後ろから検索
	int pathIndex1 = static_cast<int>(path.rfind('/'));
	// '\\'を文字列の後ろから検索
	int pathIndex2 = static_cast<int>(path.rfind('\\'));
	// 見つかったほうを使う
	auto pathIndex = max(pathIndex1, pathIndex2);
	// インデックスを使って文字列を分割
	auto folderPath = path.substr(0, pathIndex + (size_t)1);
	return folderPath + texpath;
}

std::wstring FileNaneController::GetPMXTexturePath(const std::wstring& path, const std::wstring& texpath)
{
	// '/'を文字列の後ろから検索
	int pathIndex1 = static_cast<int>(path.rfind('/'));
	// '\\'を文字列の後ろから検索
	int pathIndex2 = static_cast<int>(path.rfind('\\'));
	// 見つかったほうを使う
	auto pathIndex = max(pathIndex1, pathIndex2);
	// インデックスを使って文字列を分割
	auto folderPath = path.substr(0, pathIndex + (size_t)1);
	return folderPath + texpath.c_str();
}

std::pair<std::string, std::string> FileNaneController::SplitFileName(const std::string& fileName, const char splitter)
{
	// 二つあるファイル名を分割する
	auto idx = fileName.rfind(splitter);
	std::pair < std::string, std::string> ret;
	ret.first = fileName.substr(0, idx);
	ret.second = fileName.substr(idx + 1, fileName.length() - idx - 1);
	return ret;
}

std::string FileNaneController::GetExtension(const std::string& fileName)
{
	// 拡張子を取得
	auto idx = fileName.rfind('.');
	return fileName.substr(idx + 1, fileName.length() - idx - 1);

}
