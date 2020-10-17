
// CommonUtility.h

#pragma once

//#include <filesystem>
#include <unordered_map>
#include <memory>
#include <Windows.h>
#include <atlsync.h>
#include <boost\optional.hpp>
#include <boost\filesystem.hpp>

namespace fs = boost::filesystem;

/// 現在実行中の exeのあるフォルダのパスを返す
fs::path GetExeDirectory();

fs::path	SearchFirstFile(const fs::path& search);

std::string	LoadFile(const fs::path& filePath);
void		SaveFile(const fs::path& filePath, const std::string& content);














