#pragma once
#include <filesystem>
#include <shlobj.h>

#include "../../sdk/Logger.h"

namespace fs = std::filesystem;

class Explorer
{
public:
	static bool deleteFileFromPrefetch(const std::wstring& fileName);
	static bool deleteFileFromRecent(const std::wstring& fileName);

};