#pragma once
#include <iostream>
#include <iomanip>
#include <Windows.h>

class Registry
{
public:
	static bool deleteValueFromRecentDocs(const std::wstring& processName);
	static bool deleteValueFromShallBags(const std::wstring& processName);
	static bool deleteValueFromUserAssist(const std::wstring& processName);
	static bool deleteValueFromBAM(const std::wstring& processName);
};

