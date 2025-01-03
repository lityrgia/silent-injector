#include <vector>
#include <aclapi.h>
#include <functional>
#include <unordered_set>
#include <shlwapi.h>
#include <shlobj.h>

#include "../../sdk/Logger.h"
#include "../cleaners/Registry.h"

static const std::wstring recentDocsPath = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RecentDocs\\.dll";  //HKEY_CURRENT_USER
static const std::wstring shallBagsPath = L"SOFTWARE\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\Shell\\BagMRU";  //HKEY_CURRENT_USER
static const std::wstring shallBagsPath2 = L"Local Settings\\Software\\Microsoft\\Windows\\Shell\\BagMRU";  //HKEY_CLASSES_ROOT
static const std::wstring userAssistPath = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\UserAssist";  //HKEY_CURRENT_USER
static const std::wstring bamPath = L"SYSTEM\\CurrentControlSet\\Services\\bam\\State\\UserSettings";  //HKEY_LOCAL_MACHINE

std::wstring decodeUTF16LE(const uint8_t* data, DWORD dataSize) {
    std::wstring result;

    for (DWORD i = 0; i + 1 < dataSize; i += 2) {
        wchar_t ch = static_cast<wchar_t>(data[i] | (data[i + 1] << 8));
        if (ch == L'\0')
            break;
        result += ch;
    }

    return result;
}

std::wstring binaryToWString(const BYTE* data, DWORD dataSize) {
    std::wstring result;

    for (DWORD i = 0; i < dataSize; ++i) {
        BYTE byte = data[i];

        if (byte >= 32 && byte <= 126) {
            result += static_cast<wchar_t>(byte);
        }
    }

    return result;
}

std::wstring decodeROT13(const wchar_t* data, DWORD dataSize) {
    std::wstring result;

    for (DWORD i = 0; i < dataSize; i++) {
        wchar_t ch = static_cast<wchar_t>(data[i]);

        if (iswalpha(ch)) {
            wchar_t base = (iswupper(ch)) ? L'A' : L'a';
            ch = (ch - base + 13) % 26 + base;
            result += ch;
        }
        else if (iswprint(ch)) result += ch;
        else result += L'?';
    }

    return result;
}

std::vector<std::wstring> getSubKeysOfKey(HKEY key, std::wstring keyPath) {
    std::vector<std::wstring> subKeys{ L"" };

    HKEY hOpenedKey;
    if (RegOpenKeyEx(key, keyPath.c_str(), 0, KEY_READ, &hOpenedKey) == ERROR_SUCCESS) {

        DWORD index = 0;
        WCHAR keyName[255];
        DWORD keyNameSize;

        while (true) {
            keyNameSize = sizeof(keyName) / sizeof(keyName[0]);
            LONG result = RegEnumKeyEx(hOpenedKey, index, keyName, &keyNameSize, NULL, NULL, NULL, NULL);

            if (result == ERROR_NO_MORE_ITEMS) break;
            if (result == ERROR_SUCCESS) {
                if (!keyPath.empty()) subKeys.push_back(keyPath + L"\\" + keyName);
            }
            index++;
        }

        RegCloseKey(hOpenedKey);
    }
    else {
        Logger::log(Logger::Type::Error, "Failed to open registry key: %ls \n", keyPath.c_str());
    }

    return subKeys;
}

bool Registry::deleteValueFromRecentDocs(const std::wstring& processName)
{
    HKEY hOpenedKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, recentDocsPath.c_str(), 0, KEY_ALL_ACCESS, &hOpenedKey) == ERROR_SUCCESS) {
        DWORD index = 0;
        WCHAR valueName[255];
        DWORD valueNameSize;
        DWORD valueType;
        BYTE data[1024];
        DWORD dataSize;

        while (true) {
            valueNameSize = sizeof(valueName) / sizeof(valueName[0]);
            dataSize = sizeof(data);
            LONG result = RegEnumValue(hOpenedKey, index, valueName, &valueNameSize, NULL, &valueType, data, &dataSize);
            if (result == ERROR_NO_MORE_ITEMS) break;
            if (result == ERROR_SUCCESS) {
                if (valueType == REG_BINARY && dataSize % 2 == 0) {
                    std::wstring decodedText = decodeUTF16LE(reinterpret_cast<uint8_t*>(data), dataSize);
                    if (decodedText.find(processName) != std::string::npos) {
                        LONG deleteResult = RegDeleteValue(hOpenedKey, valueName);

                        if (deleteResult == ERROR_SUCCESS) {
                            Logger::log(Logger::Type::Info, "Removed %ls from RecentDocs \n", processName.c_str());
                            RegCloseKey(hOpenedKey);
                            return true;
                        }
                        else Logger::log(Logger::Type::Error, "Failed to remove %ls with code %d \n", processName.c_str(), deleteResult);
                    }
                }

                index++;
            }
        }

        RegCloseKey(hOpenedKey);
    }

    return false;
}

std::wstring getCurrentUserName() {
    DWORD dwSize = 256;
    wchar_t username[256];

    if (GetUserNameW(username, &dwSize)) {
        return std::wstring(username);
    }
    else return L"";
}

//ohh god thanks to https://stackoverflow.com/questions/2438291/programmatically-allow-write-access-for-a-registry-key
int grantAccessToKey(HKEY key) {
    PSECURITY_DESCRIPTOR pSD = NULL;
    EXPLICIT_ACCESS ea;
    PACL pOldDACL = NULL, pNewDACL = NULL;

    LONG lRes = GetSecurityInfo(key, SE_REGISTRY_KEY, DACL_SECURITY_INFORMATION, NULL, NULL, &pOldDACL, NULL, &pSD);
    if (lRes != ERROR_SUCCESS) {
        RegCloseKey(key);
        return lRes;
    }

    std::wstring userName = getCurrentUserName();

    memset(&ea, 0, sizeof(EXPLICIT_ACCESS));
    ea.grfAccessPermissions = KEY_ALL_ACCESS;
    ea.grfAccessMode = GRANT_ACCESS;
    ea.grfInheritance = NO_INHERITANCE;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    ea.Trustee.ptstrName = const_cast<wchar_t*>(userName.c_str());

    lRes = SetEntriesInAcl(1, &ea, pOldDACL, &pNewDACL);
    if (lRes != ERROR_SUCCESS) {
        if (pSD) LocalFree((HLOCAL)pSD);
        RegCloseKey(key);
        return lRes;
    }

    lRes = SetSecurityInfo(key, SE_REGISTRY_KEY, DACL_SECURITY_INFORMATION, NULL, NULL, pNewDACL, NULL);
    if (lRes != ERROR_SUCCESS) {
        if (pSD) LocalFree((HLOCAL)pSD);
        if (pNewDACL) LocalFree((HLOCAL)pNewDACL);
        RegCloseKey(key);
        return lRes;
    }

    if (pSD) LocalFree((HLOCAL)pSD);
    if (pNewDACL) LocalFree((HLOCAL)pNewDACL);
    RegCloseKey(key);
}

bool Registry::deleteValueFromUserAssist(const std::wstring& processName)
{
    bool foundValue = false;
    std::vector<std::wstring> userAssistKeys = getSubKeysOfKey(HKEY_CURRENT_USER, userAssistPath);

    for (auto& key : userAssistKeys) {
        key = key += L"\\Count";
        HKEY hSubKey;
        if (RegOpenKeyEx(HKEY_CURRENT_USER, key.c_str(), 0, KEY_READ | KEY_SET_VALUE, &hSubKey) == ERROR_SUCCESS) {
            DWORD valueIndex = 0;
            WCHAR valueName[255];
            DWORD valueNameSize;
            DWORD valueType;
            BYTE data[1024];
            DWORD dataSize;

            while (true) {
                valueNameSize = sizeof(valueName) / sizeof(valueName[0]);
                dataSize = sizeof(data);
                LONG resultValue = RegEnumValue(hSubKey, valueIndex, valueName, &valueNameSize, NULL, &valueType, data, &dataSize);
                if (resultValue == ERROR_NO_MORE_ITEMS) {
                    break;
                }
                if (resultValue == ERROR_SUCCESS && valueType == REG_BINARY) {

                    std::wstring decodedText = decodeROT13(valueName, valueNameSize);
                    if (decodedText.find(processName) != std::string::npos) {

                        LONG deleteResult = RegDeleteValue(hSubKey, valueName);

                        if (deleteResult == ERROR_SUCCESS) {
                            Logger::log(Logger::Type::Info, "Removed %ls from UserAssist \n", processName.c_str());
                        }
                        else Logger::log(Logger::Type::Error, "Failed to remove %ls with code %d \n", processName.c_str(), deleteResult);

                        foundValue = true;
                    }
                }

                valueIndex++;
            }

            RegCloseKey(hSubKey);
        }
    }

    return foundValue;
}

bool Registry::deleteValueFromBAM(const std::wstring& processName)
{
    bool foundValue = false;
    std::vector<std::wstring> bamKeys = getSubKeysOfKey(HKEY_LOCAL_MACHINE, bamPath);

    for (auto& key : bamKeys) {
        HKEY hSubKey;
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, key.c_str(), 0, KEY_READ, &hSubKey) == ERROR_SUCCESS) {
            grantAccessToKey(hSubKey);
            RegOpenKeyEx(HKEY_LOCAL_MACHINE, key.c_str(), 0, KEY_READ | KEY_SET_VALUE, &hSubKey);

            DWORD valueIndex = 0;
            WCHAR valueName[255];
            DWORD valueNameSize;
            DWORD valueType;
            BYTE data[1024];
            DWORD dataSize;

            while (true) {
                valueNameSize = sizeof(valueName) / sizeof(valueName[0]);
                dataSize = sizeof(data);
                LONG resultValue = RegEnumValue(hSubKey, valueIndex, valueName, &valueNameSize, NULL, &valueType, data, &dataSize);
                if (resultValue == ERROR_NO_MORE_ITEMS) {
                    break;
                }
                if (resultValue == ERROR_SUCCESS && valueType == REG_BINARY) {
                    std::wstring wstr(valueName);

                    if (wstr.find(processName) != std::string::npos) {

                        LONG deleteResult = RegDeleteValue(hSubKey, valueName);

                        if (deleteResult == ERROR_SUCCESS) {
                            Logger::log(Logger::Type::Info, "Removed %ls from BAM structures \n", wstr.c_str());
                        }
                        else Logger::log(Logger::Type::Error, "Failed to remove %ls with code %d \n", processName.c_str(), deleteResult);

                        foundValue = true;
                    }
                }

                valueIndex++;
            }

            RegCloseKey(hSubKey);
        }
    }

    return foundValue;
}

std::wstring GetPathFromItemIDList(const BYTE* data, DWORD dataSize) {
    PIDLIST_ABSOLUTE pidl = reinterpret_cast<PIDLIST_ABSOLUTE>(const_cast<BYTE*>(data));

    WCHAR path[MAX_PATH];
    if (SHGetPathFromIDListW(pidl, path)) {
        return std::wstring(path);
    }
    return L"";
}

bool Registry::deleteValueFromShallBags(const std::wstring& processName)
{
    bool foundValue = false;

    std::unordered_set<std::wstring> processedKeys;

    std::function<void(HKEY, const std::wstring&)> processRegistryKey = [&](HKEY rootKey, const std::wstring& keyPath) {
        if (processedKeys.find(keyPath) != processedKeys.end()) {
            return;
        }

        processedKeys.insert(keyPath);

        std::vector<std::wstring> subKeys = getSubKeysOfKey(rootKey, keyPath);
        subKeys.push_back(keyPath);

        for (const auto& key : subKeys) {
            DWORD index = 0;
            HKEY hOpenedKey;
            if (RegOpenKeyEx(rootKey, key.c_str(), 0, KEY_READ | KEY_SET_VALUE, &hOpenedKey) == ERROR_SUCCESS) {
                WCHAR valueName[255];
                DWORD valueNameSize;
                DWORD valueType;
                BYTE data[1024];
                DWORD dataSize;

                while (true) {
                    valueNameSize = sizeof(valueName) / sizeof(valueName[0]);
                    dataSize = sizeof(data);
                    LONG result = RegEnumValue(hOpenedKey, index, valueName, &valueNameSize, NULL, &valueType, data, &dataSize);

                    if (result == ERROR_NO_MORE_ITEMS) break;
                    if (result == ERROR_SUCCESS && valueType == REG_BINARY) {
                        std::wstring stringArray = binaryToWString(data, dataSize);

                        if (stringArray.find(processName) != std::string::npos) {
                            LONG deleteResult = RegDeleteValue(hOpenedKey, valueName);

                            if (deleteResult == ERROR_SUCCESS) {
                                Logger::log(Logger::Type::Info, "Removed %ls from Shell Bags \n", stringArray.c_str());
                            }
                            else Logger::log(Logger::Type::Error, "Failed to remove %ls with code %d \n", processName.c_str(), deleteResult);

                            foundValue = true;
                        }
                    }
                    index++;
                }

                RegCloseKey(hOpenedKey);
            }

            processRegistryKey(rootKey, key);
        }
    };

    processRegistryKey(HKEY_CURRENT_USER, shallBagsPath);
    processRegistryKey(HKEY_CLASSES_ROOT, shallBagsPath2);

    return foundValue;
}