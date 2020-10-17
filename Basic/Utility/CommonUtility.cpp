
#include "stdafx.h"
#include "CommonUtility.h"
#include <fstream>
#include <regex>
#include <codecvt>
#include <Windows.h>
#include <atlbase.h>
#include <atlsync.h>
#include "CodeConvert.h"

extern HINSTANCE	g_hInst;

/// 現在実行中の exeのあるフォルダのパスを返す
fs::path GetExeDirectory()
{
	WCHAR exePath[MAX_PATH] = L"";
	GetModuleFileName(g_hInst, exePath, MAX_PATH);
	fs::path exeFolder = exePath;
	return exeFolder.parent_path();
}

fs::path	SearchFirstFile(const fs::path& search)
{
	WIN32_FIND_DATA fd = {};
	HANDLE hFind = ::FindFirstFileW(search.c_str(), &fd);
	if (hFind == INVALID_HANDLE_VALUE)
		return L"";

	::FindClose(hFind);

	fs::path firstFilePath = search.parent_path() / fd.cFileName;
	return firstFilePath;
}


std::string	LoadFile(const fs::path& filePath)
{
	std::ifstream fs(filePath.c_str(), std::ios::in | std::ios::binary);
	if (!fs)
		return "";

	fs.seekg(0, std::ios::end);
	std::string buff;
	auto fileSize = fs.tellg();
	buff.resize(fileSize);
	fs.seekg(std::ios::beg);
	fs.clear();
	fs.read(const_cast<char*>(buff.data()), fileSize);
	return buff;
}

void	SaveFile(const fs::path& filePath, const std::string& content)
{
	std::ofstream fs(filePath.c_str(), std::ios::out | std::ios::binary);
	if (!fs) {
		return;
		//THROWEXCEPTION("SaveFile open failed");
	}
	fs.write(content.c_str(), content.length());
}
