#include "pch.h"
#include "..\\..\\obj\\Debug\\x64\\Generated Files\\winrt\\base.h"
#include "buttonsLogic.h"

#include "autorun.h"
#include "workInTray.h"
#include "configSystem.h"
#include "strategyAutorun.h"
#include <process.h>
#include <windows.h>

namespace {
	static HANDLE s_trayThreadHandle = nullptr;
	static unsigned int s_trayThreadId = 0;
	static bool s_trayRunning = false;
	// Shared state for strategy autostart
	static bool s_strategyAutostartState = false;

	// Thread proc wrapper
	static unsigned __stdcall TrayThreadProc(void* /*arg*/) {
		HINSTANCE hInst = GetModuleHandleW(nullptr);
		int res = workintray::RunTrayBackgroundProcess(hInst, L"Jaylee", L"Jaylee");
		(void)res;
		return 0;
	}
}

bool IsAutorunEnabled() {
	return IsStartupShortcutPresent_C();
}

bool ToggleAutorun() {
	bool present = IsStartupShortcutPresent_C();
	bool res;
	if (present) {
		res = RemoveStartupShortcut_C() ? false : true;
	} else {
		res = CreateStartupShortcut_C() ? true : false;
	}
	SaveSettingsToConfig();
	return res;
}

bool IsTrayRunning() {
	return s_trayRunning;
}

bool ToggleTray() {
	bool res = false;
	if (!s_trayRunning) {
		// start
		uintptr_t th = _beginthreadex(nullptr, 0, TrayThreadProc, nullptr, 0, &s_trayThreadId);
		if (th != 0) {
			s_trayThreadHandle = reinterpret_cast<HANDLE>(th);
			s_trayRunning = true;
			res = true;
		}
	} else {
		// stop
		if (s_trayThreadId != 0) {
			PostThreadMessageW(static_cast<DWORD>(s_trayThreadId), WM_QUIT, 0, 0);
		}
		if (s_trayThreadHandle) {
			WaitForSingleObject(s_trayThreadHandle, 2000);
			CloseHandle(s_trayThreadHandle);
			s_trayThreadHandle = nullptr;
		}
		s_trayThreadId = 0;
		s_trayRunning = false;
	}
	SaveSettingsToConfig();
	return res;
}

bool IsStrategyAutostartEnabled() {
	return s_strategyAutostartState;
}

bool ToggleStrategyAutostart() {
	s_strategyAutostartState = !s_strategyAutostartState;
	if (s_strategyAutostartState) {
		StartStrategyAutorunFromConfig();
	} else {
		StopStrategyAutorun();
	}
	SaveSettingsToConfig();
	return s_strategyAutostartState;
}
