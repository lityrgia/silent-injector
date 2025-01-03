#pragma once
#include <iostream>
#include <Windows.h>
#include <psapi.h>
#include <string>
#include <vector>

#pragma comment(lib, "psapi.lib")

class Process
{
public:
	static DWORD findProcessByName(const std::string& processName);
	static std::wstring getCurrentProcessName();
	static std::wstring getCurrentProcessLastFolder();
};

