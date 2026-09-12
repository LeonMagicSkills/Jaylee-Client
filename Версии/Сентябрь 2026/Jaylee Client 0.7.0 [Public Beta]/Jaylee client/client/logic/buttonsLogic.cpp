#include "pch.h"
#include "..\\..\\obj\\Debug\\x64\\Generated Files\\winrt\\base.h"
#include "buttonsLogic.h"

#include "../visual/settings.h"
#include "../visual/aboutWindow.h"

#include <windows.h>
#include <string>
#include <vector>
#include <functional>
#include <TlHelp32.h>
#include "buttonsLogic.h"
#include "autorun.h"
#include "workInTray.h"
#include "configSystem.h"
#include <process.h>
#include <shellapi.h>
#include <thread>
#include "hiddenWinwsStarter.h"
#include <fstream>


// Find file in project zapret directories
static std::wstring FindFileInZapret(const std::wstring &fileName) {
    std::vector<std::wstring> bases;
    // Получаем директорию модуля напрямую
    wchar_t buffer[MAX_PATH] = {};
    std::wstring moduleDir = L".";
    if (GetModuleFileNameW(nullptr, buffer, MAX_PATH) > 0) {
        std::wstring path(buffer);
        size_t pos = path.find_last_of(L"\\/");
        if (pos != std::wstring::npos) moduleDir = path.substr(0, pos);
    }
    if (moduleDir.size() >= 2 && moduleDir[1] == L':') {
        std::wstring drive = moduleDir.substr(0, 2);
        bases.push_back(drive + L"\\Jaylee client\\client\\zapret\\");
        bases.push_back(drive + L"\\Jaylee client\\Jaylee client\\client\\zapret\\");
    }
    bases.push_back(moduleDir + L"\\client\\zapret\\");
    bases.push_back(moduleDir + L"\\..\\client\\zapret\\");
    bases.push_back(moduleDir + L"\\..\\..\\client\\zapret\\");
    bases.push_back(moduleDir + L"\\..\\..\\..\\client\\zapret\\");
    bases.push_back(moduleDir + L"\\zapret\\");
    wchar_t cwdBuf[MAX_PATH] = {};
    if (GetCurrentDirectoryW(MAX_PATH, cwdBuf) > 0) {
        bases.push_back(std::wstring(cwdBuf) + L"\\client\\zapret\\");
        bases.push_back(std::wstring(cwdBuf) + L"\\zapret\\");
    }
    bases.push_back(moduleDir + L"\\..\\..\\..\\..\\client\\zapret\\");

    for (auto &b : bases) {
        std::wstring p = b + fileName;
        if (GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES) return p;
    }
    return L"";
}

namespace {
    std::wstring GetCurrentModuleDir() {
        wchar_t buffer[MAX_PATH] = {};
        DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        if (len == 0 || len >= MAX_PATH) return L".";
        std::wstring path(buffer);
        size_t pos = path.find_last_of(L"\\/");
        if (pos == std::wstring::npos) return L".";
        return path.substr(0, pos);
    }

    // Поиск .bat файла для стратегии ALT с указанным номером в папке client\zapret
    std::wstring FindAltBatch(int altIndex) {
        if (altIndex <= 0) return L"";
    // Специальные сопоставления для TLS-стратегий, добавленных в UI:
    // Индексы 13,14,15 в интерфейсе соответствуют файлам
    // "general (FAKE TLS AUTO ALT).bat", "general (FAKE TLS AUTO ALT2).bat", "general (FAKE TLS AUTO ALT3).bat"
    if (altIndex == 13) {
        auto p = FindFileInZapret(L"general (FAKE TLS AUTO ALT).bat");
        if (!p.empty()) return p;
    } else if (altIndex == 14) {
        auto p = FindFileInZapret(L"general (FAKE TLS AUTO ALT2).bat");
        if (!p.empty()) return p;
    } else if (altIndex == 15) {
        auto p = FindFileInZapret(L"general (FAKE TLS AUTO ALT3).bat");
        if (!p.empty()) return p;
    }
    // Попробовать несколько относительных базовых директорий, т.к. исполняемый файл может находиться
    // в папке x64\Debug и путь до исходной папки client\zapret отличается
    std::vector<std::wstring> bases;
    // Принудительно добавить путь к корню проекта, как просил пользователь:
    // <Drive>\Jaylee client\client\zapret\ (например D:\Jaylee client\client\zapret\)
    std::wstring moduleDir = GetCurrentModuleDir();
    if (moduleDir.size() >= 2 && moduleDir[1] == L':') {
        std::wstring drive = moduleDir.substr(0, 2); // например "D:"
        // Основной путь к папке zapret
        bases.push_back(drive + L"\\Jaylee client\\client\\zapret\\");
        // Запасной (исторический) вариант с дублированием папки проекта
        bases.push_back(drive + L"\\Jaylee client\\Jaylee client\\client\\zapret\\");
    }
    bases.push_back(moduleDir + L"\\client\\zapret\\");
    bases.push_back(GetCurrentModuleDir() + L"\\..\\client\\zapret\\");
    bases.push_back(GetCurrentModuleDir() + L"\\..\\..\\client\\zapret\\");
    bases.push_back(GetCurrentModuleDir() + L"\\..\\..\\..\\client\\zapret\\");
    bases.push_back(GetCurrentModuleDir() + L"\\zapret\\");
    // Текущий рабочий каталог — иногда exe запускают из папки сборки
    wchar_t cwdBuf[MAX_PATH] = {};
    if (GetCurrentDirectoryW(MAX_PATH, cwdBuf) > 0) {
        bases.push_back(std::wstring(cwdBuf) + L"\\client\\zapret\\");
        bases.push_back(std::wstring(cwdBuf) + L"\\zapret\\");
    }
    // Ещё варианты — поднимемся выше ещё на один уровень (на случай запуска из bin/Debug)
    bases.push_back(GetCurrentModuleDir() + L"\\..\\..\\..\\..\\client\\zapret\\");

    for (auto &dir : bases) {
        // рекурсивный поиск в каталоге (и в подкаталогах глубины 2)
        std::function<std::wstring(const std::wstring&, int)> searchDir;
        searchDir = [&](const std::wstring &searchDirPath, int depth) -> std::wstring {
            // сначала файлы в текущем каталоге
            WIN32_FIND_DATAW fd;
            std::wstring fileSearch = searchDirPath + L"\\*.bat";
            HANDLE h = FindFirstFileW(fileSearch.c_str(), &fd);
            if (h != INVALID_HANDLE_VALUE) {
                do {
                    std::wstring name = fd.cFileName;
                    // нормализуем имя для поиска цифры после ALT (регистронезависимо)
                    std::wstring up = name;
                    for (auto &ch : up) ch = towupper(ch);
                    size_t p = up.find(L"ALT");
                    if (p != std::wstring::npos) {
                        // найти число после ALT
                        size_t q = p + 3;
                        while (q < up.size() && !iswdigit(up[q])) q++;
                        if (q < up.size() && iswdigit(up[q])) {
                            int val = 0;
                            size_t r = q;
                            while (r < up.size() && iswdigit(up[r])) {
                                val = val * 10 + (up[r] - L'0');
                                r++;
                            }
                            if (val == altIndex) {
                                FindClose(h);
                                return searchDirPath + name;
                            }
                        } else {
                            // без числа — подойдёт для ALT == 1
                            if (altIndex == 1) {
                                // убедимся, что после ALT не буква/цифра
                                if (q >= up.size() || !iswalnum(up[q])) {
                                    FindClose(h);
                                    return searchDirPath + name;
                                }


                            }
                        }
                    }
                    // также поддерживаем форматы general ALTn без скобок
                    std::wstring upName = up;
                    if (upName.find(L"GENERAL") != std::wstring::npos) {
                        // попытаться извлечь число в имени
                        for (size_t i = 0; i < upName.size(); ++i) {
                            if (iswdigit(upName[i])) {
                                // соберём число
                                int val = upName[i] - L'0';
                                size_t j = i + 1;
                                while (j < upName.size() && iswdigit(upName[j])) { val = val * 10 + (upName[j] - L'0'); j++; }
                                if (val == altIndex) {
                                    FindClose(h);
                                    return searchDirPath + name;
                                }
                                break;
                            }
                        }
                    }
                } while (FindNextFileW(h, &fd));
                FindClose(h);
            }

            // рекурсивно пройтись по подкаталогам, если глубина позволяет
            if (depth > 0) {
                std::wstring dirSearch = searchDirPath + L"\\*";
                HANDLE dh = FindFirstFileW(dirSearch.c_str(), &fd);
                if (dh != INVALID_HANDLE_VALUE) {
                    do {
                        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && wcscmp(fd.cFileName, L".") != 0 && wcscmp(fd.cFileName, L"..") != 0) {
                            std::wstring sub = searchDirPath + L"\\" + fd.cFileName;
                            std::wstring r = searchDir(sub, depth - 1);
                            if (!r.empty()) {
                                FindClose(dh);
                                return r;
                            }
                        }
                    } while (FindNextFileW(dh, &fd));
                    FindClose(dh);
                }
            }
            return L"";
        };

        // попробовать быстрый точный поиск по ожидаемым именам (без рекурсии)
        wchar_t bufExact[MAX_PATH];
        swprintf_s(bufExact, _countof(bufExact), L"%lsgeneral (ALT%d).bat", dir.c_str(), altIndex);
        if (GetFileAttributesW(bufExact) != INVALID_FILE_ATTRIBUTES) return std::wstring(bufExact);
        swprintf_s(bufExact, _countof(bufExact), L"%lsgeneral (ALT %d).bat", dir.c_str(), altIndex);
        if (GetFileAttributesW(bufExact) != INVALID_FILE_ATTRIBUTES) return std::wstring(bufExact);
        if (altIndex == 1) {
            swprintf_s(bufExact, _countof(bufExact), L"%lsgeneral (ALT).bat", dir.c_str());
            if (GetFileAttributesW(bufExact) != INVALID_FILE_ATTRIBUTES) return std::wstring(bufExact);
        }

        // попробовать рекурсивный поиск до глубины 2
        std::wstring rec = searchDir(dir, 2);
        if (!rec.empty()) return rec;
        // нормализовать путь (не обязательно)
        WIN32_FIND_DATAW fd;
        std::wstring search = dir + L"*.bat";
        HANDLE h = FindFirstFileW(search.c_str(), &fd);
        if (h == INVALID_HANDLE_VALUE) {
            // попробовать конкретные имена
            // уже пробовали конкретные имена выше
            continue;
        }
        do {
            std::wstring name = fd.cFileName;
            // найти подстроку "ALT" и число после неё
            size_t p = name.find(L"ALT");
            if (p != std::wstring::npos) {
                // пропустить нецифровые символы
                size_t q = p + 3;
                while (q < name.size() && !iswdigit(name[q])) q++;
                if (q < name.size() && iswdigit(name[q])) {
                    int val = 0;
                    size_t r = q;
                    while (r < name.size() && iswdigit(name[r])) {
                        val = val * 10 + (name[r] - L'0');
                        r++;
                    }
                    if (val == altIndex) {
                        FindClose(h);
                        return dir + name;
                    }
                } else {
                    // Если после ALT нет числа, но стоит немаркёрный символ (например ")"),
                    // это может быть файл "general (ALT).bat" — сопоставим его со стратегией 1
                    if (altIndex == 1) {
                        // убедиться, что следующий символ не буквенно-цифровой (чтобы не поймать "ALTexp")
                        if (q < name.size()) {
                            if (!iswalnum(name[q])) {
                                FindClose(h);
                                return dir + name;
                            }
                        } else {
                            // конец строки сразу после ALT
                            FindClose(h);
                            return dir + name;
                        }
                    }
                }
            }
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }

    return L"";
    }

    // Убить все процессы с именем winws.exe
    void KillWinwsProcesses() {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return;
        PROCESSENTRY32W pe{};
        pe.dwSize = sizeof(pe);
        if (Process32FirstW(snap, &pe)) {
            do {
                std::wstring exe = pe.szExeFile;
                for (auto &c : exe) c = towlower(c);
                if (exe == L"winws.exe") {
                    HANDLE ph = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                    if (ph) {
                        TerminateProcess(ph, 0);
                        CloseHandle(ph);
                    }
                }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
    }
}

static int g_selectedAlt = 0;
static PROCESS_INFORMATION g_batchPi = {};
static bool g_running = false;
static bool g_uiBypassRunning = false;  // True когда пользователь нажал "Запустить" и получил успех

static DWORD g_lastCreateProcessError = 0;

void OpenSettingsFromLogic()
{
    // Пробрасываем вызов в визуальный модуль, который создаёт и показывает окно
    ShowSettingsWindow();
}

void OpenAboutFromLogic()
{
    // Перенаправляем в визуальный модуль, который создаёт окно "О программе"
    ShowAboutWindow();
}

void OpenServiceBatchFromLogic()
{
    std::wstring found = FindFileInZapret(L"service.bat");
    if (found.empty()) {
        MessageBoxW(NULL, L"Файл service.bat не найден в папке client\\zapret. Проверьте расположение проекта.", L"Файл не найден", MB_OK | MB_ICONERROR);
        return;
    }

    // Попробовать запустить батник через ShellExecute
    HINSTANCE h = ShellExecuteW(NULL, L"open", found.c_str(), NULL, NULL, SW_SHOWNORMAL);
    if ((INT_PTR)h <= 32) {
        // Попытаться запустить через cmd.exe
        std::wstring cmd = L"/c \"" + found + L"\"";
        SHELLEXECUTEINFOW sei{};
        sei.cbSize = sizeof(sei);
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        sei.hwnd = NULL;
        sei.lpVerb = L"open";
        sei.lpFile = L"cmd.exe";
        sei.lpParameters = cmd.c_str();
        sei.nShow = SW_SHOW;
        ShellExecuteExW(&sei);
    }
}

// Открыть файл list-general.txt из папки client\zapret\lists
void OpenListFileFromLogic()
{
    std::wstring moduleDir = GetCurrentModuleDir();
    std::vector<std::wstring> bases;
    if (moduleDir.size() >= 2 && moduleDir[1] == L':') {
        std::wstring drive = moduleDir.substr(0, 2);
        // Основной путь к папке lists
        bases.push_back(drive + L"\\Jaylee client\\client\\zapret\\lists\\");
        // Запасной (исторический) вариант с дублированием папки проекта
        bases.push_back(drive + L"\\Jaylee client\\Jaylee client\\client\\zapret\\lists\\");
    }
    bases.push_back(moduleDir + L"\\client\\zapret\\lists\\");
    bases.push_back(GetCurrentModuleDir() + L"\\..\\client\\zapret\\lists\\");
    bases.push_back(GetCurrentModuleDir() + L"\\..\\..\\client\\zapret\\lists\\");
    bases.push_back(GetCurrentModuleDir() + L"\\zapret\\lists\\");
    wchar_t cwdBuf[MAX_PATH] = {};
    if (GetCurrentDirectoryW(MAX_PATH, cwdBuf) > 0) {
        bases.push_back(std::wstring(cwdBuf) + L"\\client\\zapret\\lists\\");
        bases.push_back(std::wstring(cwdBuf) + L"\\zapret\\lists\\");
    }

    std::wstring targetName = L"list-general.txt";
    std::wstring found;
    for (auto &b : bases) {
        std::wstring p = b + targetName;
        if (GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES) {
            found = p;
            break;
        }
    }

    if (found.empty()) {
        // Попробовать открыть относительный путь прямо (на случай, если текущая рабочая директория — корень проекта)
        std::wstring rel = moduleDir + L"\\..\\..\\client\\zapret\\lists\\" + targetName;
        if (GetFileAttributesW(rel.c_str()) != INVALID_FILE_ATTRIBUTES) found = rel;
    }

    if (found.empty()) {
        MessageBoxW(NULL, L"Файл list-general.txt не найден в папке client\\zapret\\lists. Проверьте расположение проекта.", L"Файл не найден", MB_OK | MB_ICONERROR);
        return;
    }

    // Открыть файл в ассоциированном приложении (обычно Блокнот)
    HINSTANCE h = ShellExecuteW(NULL, L"open", found.c_str(), NULL, NULL, SW_SHOW);
    // Если не удалось, показать сообщение
    if ((INT_PTR)h <= 32) {
        MessageBoxW(NULL, L"Не удалось открыть файл списка. Проверьте права доступа.", L"Ошибка", MB_OK | MB_ICONERROR);
    }
}

void SelectStrategy(int altIndex)
{
    bool wasRunning = IsBypassRunning();
    g_selectedAlt = altIndex;
    SaveSettingsToConfig();

    if (wasRunning) {
        if (altIndex != 0) {
            StartSelectedBypass();
        } else {
            StopBypass();
        }
    }
}

int GetSelectedStrategy()
{
    return g_selectedAlt;
}

bool StartSelectedBypass()
{
    if (g_selectedAlt == 0) return false;

    if (IsBypassRunning()) {
        StopBypass();
        Sleep(200); // Wait briefly to let WinDivert handles release
    }

    std::wstring bat = FindAltBatch(g_selectedAlt);
    if (bat.empty()) return false;

    // Попробуем запустить winws.exe напрямую через HiddenWinwsStarter, разобрав .bat.
    bool started = false;
    std::wstring exePath;
    std::wstring args;
    std::wstring batDir;

    {
        // Считать файл .bat и собрать одну строку, учитывая продолжения с '^'
        std::wstring joined;
        std::wifstream ifs(bat);
        if (ifs.good()) {
            std::wstring line;
            while (std::getline(ifs, line)) {
                // удалить CR если есть
                if (!line.empty() && line.back() == L'\r') line.pop_back();
                // если строка кончается на ^ — удалить и продолжить
                size_t i = line.find_last_not_of(L" \t");
                if (i != std::wstring::npos && line[i] == L'^') {
                    line.resize(i); // уберём ^ и пробелы
                    joined += line;
                } else {
                    joined += line + L' ';
                }
            }
        }
        if (!joined.empty()) {
            // Найти упоминание winws.exe
            size_t pos = joined.find(L"winws.exe");
            if (pos != std::wstring::npos) {
                // Попытаться вычислить путь к exe и аргументы
                // Найдём ближайшие кавычки вокруг пути
                size_t left = joined.rfind(L'"', pos);
                size_t right = (left != std::wstring::npos) ? joined.find(L'"', left + 1) : std::wstring::npos;
                std::wstring exeToken;
                if (left != std::wstring::npos && right != std::wstring::npos && right > left) {
                    exeToken = joined.substr(left + 1, right - left - 1);
                }

                // Получим директорию .bat
                size_t p = bat.find_last_of(L"\\/");
                batDir = (p == std::wstring::npos) ? L"." : bat.substr(0, p + 1);

                // Создадим необходимые пользовательские списки, если их нет
                std::wstring listsPath = batDir + L"lists\\";
                auto ensureFile = [](const std::wstring& path, const std::string& defaultContent) {
                    if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) {
                        std::ofstream ofs(path);
                        if (ofs.is_open()) ofs << defaultContent;
                    }
                };
                ensureFile(listsPath + L"ipset-exclude-user.txt", "203.0.113.113/32\n");
                ensureFile(listsPath + L"list-general-user.txt", "# Never leave this file empty\ndomain.example.abc\n");
                ensureFile(listsPath + L"list-exclude-user.txt", "domain.example.abc\n");

                // Прочитаем режим GameFilter
                std::wstring gameFilterTCP = L"12";
                std::wstring gameFilterUDP = L"12";
                std::wstring gameFlagFile = batDir + L"utils\\game_filter.enabled";
                if (GetFileAttributesW(gameFlagFile.c_str()) != INVALID_FILE_ATTRIBUTES) {
                    std::wifstream gff(gameFlagFile);
                    if (gff.good()) {
                        std::wstring mode;
                        gff >> mode;
                        for (auto& c : mode) c = towlower(c);
                        if (mode == L"all") {
                            gameFilterTCP = L"1024-65535";
                            gameFilterUDP = L"1024-65535";
                        } else if (mode == L"tcp") {
                            gameFilterTCP = L"1024-65535";
                        } else if (mode == L"udp") {
                            gameFilterUDP = L"1024-65535";
                        }
                    }
                }

                auto replaceAll = [&](std::wstring &s, const std::wstring &from, const std::wstring &to) {
                    if (from.empty()) return;
                    size_t pos = 0;
                    while (pos <= s.length() && pos + from.length() <= s.length()) {
                        bool match = true;
                        for (size_t i = 0; i < from.length(); ++i) {
                            if (towupper(s[pos + i]) != towupper(from[i])) {
                                match = false;
                                break;
                            }
                        }
                        if (match) {
                            s.replace(pos, from.length(), to);
                            pos += to.length();
                        } else {
                            pos++;
                        }
                    }
                };
                std::wstring binPath = batDir + L"bin\\";

                // Попробуем сформировать путь к winws.exe
                if (!exeToken.empty()) {
                    replaceAll(exeToken, L"%BIN%", binPath);
                    replaceAll(exeToken, L"%~dp0", batDir);
                    replaceAll(exeToken, L"%LISTS%", listsPath);
                    exePath = exeToken;
                } else {
                    exePath = batDir + L"bin\\winws.exe";
                }

                // Аргументы — остаток после пути
                if (right != std::wstring::npos) {
                    args = joined.substr(right + 1);
                } else if (!exeToken.empty()) {
                    size_t endPos = pos + wcslen(L"winws.exe");
                    if (endPos < joined.size()) args = joined.substr(endPos);
                }

                // Заменим переменные в args
                if (!args.empty()) {
                    replaceAll(args, L"%BIN%", binPath);
                    replaceAll(args, L"%~dp0", batDir);
                    replaceAll(args, L"%LISTS%", listsPath);
                    replaceAll(args, L"%GameFilterTCP%", gameFilterTCP);
                    replaceAll(args, L"%GameFilterUDP%", gameFilterUDP);
                }

                // Убрать лишние кавычки и пробелы
                auto trim = [&](std::wstring &s) {
                    size_t a = s.find_first_not_of(L" \t\r\n");
                    size_t b = s.find_last_not_of(L" \t\r\n");
                    if (a == std::wstring::npos) { s.clear(); return; }
                    s = s.substr(a, b - a + 1);
                };
                trim(exePath);
                trim(args);

                // Если путь относительный — сделать абсолютным относительно batDir
                if (!exePath.empty() && exePath.size() >= 2 && exePath[1] != L':') {
                    exePath = batDir + exePath;
                }

                // Проверить существование
                if (!exePath.empty() && GetFileAttributesW(exePath.c_str()) != INVALID_FILE_ATTRIBUTES) {
                    // Вызвать HiddenWinwsStarter
                    std::wstring finalArgs = L"\"" + exePath + L"\" " + args;
                    auto res = HiddenWinwsStarter::Start(exePath, finalArgs);
                    if (res.success) {
                        g_batchPi.hProcess = res.processHandle;
                        g_batchPi.hThread = nullptr;
                        g_running = true;
                        g_uiBypassRunning = true;
                        started = true;
                    }
                }
            }
        }
    }

    if (started) return true;

    // Fallback: Запускаем winws.exe напрямую через ShellExecuteEx (без cmd.exe и без start)
    // Если exePath не сформирован, используем старый fallback
    if (!exePath.empty() && GetFileAttributesW(exePath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        SHELLEXECUTEINFOW sei{};
        sei.cbSize = sizeof(sei);
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        sei.hwnd = nullptr;
        sei.lpVerb = L"runas"; // В случае, если нужны админские права
        sei.lpFile = exePath.c_str();
        sei.lpParameters = args.c_str();
        sei.nShow = SW_HIDE; // Гарантированно скрыть окно

        std::wstring workDir;
        size_t slashPos = exePath.find_last_of(L"\/");
        if (slashPos != std::wstring::npos) {
            workDir = exePath.substr(0, slashPos);
            sei.lpDirectory = workDir.c_str();
        }

        if (ShellExecuteExW(&sei) && sei.hProcess) {
            g_batchPi.hProcess = sei.hProcess;
            g_batchPi.hThread = nullptr;
            g_running = true;
            g_uiBypassRunning = true;
            return true;
        }
    }

    // Если всё провалилось — старый fallback (через cmd.exe)
    std::wstring params = L"/C "" + bat + L""";
    SHELLEXECUTEINFOW sei{};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.hwnd = nullptr;
    sei.lpVerb = L"open";
    sei.lpFile = L"cmd.exe";
    sei.lpParameters = params.c_str();
    sei.nShow = SW_HIDE;

    if (!ShellExecuteExW(&sei) || !sei.hProcess) {
        g_lastCreateProcessError = GetLastError();
        if (sei.hProcess) CloseHandle(sei.hProcess);
        g_uiBypassRunning = false;
        return false;
    }

    // Сохранить handle процесса, чтобы можно было остановить
    g_batchPi.hProcess = sei.hProcess;
    g_batchPi.hThread = nullptr;
    g_running = true;
    g_uiBypassRunning = true;
    return true;
}

void StopBypass()
{
    OutputDebugStringW(L"StopBypass: called\n");
    if (g_batchPi.hProcess) {
        DWORD code = 0;
        if (GetExitCodeProcess(g_batchPi.hProcess, &code) && code == STILL_ACTIVE) {
            TerminateProcess(g_batchPi.hProcess, 0);
        }
        CloseHandle(g_batchPi.hProcess);
        g_batchPi = {};
    }
    // Убедиться, что также завершены процессы winws.exe, если они были запущены
    KillWinwsProcesses();
    g_lastCreateProcessError = 0;
    g_running = false;
    g_uiBypassRunning = false;
    OutputDebugStringW(L"StopBypass: finished\n");
}

bool IsBypassRunning()
{
    if (g_running) return true;
    if (g_batchPi.hProcess) {
        DWORD code = 0;
        if (GetExitCodeProcess(g_batchPi.hProcess, &code) && code == STILL_ACTIVE) return true;
    }
    return false;
}

bool CheckBypassStatus(std::wstring &outMessage)
{
    if (IsBypassRunning()) {
        outMessage = L"OK: bypass is running.";
        return true;
    }
    if (g_selectedAlt == 0) {
        outMessage = L"Ошибка: стратегия не выбрана.";
        return false;
    }
    std::wstring bat = FindAltBatch(g_selectedAlt);
    if (bat.empty()) {
        outMessage = L"Ошибка: .bat для выбранной стратегии не найден. Попробуйте проверить папку client\\zapret относительно каталога приложения.";
        return false;
    }
    if (g_lastCreateProcessError != 0) {
        wchar_t buf[512];
        swprintf_s(buf, _countof(buf), L"Ошибка CreateProcess/ShellExecuteEx: код=%lu.", g_lastCreateProcessError);
        outMessage = buf;
        return false;
    }
    outMessage = L"Ошибка: .bat найден, но процесс не запущен.";
    return false;
}

void ReportBypassStatusIfNotRunning()
{
    std::wstring msg;
    if (!CheckBypassStatus(msg)) {
        wchar_t tmpPath[MAX_PATH] = {};
        if (GetTempPathW(MAX_PATH, tmpPath) > 0) {
            std::wstring filePath = std::wstring(tmpPath) + L"Jaylee_bypass_status.txt";
            HANDLE h = CreateFileW(filePath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (h != INVALID_HANDLE_VALUE) {
                DWORD written = 0;
                const unsigned char bomUtf8[] = { 0xEF, 0xBB, 0xBF };
                WriteFile(h, bomUtf8, sizeof(bomUtf8), &written, nullptr);
                int required = WideCharToMultiByte(CP_UTF8, 0, msg.c_str(), static_cast<int>(msg.size()), nullptr, 0, nullptr, nullptr);
                if (required > 0) {
                    std::string utf8;
                    utf8.resize(required);
                    WideCharToMultiByte(CP_UTF8, 0, msg.c_str(), static_cast<int>(msg.size()), utf8.data(), required, nullptr, nullptr);
                    DWORD wrote2 = 0;
                    if (!utf8.empty()) WriteFile(h, utf8.data(), static_cast<DWORD>(utf8.size()), &wrote2, nullptr);
                }
                CloseHandle(h);
                std::wstring ansiPath = std::wstring(tmpPath) + L"Jaylee_bypass_status_ansi.txt";
                HANDLE h2 = CreateFileW(ansiPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
                if (h2 != INVALID_HANDLE_VALUE) {
                    int reqA = WideCharToMultiByte(CP_ACP, 0, msg.c_str(), static_cast<int>(msg.size()), nullptr, 0, nullptr, nullptr);
                    if (reqA > 0) {
                        std::string ansi;
                        ansi.resize(reqA);
                        WideCharToMultiByte(CP_ACP, 0, msg.c_str(), static_cast<int>(msg.size()), ansi.data(), reqA, nullptr, nullptr);
                        DWORD wroteA = 0;
                        if (!ansi.empty()) WriteFile(h2, ansi.data(), static_cast<DWORD>(ansi.size()), &wroteA, nullptr);
                    }
                    CloseHandle(h2);
                    ShellExecuteW(NULL, L"open", L"notepad.exe", ansiPath.c_str(), nullptr, SW_SHOW);
                    return;
                }
            }
        }
        MessageBoxW(NULL, L"Диагностика не удалась. Проверьте права и попробуйте запустить программу от имени администратора.", L"Ошибка обхода блокировок", MB_OK | MB_ICONERROR);
    }
}

// (удалён дубликат реализации ReportBypassStatusIfNotRunning)

// Возвращает сохранённое состояние UI
bool GetUIBypassRunningState()
{
    return g_uiBypassRunning;
}

// Улучшенная функция проверки состояния
bool CheckBypassStatusEnhanced(std::wstring& outMessage, DWORD* pidOut)
{
    if (g_batchPi.hProcess) {
        DWORD code = 0;
        if (GetExitCodeProcess(g_batchPi.hProcess, &code) && code == STILL_ACTIVE) {
            if (pidOut) *pidOut = GetProcessId(g_batchPi.hProcess);
            wchar_t msg[256];
            swprintf_s(msg, _countof(msg), L"OK: батник запущен (PID: %lu).", GetProcessId(g_batchPi.hProcess));
            outMessage = msg;
            return true;
        }
    }
    outMessage = L"Процесс батника не найден.";
    return false;
}