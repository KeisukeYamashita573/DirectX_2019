#pragma once
#include <string>

// ファイル名操作用クラス
class FileNaneController
{
private:

public:
	FileNaneController();
	~FileNaneController();

	// モデル用テクスチャのパスを取得する関数
	std::string GetPMDTexturePath(const std::string& path, const char* texpath);
	std::wstring GetPMXTexturePath(const std::wstring& path, const std::wstring& texpath);
	// 二つ連結されたファイル名を分割してpairで返す
	std::pair<std::string, std::string> SplitFileName(const std::string& fileName, const char splitter = '*');
	// ファイルの拡張子を取得する
	std::string GetExtension(const std::string& fileName);
};

