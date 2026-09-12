#pragma once

#include <windows.h>
#include <string>

// HiddenWinwsStarter
// Простая утилита для запуска существующего winws.exe в скрытом режиме.
// Не управляет жизненным циклом процесса — он остаётся за вызывающим кодом.
class HiddenWinwsStarter {
public:
	struct StartResult {
		bool success = false;
		DWORD processId = 0;        // PID процесса (валиден, если success == true)
		HANDLE processHandle = nullptr; // Дескриптор процесса. При успехе дескриптор передаётся вызывающему и ОН ДОЛЖЕН быть закрыт CloseHandle().
		std::wstring errorMessage;  // Текст ошибки при неудаче
	};

	// Запустить winws.exe скрыто.
	// exePath: полный путь до исполняемого файла (может содержать пробелы).
	// args: строка аргументов (как есть) — НЕ ИЗМЕНЯТЬ.
	// Возвращает StartResult. При success == true возвращается processHandle и processId — вызывающий код отвечает за закрытие processHandle.
	static StartResult Start(const std::wstring& exePath, const std::wstring& args);
};
