#include <iostream>
#include <string>

#include "main/cleaners/Explorer.h"
#include "main/cleaners/Registry.h"
#include "main/injector/Injector.h"
#include "sdk/Logger.h"
#include "sdk/Process.h"

int main()
{
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    setlocale(LC_ALL, "");
    system("title Silent injector");

    if (!IsUserAnAdmin()) {
        Logger::log(Logger::Type::Warning, "Run this program as an administrator \n");
        system("pause");
        return 1;
    }

    Logger::log(Logger::Type::Input, "Enter dll path: ");
    
    std::wstring dllPath;
    std::getline(std::wcin, dllPath);

    Logger::log(Logger::Type::Input, "Enter process name: ");

    std::string processName;
    std::getline(std::cin, processName);

    Injector::adjustPrivileges();
    bool injectedDll = Injector::injectDll(dllPath, processName);

    if (injectedDll) Logger::log(Logger::Type::Info, "Dll succesfly injected\n");
    else Logger::log(Logger::Type::Error, "Failed to inject dll\n");

    Logger::log(Logger::Type::Info, "Starting cleaning routine\n");

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::wstring currentProcessName = Process::getCurrentProcessName();
    std::wstring currentProcessLastFolder = Process::getCurrentProcessLastFolder();
    std::wstring dllFileName = L"UnknownZXC";

    if (injectedDll) {
        size_t pos = dllPath.find_last_of(L"\\/");
        if (pos == std::wstring::npos) dllFileName = dllPath;
        else dllFileName = dllPath.substr(pos + 1);

        if (!Registry::deleteValueFromRecentDocs(dllFileName)) Logger::log(Logger::Type::Warning, "Nothing found inside Recent Docs\n");
    }
    
    if (!Explorer::deleteFileFromPrefetch(currentProcessName)) Logger::log(Logger::Type::Warning, "Nothing found inside Prefetch\n");
    if (!Explorer::deleteFileFromRecent(currentProcessName)) Logger::log(Logger::Type::Warning, "Nothing found inside Recent Files\n");
    if (!Registry::deleteValueFromUserAssist(currentProcessName)) Logger::log(Logger::Type::Warning, "Nothing found inside User Assist\n");
    if (!Registry::deleteValueFromBAM(currentProcessName)) Logger::log(Logger::Type::Warning, "Nothing found inside BAM structures\n");
    if (!Registry::deleteValueFromShallBags(currentProcessLastFolder)) Logger::log(Logger::Type::Warning, "Nothing found inside Shell Bags\n");

    system("pause");
}