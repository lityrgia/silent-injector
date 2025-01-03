#include "Explorer.h"
#include "../../sdk/Logger.h"

static const std::string prefetchPath = "C:\\Windows\\Prefetch";

bool Explorer::deleteFileFromPrefetch(const std::wstring& fileName) {
    if (fs::exists(prefetchPath) && fs::is_directory(prefetchPath)) {

        for (const auto& entry : fs::directory_iterator(prefetchPath)) {

            if (entry.is_regular_file() && entry.path().extension() == ".pf") {
                std::wstring currentFileName = entry.path().filename().wstring();
                std::wstring transformatedFileName = fileName;

                std::transform(transformatedFileName.begin(), transformatedFileName.end(), transformatedFileName.begin(), ::toupper);

                if (currentFileName.find(transformatedFileName) != std::string::npos) {
                    fs::remove(entry.path());
                    Logger::log(Logger::Type::Info, "Removed file %ls from Prefetch\n", currentFileName.c_str());
                    return true;
                }
            }
        }
    }
    
    return false;
}

bool Explorer::deleteFileFromRecent(const std::wstring& fileName) {
    PWSTR path = NULL;
    std::string appDataPath = "";

    if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &path) == S_OK) {
        std::wstring widePath(path);
        appDataPath = std::string(widePath.begin(), widePath.end());

        CoTaskMemFree(path);
    }

    appDataPath += "\\Microsoft\\Windows\\Recent";

        if (fs::exists(appDataPath) && fs::is_directory(appDataPath)) {

            for (const auto& entry : fs::directory_iterator(appDataPath)) {

                if (entry.is_regular_file()) {
                    std::wstring currentFileName = entry.path().filename().wstring();
                    std::wstring transformatedFileName = fileName;

                    std::transform(currentFileName.begin(), currentFileName.end(), currentFileName.begin(), ::toupper);
                    std::transform(transformatedFileName.begin(), transformatedFileName.end(), transformatedFileName.begin(), ::toupper);

                    if (currentFileName.find(transformatedFileName) != std::string::npos) {
                        fs::remove(entry.path());
                        Logger::log(Logger::Type::Info, "Removed file %ls from Recent\n", currentFileName.c_str());
                        return true;
                    }
                }
            }
        }

    return false; 
}