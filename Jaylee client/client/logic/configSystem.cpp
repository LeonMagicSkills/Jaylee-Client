#include "pch.h"
#include "configSystem.h"
#include "buttonsLogic.h"
#include <windows.h>
#include <shlobj.h>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <utility>

namespace fs = std::filesystem;

static std::wstring GetConfigPath() {
    wchar_t path[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, path)) && path[0] != L'\0') {
        std::error_code ec;
        fs::path cfgPath = fs::path(path) / L"Jaylee" / L"cfg";
        if (!fs::exists(cfgPath, ec)) {
            fs::create_directories(cfgPath, ec);
        }
        return (cfgPath / L"config.cfg").wstring();
    }
    return L"";
}

void LoadSettingsFromConfig() {
    std::wstring configPath = GetConfigPath();
    if (configPath.empty()) return;

    if (!fs::exists(configPath)) {
        SaveSettingsToConfig();
        return;
    }

    std::ifstream ifs(configPath.c_str());
    if (!ifs.is_open()) return;

    std::vector<std::pair<std::string, std::string>> loadedSettings;
    std::string line;
    while (std::getline(ifs, line)) {
        std::istringstream iss(line);
        std::string key;
        if (std::getline(iss, key, '=')) {
            std::string value;
            if (std::getline(iss, value)) {
                loadedSettings.push_back({key, value});
            }
        }
    }
    ifs.close();

    for (const auto& pair : loadedSettings) {
        const std::string& key = pair.first;
        const std::string& value = pair.second;

        try {
            if (key == "Strategy") {
                int alt = std::stoi(value);
                SelectStrategy(alt);
            } else if (key == "TrayRunning") {
                int tray = std::stoi(value);
                if (tray == 1 && !IsTrayRunning()) {
                    ToggleTray();
                } else if (tray == 0 && IsTrayRunning()) {
                    ToggleTray();
                }
            } else if (key == "StrategyAutostart") {
                int sa = std::stoi(value);
                if (sa == 1 && !IsStrategyAutostartEnabled()) {
                    ToggleStrategyAutostart();
                } else if (sa == 0 && IsStrategyAutostartEnabled()) {
                    ToggleStrategyAutostart();
                }
            } else if (key == "Autorun") {
                int ar = std::stoi(value);
                if (ar == 1 && !IsAutorunEnabled()) {
                    ToggleAutorun();
                } else if (ar == 0 && IsAutorunEnabled()) {
                    ToggleAutorun();
                }
            }
        } catch (...) {
            // Ignore parse errors
        }
    }
}

void SaveSettingsToConfig() {
    std::wstring configPath = GetConfigPath();
    if (configPath.empty()) return;

    std::ofstream ofs(configPath.c_str(), std::ios::trunc);
    if (ofs.is_open()) {
        ofs << "Strategy=" << GetSelectedStrategy() << "\n";
        ofs << "TrayRunning=" << (IsTrayRunning() ? 1 : 0) << "\n";
        ofs << "StrategyAutostart=" << (IsStrategyAutostartEnabled() ? 1 : 0) << "\n";
        ofs << "Autorun=" << (IsAutorunEnabled() ? 1 : 0) << "\n";
    }
}
