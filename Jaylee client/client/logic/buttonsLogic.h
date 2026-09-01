#pragma once
#include <string>

// Функция вызывается из UI-обработчика при нажатии на кнопку шестерёнки
void OpenSettingsFromLogic();
// Открыть файл списка list-general.txt из папки client\zapret\lists
void OpenListFileFromLogic();
// Открыть файл service.bat из папки client\zapret
void OpenServiceBatchFromLogic();

// Открыть окно "Информация о программе"
void OpenAboutFromLogic();

void SelectStrategy(int altIndex);
int GetSelectedStrategy();
// Загрузить настройки (включая выбранную стратегию) из configSystem
void LoadSettingsFromConfig();
bool IsAutorunEnabled();
// Toggle autorun. Returns new state: true = enabled, false = disabled
bool ToggleAutorun();

bool IsTrayRunning();
// Toggle tray background process. Returns new state: true = running, false = stopped
bool ToggleTray();

bool IsStrategyAutostartEnabled();
// Toggle visual strategy autostart (placeholder). Returns new state.
bool ToggleStrategyAutostart();

// Запустить/остановить выбранный .bat обхода блокировок
bool StartSelectedBypass();
void StopBypass();

// Возвращает true если UI должен показывать кнопку "Остановить" (красный цвет)
bool GetUIBypassRunningState();

// Проверяет, запущен ли обход блокировок.
bool IsBypassRunning();

// Выполняет расширенную проверку состояния обхода. Если возвращается false, outMessage
// заполняется диагностикой.
bool CheckBypassStatus(std::wstring &outMessage);

// Показывает сообщение об ошибке, если обход не запущен.
void ReportBypassStatusIfNotRunning();

// Расширенная проверка состояния
bool CheckBypassStatusEnhanced(std::wstring& outMessage, DWORD* pidOut = nullptr);