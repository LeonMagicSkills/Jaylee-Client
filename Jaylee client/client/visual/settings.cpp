#include "pch.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include "settings.h"
// Подключаем панель кнопок настроек
#include "settingsButtons.h"

#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Windows.Foundation.h>
#include <Windows.h>

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml::Media;

// Процедура окна, которая игнорирует WM_CLOSE (блокирует закрытие окна)
static LRESULT CALLBACK SettingsWindowProcImpl(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_CLOSE) return 0;
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void ShowSettingsWindow()
{
    static Window settingsWindow{};
    static bool created = false;

    if (!created) {
        settingsWindow = Window();

        Grid grid{};
        grid.RequestedTheme(ElementTheme::Light);

        // простой белый фон
        winrt::Windows::UI::Color bg{}; bg.A = 0xFF; bg.R = 0xFF; bg.G = 0xFF; bg.B = 0xFF;
        grid.Background(SolidColorBrush{ bg });

        // В окне настроек больше нет центральной надписи

        // Добавить панель кнопок настроек (включая кнопку автозагрузки)
        {
            // Заголовок панели объявлен в settingsButtons.h
            auto settingsButtons = CreateSettingsButtonsPanel();
            grid.Children().Append(settingsButtons);
        }

        settingsWindow.Content(grid);

        try {
            settingsWindow.Title(winrt::hstring(L"\u041D\u0430\u0441\u0442\u0440\u043E\u0439\u043A\u0438"));
        }
        catch (...) {
            // Title может быть недоступно на некоторых конфигурациях WinUI; игнорируем
        }

        settingsWindow.Activate();

        // Подогнать размер окна (если HWND получен по заголовку)
        // Поиск HWND по заголовку — используем ту же Unicode-последовательность
        HWND hwnd = FindWindowW(nullptr, L"\u041D\u0430\u0441\u0442\u0440\u043E\u0439\u043A\u0438");
        if (hwnd) {
            const int width = 380, height = 360;
            SetWindowPos(hwnd, nullptr, 0, 0, width, height, SWP_NOZORDER | SWP_NOMOVE);

            // Убрать кнопки сворачивания и разворачивания, запретить изменение размера
            LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
            style &= ~WS_MAXIMIZEBOX; // убрать кнопку разворачивания
            style &= ~WS_MINIMIZEBOX; // убрать кнопку сворачивания
            style &= ~WS_THICKFRAME;  // запретить изменение размера (граница)
            SetWindowLongPtr(hwnd, GWL_STYLE, style);
            // Обновить оформление окна
            SetWindowPos(hwnd, nullptr, 0, 0, width, height, SWP_NOZORDER | SWP_NOMOVE | SWP_FRAMECHANGED);

            // Удалить пункт "Закрыть" из системного меню, чтобы убрать кнопку закрытия
            HMENU hMenu = GetSystemMenu(hwnd, FALSE);
            if (hMenu) {
                DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
                DrawMenuBar(hwnd);
            }

            // Перехват сообщения WM_CLOSE — игнорируем закрытие (например Alt+F4)
            // Подменяем оконную процедуру один раз
            static WNDPROC g_prevSettingsWndProc = nullptr;
            static bool g_subclassed = false;
            if (!g_subclassed) {
                // Подменяем оконную процедуру на SettingsWindowProcImpl и сохраняем прежнюю
                g_prevSettingsWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)SettingsWindowProcImpl);
                g_subclassed = true;
            }
        }

        created = true;
        return;
    }



    // Если окно уже создано, просто активировать его
    try {
        settingsWindow.Activate();
    }
    catch (...) {}
}

