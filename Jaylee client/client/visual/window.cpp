#include "pch.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include "window.h"
#include "buttons.h"
#include "../logic/buttonsLogic.h"
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.h>
#include <Windows.h>

using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml::Media;

Window CreateJayleeWindow(winrt::hstring const& title)
{
	OutputDebugStringW(L"CreateJayleeWindow invoked\n");
	Window window{};

	// Простое содержимое: Grid с белым фоном и светлой темой
	Grid grid{};
	// Явно указать светлую тему
	grid.RequestedTheme(winrt::Microsoft::UI::Xaml::ElementTheme::Light);
	// Создать белую кисть через Windows::UI::Color
	winrt::Windows::UI::Color color{};
	color.A = 0xFF;
	color.R = 0xFF;
	color.G = 0xFF;
	color.B = 0xFF;
	SolidColorBrush brush{ color };
	grid.Background(brush);

	// Добавить панель кнопок стратегий в левый верхний угол
	auto buttonsPanel = CreateStrategyButtonsPanel();
	// Разместим панель в небольшой вложенной Grid, чтобы она занимала верхнюю-левую область
	Grid inner{};
	inner.HorizontalAlignment(winrt::Microsoft::UI::Xaml::HorizontalAlignment::Left);
	inner.VerticalAlignment(winrt::Microsoft::UI::Xaml::VerticalAlignment::Top);
	inner.Children().Append(buttonsPanel);

	grid.Children().Append(inner);

	// Кнопка запуска/остановки обхода блокировок внизу по центру
	auto toggleBtn = CreateToggleBypassButton();

	grid.Children().Append(toggleBtn);

	// Нижние кнопки вынесены в отдельную панель в buttons.cpp
	{
		auto bottomPanel = CreateBottomCornerButtonsPanel();
		grid.Children().Append(bottomPanel);
	}

	window.Content(grid);

	try { window.Title(title); } catch (...) {}

	// Активируем окно, затем изменим размер через Win32 API
	window.Activate();

	const int width = 800;
	// Уменьшенная высота окна — сделаем окно ниже
	const int height = 440;

	// Попробуем получить HWND по заголовку окна
	auto hTitle = title.c_str();
	HWND hwnd = FindWindowW(nullptr, hTitle);

	if (hwnd)
	{
		// Установим размер окна и немного поднимем его вверх на 40 пикселей
		RECT rc;
		if (GetWindowRect(hwnd, &rc)) {
			int newX = rc.left;
			int newY = rc.top - 40;
			if (newY < 0) newY = 0;
			SetWindowPos(hwnd, nullptr, newX, newY, width, height, SWP_NOZORDER);
		}
		// Отключить изменение размера и кнопку "Развернуть" (запретить полноэкранный режим)
		LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
		style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
		SetWindowLongPtrW(hwnd, GWL_STYLE, style);

		// Удалить пункт максимизации из системного меню
		HMENU hMenu = GetSystemMenu(hwnd, FALSE);
		if (hMenu) {
			EnableMenuItem(hMenu, SC_MAXIMIZE, MF_BYCOMMAND | MF_GRAYED);
			RemoveMenu(hMenu, SC_MAXIMIZE, MF_BYCOMMAND);
		}

		// Применить изменения к фрейму окна (с уже установленной позицией)
		SetWindowPos(hwnd, nullptr, 0, 0, width, height, SWP_NOZORDER | SWP_NOMOVE | SWP_FRAMECHANGED);
	}

	window.Closed([hwnd](winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::WindowEventArgs const& args) {
		if (IsTrayRunning()) {
			args.Handled(true);
			if (hwnd) {
				ShowWindow(hwnd, SW_HIDE);
			}
		}
	});

	return window;
}
