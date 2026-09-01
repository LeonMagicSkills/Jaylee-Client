#include "pch.h"
#include "hiddenWinwsStarter.h"
#include <windows.h>
#include <string>
#include <vector>
#include <utility>
#include <tlhelp32.h>
#include <algorithm>
#include <atomic>

static std::wstring FormatLastErrorMessage(DWORD err)
{
	if (err == 0) return L"";
	LPWSTR msgBuf = nullptr;
	DWORD size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
								nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&msgBuf, 0, nullptr);
	std::wstring msg;
	if (size && msgBuf) {
		msg.assign(msgBuf, msgBuf + size);
		LocalFree(msgBuf);
	}
	return msg;
}

struct WindowInfo {
	HWND hwnd;
	std::wstring className;
	std::wstring title;
};

struct EnumWindowsParam {
	DWORD targetPid;
	std::vector<WindowInfo>* out;
};

static BOOL CALLBACK EnumWindowsProcCallback(HWND hwnd, LPARAM lParam)
{
	EnumWindowsParam* param = reinterpret_cast<EnumWindowsParam*>(lParam);
	if (!param || !param->out) return TRUE;

	DWORD pid = 0;
	GetWindowThreadProcessId(hwnd, &pid);
	if (pid != param->targetPid) return TRUE; // continue

	// consider only visible top-level windows
	if (!IsWindowVisible(hwnd)) return TRUE;
	if (GetWindow(hwnd, GW_OWNER) != nullptr) return TRUE;

	WindowInfo info{};
	info.hwnd = hwnd;

	wchar_t cls[256] = {};
	GetClassNameW(hwnd, cls, _countof(cls));
	info.className = cls;

	wchar_t title[512] = {};
	GetWindowTextW(hwnd, title, _countof(title));
	info.title = title;

	param->out->push_back(info);
	return TRUE;
}

static std::vector<WindowInfo> GetTopLevelVisibleWindowsForPid(DWORD pid)
{
	std::vector<WindowInfo> found;
	EnumWindowsParam param;
	param.targetPid = pid;
	param.out = &found;
	EnumWindows(EnumWindowsProcCallback, reinterpret_cast<LPARAM>(&param));
	return found;
}

static std::vector<DWORD> GetChildProcessesByParent(DWORD parentPid)
{
	std::vector<DWORD> children;
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snap == INVALID_HANDLE_VALUE) return children;

	PROCESSENTRY32W pe;
	pe.dwSize = sizeof(pe);
	if (Process32FirstW(snap, &pe)) {
		do {
			if (pe.th32ParentProcessID == parentPid) {
				children.push_back(pe.th32ProcessID);
			}
		} while (Process32NextW(snap, &pe));
	}
	CloseHandle(snap);
	return children;
}

static std::wstring GetProcessImageNameByPid(DWORD pid)
{
	std::wstring name;
	HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	if (!h) return name;
	wchar_t buf[MAX_PATH] = {};
	DWORD sz = _countof(buf);
	if (QueryFullProcessImageNameW(h, 0, buf, &sz)) {
		name = buf;
	}
	CloseHandle(h);
	return name;
}

HiddenWinwsStarter::StartResult HiddenWinwsStarter::Start(const std::wstring& exePath, const std::wstring& args)
{
	StartResult result;

	if (exePath.empty()) {
		result.errorMessage = L"Executable path is empty";
		return result;
	}

	// Формируем командную строку: в первую позицию ставим путь к исполняемому файлу в кавычках,
	// затем добавляем переданные аргументы (без изменений).
	std::wstring cmdLine;
	cmdLine.reserve(exePath.size() + args.size() + 4);
	cmdLine.push_back(L'"');
	cmdLine.append(exePath);
	cmdLine.push_back(L'"');
	if (!args.empty()) {
		cmdLine.push_back(L' ');
		cmdLine.append(args);
	}

	// CreateProcessW требует изменяемую буферную строку для lpCommandLine
	std::vector<wchar_t> cmdBuf(cmdLine.begin(), cmdLine.end());
	cmdBuf.push_back(L'\0');

	// Открываем NUL для перенаправления std handles
	HANDLE hNullIn = CreateFileW(L"NUL:", GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	HANDLE hNullOut = CreateFileW(L"NUL:", GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	HANDLE hNullErr = CreateFileW(L"NUL:", GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	// Подготовим расширённую STARTUPINFOEX, чтобы ограничить наследуемые дескрипторы только NUL
	STARTUPINFOEXW siex;
	PROCESS_INFORMATION pi;
	ZeroMemory(&siex, sizeof(siex));
	siex.StartupInfo.cb = sizeof(siex);

	// Скрыть окно
	siex.StartupInfo.dwFlags |= STARTF_USESHOWWINDOW;
	siex.StartupInfo.wShowWindow = SW_HIDE;

	// Список дескрипторов, которые будут явно унаследованы (включаем только валидные)
	HANDLE inheritListArr[3];
	SIZE_T inheritCount = 0;
	if (hNullIn != INVALID_HANDLE_VALUE) inheritListArr[inheritCount++] = hNullIn;
	if (hNullOut != INVALID_HANDLE_VALUE) inheritListArr[inheritCount++] = hNullOut;
	if (hNullErr != INVALID_HANDLE_VALUE) inheritListArr[inheritCount++] = hNullErr;

	bool usedAttrList = false;
	SIZE_T attrListSize = 0;
	LPPROC_THREAD_ATTRIBUTE_LIST attrList = nullptr;

	if (inheritCount > 0) {
		// Инициализация списка атрибутов: сначала получаем размер
		InitializeProcThreadAttributeList(nullptr, 1, 0, &attrListSize);
		if (attrListSize > 0) {
			attrList = (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), 0, attrListSize);
			if (attrList) {
				if (InitializeProcThreadAttributeList(attrList, 1, 0, &attrListSize)) {
					// Установим список дескрипторов для наследования
					if (UpdateProcThreadAttribute(attrList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, inheritListArr, inheritCount * sizeof(HANDLE), nullptr, nullptr)) {
						siex.lpAttributeList = attrList;
						usedAttrList = true;
						// Установим стандартные дескрипторы только когда атрибут-лист успешно создан
						siex.StartupInfo.dwFlags |= STARTF_USESTDHANDLES;
						siex.StartupInfo.hStdInput = (hNullIn != INVALID_HANDLE_VALUE) ? hNullIn : nullptr;
						siex.StartupInfo.hStdOutput = (hNullOut != INVALID_HANDLE_VALUE) ? hNullOut : nullptr;
						siex.StartupInfo.hStdError = (hNullErr != INVALID_HANDLE_VALUE) ? hNullErr : nullptr;
					}
				}
			}
		}
	}

	ZeroMemory(&pi, sizeof(pi));

	DWORD creationFlags = CREATE_NO_WINDOW;
	if (usedAttrList) {
		creationFlags |= EXTENDED_STARTUPINFO_PRESENT;
	}

	std::wstring workDir;
	size_t slashPos = exePath.find_last_of(L"\\/");
	if (slashPos != std::wstring::npos) {
		workDir = exePath.substr(0, slashPos);
	}

	BOOL ok = CreateProcessW(
		exePath.c_str(),
		cmdBuf.data(),
		nullptr,
		nullptr,
		usedAttrList ? TRUE : FALSE, // bInheritHandles: TRUE чтобы унаследовать только те дескрипторы, которые указали в attrList
		creationFlags,
		nullptr,
		workDir.empty() ? nullptr : workDir.c_str(),
		reinterpret_cast<LPSTARTUPINFOW>(&siex),
		&pi
	);

	DWORD lastErr = ok ? 0 : GetLastError();

	// Освобождаем список атрибутов
	if (attrList) {
		if (usedAttrList) {
			DeleteProcThreadAttributeList(attrList);
		}
		HeapFree(GetProcessHeap(), 0, attrList);
		attrList = nullptr;
	}

	// Закрываем наши временные NUL дескрипторы — дочерний процесс уже унаследовал их копии
	if (hNullIn && hNullIn != INVALID_HANDLE_VALUE) CloseHandle(hNullIn);
	if (hNullOut && hNullOut != INVALID_HANDLE_VALUE) CloseHandle(hNullOut);
	if (hNullErr && hNullErr != INVALID_HANDLE_VALUE) CloseHandle(hNullErr);

	if (!ok) {
		result.errorMessage = FormatLastErrorMessage(lastErr);
		if (result.errorMessage.empty()) {
			result.errorMessage = L"CreateProcess failed with code: " + std::to_wstring(lastErr);
		}
		return result;
	}

	// После создания процесса — выполнем post-launch validation
	DWORD childPid = pi.dwProcessId;

	// Небольшая пауза, чтобы дочерний процесс успел инициализироваться
	const int maxWaitMs = 1500;
	const int stepMs = 100;
	int waited = 0;
	bool validationFailed = false;
	std::wstring validationMsg;

	while (waited < maxWaitMs) {
		// 1) Проверим, не появились ли окна верхнего уровня, принадлежащие childPid
		auto wins = GetTopLevelVisibleWindowsForPid(childPid);
		if (!wins.empty()) {
			validationFailed = true;
			validationMsg = L"Visible window(s) found for pid=" + std::to_wstring(childPid) + L": ";
			for (auto &w : wins) {
				validationMsg += L"[class='" + w.className + L"' title='" + w.title + L"'] ";
			}
			break;
		}

		// 2) Проверим дочерние процессы: conhost, cmd, powershell
		auto children = GetChildProcessesByParent(childPid);
		for (auto cpid : children) {
			std::wstring img = GetProcessImageNameByPid(cpid);
			std::wstring lower;
			lower.resize(img.size());
			std::transform(img.begin(), img.end(), lower.begin(), ::towlower);
			// extract filename only
			size_t pos = lower.find_last_of(L"\\/");
			std::wstring fname = (pos == std::wstring::npos) ? lower : lower.substr(pos + 1);
			if (fname == L"conhost.exe" || fname == L"cmd.exe" || fname == L"powershell.exe") {
				validationFailed = true;
				validationMsg = L"Unexpected helper process spawned by winws.exe: " + fname;
				break;
			}
			// additionally check windows owned by these helper processes
			auto hw = GetTopLevelVisibleWindowsForPid(cpid);
			if (!hw.empty()) {
				validationFailed = true;
				validationMsg = L"Visible window(s) found for helper pid=" + std::to_wstring(cpid) + L" of parent=" + std::to_wstring(childPid);
				break;
			}
		}

		if (validationFailed) break;

		// Проверим, не завершился ли процесс с ошибкой (например, из-за неверных аргументов)
		DWORD exitCode = 0;
		if (GetExitCodeProcess(pi.hProcess, &exitCode) && exitCode != STILL_ACTIVE) {
			validationFailed = true;
			validationMsg = L"winws.exe exited immediately with code " + std::to_wstring(exitCode) + L" (Possibly invalid arguments or missing files)";
			break;
		}

		// Wait and re-check
		Sleep(stepMs);
		waited += stepMs;
	}

	if (validationFailed) {
		// Если проверка провалилась — завершаем процесс, чтобы не оставлять видимую консоль.
		DWORD termErr = 0;
		if (TerminateProcess(pi.hProcess, 1)) {
			// дождёмся завершения
			WaitForSingleObject(pi.hProcess, 2000);
		} else {
			termErr = GetLastError();
		}

		// Очистка дескрипторов
		if (pi.hThread) CloseHandle(pi.hThread);
		if (pi.hProcess) CloseHandle(pi.hProcess);

		// Сообщаем ошибку
		result.errorMessage = L"Post-launch validation failed: ";
		result.errorMessage += validationMsg;
		if (termErr) result.errorMessage += L" (TerminateProcess failed: " + std::to_wstring(termErr) + L")";
		return result;
	}

	// Успешный запуск и валидация — передаём дескриптор процесса вызывающему коду (он отвечает за закрытие)
	result.success = true;
	result.processId = pi.dwProcessId;
	result.processHandle = pi.hProcess;

	if (pi.hThread) {
		CloseHandle(pi.hThread);
	}

	return result;
}