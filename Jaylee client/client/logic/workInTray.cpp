#include "pch.h"
#include "Generated Files\\winrt\\base.h"
#include "workInTray.h"
#include <shellapi.h>
#include <string>
#include <wchar.h>
#include <tlhelp32.h>

#pragma comment(lib, "Shell32.lib")

namespace workintray {
    namespace {

        constexpr UINT kTrayMessage = WM_APP + 1;
        constexpr UINT kMenuOpen = 1001;
        constexpr UINT kMenuExit = 1002;

        struct TrayState {
            HWND hwnd = nullptr;
            NOTIFYICONDATAW notifyIcon{};
            std::wstring appName;
            bool exiting = false;
        };

        TrayState* GetTrayState(HWND hwnd) {
            return reinterpret_cast<TrayState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        void SetTrayState(HWND hwnd, TrayState* state) {
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
        }

        void ShowTrayMenu(HWND hwnd) {
            auto* state = GetTrayState(hwnd);
            if (!state) {
                return;
            }

            HMENU menu = CreatePopupMenu();
            if (menu) {
                AppendMenuW(menu, MF_STRING, kMenuOpen, L"Open client window");
                AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                AppendMenuW(menu, MF_STRING, kMenuExit, L"Exit");

                POINT pt{};
                GetCursorPos(&pt);
                int command = TrackPopupMenuEx(menu, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, hwnd, nullptr);
                DestroyMenu(menu);

                if (command == kMenuExit) {
                    // Попытаться корректно закрыть главное окно(а) приложения, затем выйти.
                    state->exiting = true;

                    // Сначала пробуем найти окно по заголовку и послать WM_CLOSE.
                    HWND mainHwnd = FindWindowW(nullptr, L"Jaylee client");
                    if (!mainHwnd) {
                        mainHwnd = FindWindowW(nullptr, L"Jaylee Client");
                    }
                    if (mainHwnd) {
                        PostMessageW(mainHwnd, WM_CLOSE, 0, 0);
                    }
                    else {
                        // Если по заголовку не найдено, пробуем закрыть все окна текущего процесса.
                        const DWORD pid = GetCurrentProcessId();
                        auto enumProc = [](HWND hwnd, LPARAM lParam) -> BOOL {
                            DWORD wndPid = 0;
                            GetWindowThreadProcessId(hwnd, &wndPid);
                            DWORD targetPid = static_cast<DWORD>(lParam);
                            if (wndPid == targetPid) {
                                // Игнорируем невидимые окна (опционально) и посылаем WM_CLOSE
                                PostMessageW(hwnd, WM_CLOSE, 0, 0);
                            }
                            return TRUE; // продолжать перечисление
                        };
                        EnumWindows(enumProc, static_cast<LPARAM>(pid));
                    }

                    // Удаляем иконку.
                    Shell_NotifyIconW(NIM_DELETE, &state->notifyIcon);

                    // Попробуем завершить все процессы с таким же именем исполняемого файла.
                    DWORD currentPid = GetCurrentProcessId();
                    wchar_t exePath[MAX_PATH] = {0};
                    if (GetModuleFileNameW(nullptr, exePath, ARRAYSIZE(exePath))) {
                        wchar_t* exeName = wcsrchr(exePath, L'\\');
                        exeName = exeName ? exeName + 1 : exePath;

                        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
                        if (snap != INVALID_HANDLE_VALUE) {
                            PROCESSENTRY32W pe{};
                            pe.dwSize = sizeof(pe);
                            if (Process32FirstW(snap, &pe)) {
                                do {
                                    if (_wcsicmp(pe.szExeFile, exeName) == 0) {
                                        // Завершить все процессы с таким именем, кроме текущего (сначала)
                                        if (pe.th32ProcessID != currentPid) {
                                            HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                                            if (hProc) {
                                                TerminateProcess(hProc, 1);
                                                CloseHandle(hProc);
                                            }
                                        }
                                    }
                                } while (Process32NextW(snap, &pe));
                            }
                            CloseHandle(snap);
                        }

                        // Теперь принудительно завершим текущий процесс.
                        // Это немедленно завершит выполнение текущего приложения.
                        TerminateProcess(GetCurrentProcess(), 1);
                    }

                    // На случай, если TerminateProcess не сработал — корректно выйти из цикла сообщений.
                    PostQuitMessage(0);
                }
                else if (command == kMenuOpen) {
                    // Попытаться показать/активировать главное окно приложения.
                    // Ищем окно по заголовку (варианты с разным регистром).
                    HWND mainHwnd = FindWindowW(nullptr, L"Jaylee client");
                    if (!mainHwnd) {
                        mainHwnd = FindWindowW(nullptr, L"Jaylee Client");
                    }

                    if (mainHwnd) {
                        // Показать и установить как активное окно
                        ShowWindow(mainHwnd, SW_SHOW);
                        SetForegroundWindow(mainHwnd);
                        BringWindowToTop(mainHwnd);
                    }
                    else {
                        // Если окно не найдено, попытаться запустить исполняемый файл приложения
                        wchar_t exePath[MAX_PATH] = {0};
                        if (GetModuleFileNameW(nullptr, exePath, ARRAYSIZE(exePath))) {
                            ShellExecuteW(nullptr, L"open", exePath, nullptr, nullptr, SW_SHOWNORMAL);
                        }
                    }
                }
            }
        }

        LRESULT CALLBACK TrayWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
            if (message == WM_CREATE) {
                auto* createStruct = reinterpret_cast<LPCREATESTRUCTW>(lParam);
                auto* state = reinterpret_cast<TrayState*>(createStruct->lpCreateParams);
                state->hwnd = hwnd;
                SetTrayState(hwnd, state);
                return 0;
            }

            if (auto* state = GetTrayState(hwnd)) {
                if (message == kTrayMessage) {
                    if (LOWORD(lParam) == WM_LBUTTONUP || LOWORD(lParam) == WM_RBUTTONUP) {
                        ShowTrayMenu(hwnd);
                    }
                    return 0;
                }

                if (message == WM_CLOSE) {
                    ShowWindow(hwnd, SW_HIDE);
                    return 0;
                }

                if (message == WM_DESTROY) {
                    if (!state->exiting) {
                        ShowWindow(hwnd, SW_HIDE);
                        return 0;
                    }
                    SetTrayState(hwnd, nullptr);
                    return 0;
                }
            }

            return DefWindowProcW(hwnd, message, wParam, lParam);
        }

        bool RegisterTrayWindow(HINSTANCE hInstance, const wchar_t* className) {
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.lpfnWndProc = TrayWindowProc;
            wc.hInstance = hInstance;
            wc.lpszClassName = className;
            wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
            if (RegisterClassExW(&wc) == 0) {
                return GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
            }
            return true;
        }

    } // namespace

    int RunTrayBackgroundProcess(HINSTANCE hInstance, const std::wstring& appName, const std::wstring& tooltip) {
        const wchar_t* className = L"ZamokTrayWindow";
        if (!RegisterTrayWindow(hInstance, className)) {
            return 1;
        }

        auto* state = new TrayState{};
        state->appName = appName;
        
        HWND hwnd = CreateWindowExW(
            0,
            className,
            appName.c_str(),
            WS_OVERLAPPED,
            0,
            0,
            0,
            0,
            nullptr,
            nullptr,
            hInstance,
            state);

        if (!hwnd) {
            delete state;
            return 2;
        }

        ShowWindow(hwnd, SW_HIDE);

        state->notifyIcon.cbSize = sizeof(state->notifyIcon);
        state->notifyIcon.hWnd = hwnd;
        state->notifyIcon.uID = 1;
        state->notifyIcon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        state->notifyIcon.uCallbackMessage = kTrayMessage;
        state->notifyIcon.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        state->notifyIcon.uVersion = NOTIFYICON_VERSION_4;
        wcsncpy_s(state->notifyIcon.szTip, ARRAYSIZE(state->notifyIcon.szTip), tooltip.c_str(), _TRUNCATE);

        if (!Shell_NotifyIconW(NIM_ADD, &state->notifyIcon)) {
            DestroyWindow(hwnd);
            delete state;
            return 3;
        }
        Shell_NotifyIconW(NIM_SETVERSION, &state->notifyIcon);

        MSG msg{};
        while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        Shell_NotifyIconW(NIM_DELETE, &state->notifyIcon);
        delete state;

        return static_cast<int>(msg.wParam);
    }

} // namespace workintray
