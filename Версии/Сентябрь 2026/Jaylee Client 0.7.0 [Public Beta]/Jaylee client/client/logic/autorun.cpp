#include "pch.h"
#include "autorun.h"
#include <windows.h>
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")
#include <shlobj.h>
#include <objbase.h>
#include <string>
#include <filesystem>

namespace autorun {

    bool CreateStartupShortcut() {
        wchar_t exePath[MAX_PATH] = {};
        DWORD length = GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        if (length == 0 || length == MAX_PATH) {
            return false;
        }

        wchar_t startupFolder[MAX_PATH] = {};
        HRESULT folderResult = SHGetFolderPathW(nullptr, CSIDL_STARTUP, nullptr, SHGFP_TYPE_CURRENT, startupFolder);
        if (FAILED(folderResult)) {
            return false;
        }

        std::wstring shortcutPath = std::wstring(startupFolder) + L"\\Jaylee.lnk";
        std::wstring exePathString = exePath;
        std::wstring workingDirectory = exePathString;

        const size_t slashPosition = workingDirectory.find_last_of(L"\\/");
        if (slashPosition != std::wstring::npos) {
            workingDirectory = workingDirectory.substr(0, slashPosition);
        }
        else {
            workingDirectory = L".";
        }

        HRESULT comInitResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(comInitResult) && comInitResult != RPC_E_CHANGED_MODE) {
            return false;
        }

        IShellLinkW* shellLink = nullptr;
        IPersistFile* persistFile = nullptr;
        bool success = false;

        if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, reinterpret_cast<void**>(&shellLink)))) {
            shellLink->SetPath(exePath);
            shellLink->SetWorkingDirectory(workingDirectory.c_str());
            shellLink->SetDescription(L"Jaylee client");
            shellLink->SetIconLocation(exePath, 0);

            if (SUCCEEDED(shellLink->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&persistFile)))) {
                if (SUCCEEDED(persistFile->Save(shortcutPath.c_str(), TRUE))) {
                    success = true;
                }
                persistFile->Release();
            }

            shellLink->Release();
        }

        if (SUCCEEDED(comInitResult)) {
            CoUninitialize();
        }

        return success;
    }

    bool IsStartupShortcutPresent() {
        wchar_t startupFolder[MAX_PATH] = {};
        HRESULT folderResult = SHGetFolderPathW(nullptr, CSIDL_STARTUP, nullptr, SHGFP_TYPE_CURRENT, startupFolder);
        if (FAILED(folderResult)) {
            return false;
        }
        std::wstring shortcutPath = std::wstring(startupFolder) + L"\\Jaylee.lnk";
        DWORD attrs = GetFileAttributesW(shortcutPath.c_str());
        return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
    }

    bool RemoveStartupShortcut() {
        wchar_t startupFolder[MAX_PATH] = {};
        HRESULT folderResult = SHGetFolderPathW(nullptr, CSIDL_STARTUP, nullptr, SHGFP_TYPE_CURRENT, startupFolder);
        if (FAILED(folderResult)) {
            return false;
        }
        std::wstring shortcutPath = std::wstring(startupFolder) + L"\\Jaylee.lnk";
        return DeleteFileW(shortcutPath.c_str()) == TRUE;
    }

}

extern "C" bool CreateStartupShortcut_C() {
    return autorun::CreateStartupShortcut();
}

extern "C" bool IsStartupShortcutPresent_C() {
    return autorun::IsStartupShortcutPresent();
}

extern "C" bool RemoveStartupShortcut_C() {
    return autorun::RemoveStartupShortcut();
}
