#include <codecvt>
#include <filesystem>

#include "Process.h"

namespace fs = std::filesystem;

DWORD Process::findProcessByName(const std::string& processName) {
    DWORD processes[1024], cbNeeded, processCount;

    EnumProcesses(processes, sizeof(processes), &cbNeeded);

    processCount = cbNeeded / sizeof(DWORD);

    for (unsigned int i = 0; i < processCount; ++i) {
        DWORD processId = processes[i];

        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
        if (hProcess) {
            char processPath[MAX_PATH];

            if (GetModuleFileNameExA(hProcess, NULL, processPath, sizeof(processPath) / sizeof(char))) {
                std::string processFileName = processPath;
                size_t pos = processFileName.find_last_of("\\/");
                if (pos != std::string::npos)
                    processFileName = processFileName.substr(pos + 1);

                if (processFileName == processName) {
                    CloseHandle(hProcess);
                    return processId;
                }
            }
            CloseHandle(hProcess);
        }
    }

    return 0;
}

std::wstring Process::getCurrentProcessLastFolder() {
    char buffer[MAX_PATH];

    if (GetModuleFileNameA(NULL, buffer, MAX_PATH)) {
        std::string fullPath(buffer);

        size_t slashPos = fullPath.find_last_of("\\/");

        std::string fileName = fullPath.substr(0, slashPos);

        slashPos = fileName.find_last_of("\\/");

        fileName = fileName.substr(slashPos + 1);
        return fs::path(fileName).wstring();
    }

    return L"UnknownZXC";
}

std::wstring Process::getCurrentProcessName() {
    char buffer[MAX_PATH];

    if (GetModuleFileNameA(NULL, buffer, MAX_PATH)) {
        std::string fullPath(buffer);

        size_t slashPos = fullPath.find_last_of("\\/");

        std::string fileName = fullPath.substr(slashPos + 1);

        size_t dotPos = fileName.find_last_of(".");

        fileName = fileName.substr(0, dotPos);

        return fs::path(fileName).wstring();
    }

    return L"UnknownZXC";
}