#include "pch.h"
#include "JayleeClient.h"
#include "visual/window.h"
#include "logic/hiddenWinwsStarter.h"
#include "logic/configSystem.h"
#include <sstream>
#include <iostream>
#include <thread>
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <appmodel.h>

using namespace winrt::Microsoft::UI::Xaml;

namespace winrt::Jaylee_client::implementation {

JayleeClient::JayleeClient()
{
}

void JayleeClient::Start()
{
	// Диагностика запуска: лог через OutputDebugString и попытка записать в %APPDATA%\Jaylee Client
	OutputDebugStringW(L"JayleeClient::Start invoked\n");
	// Путь для записи логов в %APPDATA%\Jaylee Client
	auto GetAppDataPath = []() -> std::wstring {
		wchar_t path[MAX_PATH] = {};
		if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, path)) && path[0] != L'\0') {
			return std::wstring(path);
		}
		return L"";
	};
	std::wstring appdata = GetAppDataPath();
	std::wstring logPath;
	if (!appdata.empty()) {
		std::filesystem::create_directories(std::filesystem::path(appdata) / L"Jaylee Client");
		logPath = appdata + L"\\Jaylee Client\\startup_log.txt";
	} else {
		wchar_t buf[MAX_PATH] = {};
		if (GetModuleFileNameW(nullptr, buf, MAX_PATH) == 0) buf[0] = L'\0';
		std::wstring moduleDir(buf);
		size_t pos = moduleDir.find_last_of(L"\\/");
		if (pos != std::wstring::npos) moduleDir = moduleDir.substr(0, pos);
		logPath = moduleDir + L"\\startup_log.txt";
	}
	try {
		std::wofstream ofs(logPath, std::ios::app);
		if (ofs) {
			ofs << L"Jaylee Start invoked\n";
			ofs << L"LogPath: " << logPath << L"\n";
		}
	} catch (...) { OutputDebugStringW(L"JayleeClient: failed to write startup log\n"); }

	LoadSettingsFromConfig();

	OutputDebugStringW(L"JayleeClient: about to call CreateJayleeWindow\n");
	m_window = CreateJayleeWindow(L"Jaylee Client");
	OutputDebugStringW(L"JayleeClient: CreateJayleeWindow returned\n");
	if (m_window)
	{
		OutputDebugStringW(L"JayleeClient: m_window non-null, activating\n");
		m_window.Activate();
	} else {
		OutputDebugStringW(L"JayleeClient: m_window is null\n");
	}
}

void JayleeClient::StartWinws(const std::wstring& exePath, const std::wstring& args)
{
	auto res = HiddenWinwsStarter::Start(exePath, args);
	if (!res.success) {
		std::wstring msg = L"HiddenWinwsStarter failed: " + res.errorMessage;
		OutputDebugStringW(msg.c_str());
		return;
	}

	std::wstring okMsg = L"HiddenWinwsStarter started winws.exe PID=" + std::to_wstring(res.processId);
	OutputDebugStringW(okMsg.c_str());

	}
}