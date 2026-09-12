#pragma once

#include <string>
#include <windows.h>

namespace workintray {

    int RunTrayBackgroundProcess(
        HINSTANCE hInstance,
        const std::wstring& appName = L"ZamokClient",
        const std::wstring& tooltip = L"ZamokClient");

}
