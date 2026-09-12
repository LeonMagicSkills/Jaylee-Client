#include "pch.h"
#include "App.xaml.h"
#if __has_include("XamlMetaDataProvider.g.cpp")
#include "XamlMetaDataProvider.g.cpp"
#endif
#include "client/JayleeClient.h"
#include <fstream>
#include <string>
#include <shlobj.h>
#include <filesystem>

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::Jaylee_client::implementation
{
    App::App()
    {
        OutputDebugStringW(L"App::App constructor invoked\n");
        InitializeComponent();
        RequestedTheme(Microsoft::UI::Xaml::ApplicationTheme::Light);

#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
        UnhandledException([](IInspectable const&, UnhandledExceptionEventArgs const& e)
        {
            auto errorMessage = e.Message();
            OutputDebugStringW(L"WinUI unhandled exception: ");
            OutputDebugStringW(errorMessage.c_str());
            OutputDebugStringW(L"\n");
            MessageBoxW(nullptr, errorMessage.c_str(), L"Ошибка запуска WinUI", MB_OK | MB_ICONERROR);
            e.Handled(true);
        });
#endif
    }

    void App::OnLaunched([[maybe_unused]] LaunchActivatedEventArgs const& e)
    {

        auto GetModuleDir = []() -> std::wstring {
            wchar_t buf[MAX_PATH] = {};
            if (GetModuleFileNameW(nullptr, buf, MAX_PATH) == 0) return L".";
            std::wstring p(buf);
            size_t pos = p.find_last_of(L"\\/");
            if (pos == std::wstring::npos) return L".";
            return p.substr(0, pos);
        };
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
            logPath = appdata + L"\\Jaylee Client\\startup_log.txt";
            try {
                std::filesystem::create_directories(std::filesystem::path(appdata) / L"Jaylee Client");
            } catch (...) {
                OutputDebugStringW(L"App::OnLaunched: failed to create log directory\n");
                logPath.clear();
            }
        } else {
            std::wstring moduleDir = GetModuleDir();
            logPath = moduleDir + L"\\startup_log.txt";
        }
        try {
            if (!logPath.empty()) {
                std::wofstream ofs(logPath, std::ios::app);
                ofs << L"App::OnLaunched invoked\n";
            }
        } catch (...) { OutputDebugStringW(L"App::OnLaunched: failed to write log\n"); }

        try {
            JayleeClient client;
            client.Start();
            window = client.Window();
            if (window)
            {
                window.Activate();
            }
        }

        catch (const std::exception& ex) {
            OutputDebugStringA(ex.what());
            try {
                std::ofstream ofs("startup_log.txt", std::ios::app);
                ofs << "std::exception: " << ex.what() << "\n";
            } catch(...) {}
            MessageBoxW(NULL, L"Exception during launch. See startup_log.txt.", L"Launch error", MB_OK | MB_ICONERROR);
        }
        catch (...) {
            OutputDebugStringW(L"Unknown exception in OnLaunched\n");
            try { std::ofstream ofs(std::string("") + std::string("startup_log.txt"), std::ios::app); ofs << "Unknown exception in OnLaunched\n"; } catch(...) {}
            MessageBoxW(NULL, L"Unknown error during launch. See startup_log.txt.", L"Launch error", MB_OK | MB_ICONERROR);
        }
    }
}

void* winrt_make_Jaylee_client_App()
{
    return winrt::detach_abi(winrt::make<winrt::Jaylee_client::factory_implementation::App>());
}

