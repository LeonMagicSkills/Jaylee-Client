
#include "pch.h"
#include "..\\..\\obj\\Debug\\x64\\Generated Files\\winrt\\base.h"
#include "strategyAutorun.h"
#include "buttonsLogic.h"
#include <windows.h>
#include <shlobj.h>
#include <string>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

static bool s_autorunActive = false;

static std::wstring GetConfigPathLocal() {
	wchar_t path[MAX_PATH] = {};
	if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, path)) && path[0] != L'\0') {
		fs::path cfgPath = fs::path(path) / L"Jaylee" / L"cfg";
		return (cfgPath / L"config.cfg").wstring();
	}
	return L"";
}

static int ReadStrategyFromConfig() {
	std::wstring cfg = GetConfigPathLocal();
	if (cfg.empty()) return 0;
	if (!fs::exists(cfg)) return 0;
	std::ifstream ifs(cfg);
	if (!ifs.is_open()) return 0;
	std::string line;
	while (std::getline(ifs, line)) {
		if (line.rfind("Strategy=", 0) == 0) {
			try {
				int v = std::stoi(line.substr(9));
				return v;
			} catch (...) {
				return 0;
			}
		}
	}
	return 0;
}

void StartStrategyAutorunFromConfig() {
	if (s_autorunActive) return;
	int alt = ReadStrategyFromConfig();
	if (alt <= 0) return;
	SelectStrategy(alt);
	StartSelectedBypass();
	s_autorunActive = true;
}

void StopStrategyAutorun() {
	if (!s_autorunActive) return;
	StopBypass();
	s_autorunActive = false;
}

