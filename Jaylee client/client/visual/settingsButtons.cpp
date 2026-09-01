#include "pch.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include "settingsButtons.h"
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Xaml.Input.h>
#include <Windows.h>

#include "../logic/buttonsLogic.h"

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml::Media;

// Создаёт простую панель с кнопкой в левом верхнем углу
winrt::Microsoft::UI::Xaml::UIElement CreateSettingsButtonsPanel()
{
	Grid container{};
	// Растянуть контейнер на всё окно, чтобы можно было позиционировать кнопку внизу по центру
	container.HorizontalAlignment(winrt::Microsoft::UI::Xaml::HorizontalAlignment::Stretch);
	container.VerticalAlignment(winrt::Microsoft::UI::Xaml::VerticalAlignment::Stretch);
	// Уменьшим внутренние отступы, чтобы кнопки помещались в узкое окно
	container.Margin({8,8,8,8});

	// Кнопка в виде Border + TextBlock
	Border btnBorder{};
	btnBorder.CornerRadius({6,6,6,6});
	btnBorder.Padding({10,6,10,6});
	btnBorder.Background(SolidColorBrush{ winrt::Windows::UI::Color{ 0xFF, 0xF5, 0xF5, 0xF5 } });
	// Индикатор состояния автозапуска: зелёная — включено, красная — отключено
	bool startupPresent = IsAutorunEnabled();
	if (startupPresent) {
		btnBorder.BorderBrush(SolidColorBrush{ winrt::Windows::UI::Color{ 0xFF, 0x00, 0xC8, 0x00 } }); // зелёная
	} else {
		btnBorder.BorderBrush(SolidColorBrush{ winrt::Windows::UI::Color{ 0xFF, 0xFF, 0x00, 0x00 } }); // красная
	}
	btnBorder.BorderThickness({1,1,1,1});

	TextBlock tb{};
	// "Добавить в автозагрузку" как Unicode escape
	// Вернуть оригинальную надпись кнопки автозагрузки
	tb.Text(winrt::hstring(L"\u0414\u043E\u0431\u0430\u0432\u0438\u0442\u044C \u0432 \u0430\u0432\u0442\u043E\u0437\u0430\u0433\u0440\u0443\u0437\u043A\u0443"));
	// Уменьшим размер шрифта для кнопки автозагрузки
	tb.FontSize(12);
	tb.HorizontalAlignment(winrt::Microsoft::UI::Xaml::HorizontalAlignment::Center);
	tb.VerticalAlignment(winrt::Microsoft::UI::Xaml::VerticalAlignment::Center);
	// Ограничим максимальную ширину первой кнопки, чтобы влезла в окно вместе с правой кнопкой
	tb.TextWrapping(winrt::Microsoft::UI::Xaml::TextWrapping::NoWrap);
	btnBorder.MaxWidth(260.0);
	// Кнопка автозапуска: стандартная подпись, без переноса

	btnBorder.Child(tb);

	// При нажатии переключаем автозапуск через buttonsLogic
	btnBorder.PointerPressed([](winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& /*args*/) {
		auto border = sender.as<Border>();
		bool newState = ToggleAutorun();
		if (newState) {
			border.BorderBrush(SolidColorBrush{ winrt::Windows::UI::Color{ 0xFF, 0x00, 0xC8, 0x00 } });
		} else {
			border.BorderBrush(SolidColorBrush{ winrt::Windows::UI::Color{ 0xFF, 0xFF, 0x00, 0x00 } });
		}
	});

	// Разместим кнопки автозапуска и фонового режима по левому и правому краю сверху
	Grid topGrid{};
	topGrid.HorizontalAlignment(winrt::Microsoft::UI::Xaml::HorizontalAlignment::Stretch);
	topGrid.VerticalAlignment(winrt::Microsoft::UI::Xaml::VerticalAlignment::Top);
	// Уменьшим отступ у сетки, чтобы элементы были ближе к верхней границе
	topGrid.Margin({0,6,0,0});

	ColumnDefinition col1{};
	ColumnDefinition col2{};
	col1.Width(GridLength(1.0, GridUnitType::Star));
	col2.Width(GridLength(1.0, GridUnitType::Star));
	topGrid.ColumnDefinitions().Append(col1);
	topGrid.ColumnDefinitions().Append(col2);

	StackPanel leftPanel{};
	leftPanel.Orientation(winrt::Microsoft::UI::Xaml::Controls::Orientation::Horizontal);
	// Выравниваем по центру окна, чтобы две кнопки были ближе к центру
	leftPanel.HorizontalAlignment(winrt::Microsoft::UI::Xaml::HorizontalAlignment::Center);
	leftPanel.VerticalAlignment(winrt::Microsoft::UI::Xaml::VerticalAlignment::Top);
	// Отступ сверху немного увеличен
	leftPanel.Margin({0,20,0,0});

	btnBorder.Margin({0,0,8,0});
	btnBorder.MaxWidth(160.0);

	leftPanel.Children().Append(btnBorder);

	{
		Border bgModeBorder{};
		bgModeBorder.CornerRadius({6,6,6,6});
		bgModeBorder.Padding({8,4,8,4});
		bgModeBorder.Background(SolidColorBrush{ winrt::Windows::UI::Color{ 0xFF, 0xF5, 0xF5, 0xF5 } });
		bgModeBorder.BorderBrush(SolidColorBrush{ winrt::Windows::UI::Color{ 0xFF, 0xDD, 0xDD, 0xDD } });
		bgModeBorder.BorderThickness({1,1,1,1});
		bgModeBorder.Margin({6,0,0,0});
		bgModeBorder.MinWidth(80.0);

		TextBlock tbBg{};
		tbBg.Text(winrt::hstring(L"\u0424\u043E\u043D\u043E\u0432\u044B\u0439 \u0440\u0435\u0436\u0438\u043C"));
		tbBg.FontSize(13);
		tbBg.HorizontalAlignment(winrt::Microsoft::UI::Xaml::HorizontalAlignment::Center);
		tbBg.VerticalAlignment(winrt::Microsoft::UI::Xaml::VerticalAlignment::Center);

		bgModeBorder.Child(tbBg);


		// Индикатор состояния для фонового режима и запуск/остановка процесса в трее
		bool trayRunning = IsTrayRunning();
		if (trayRunning) {
			bgModeBorder.BorderBrush(SolidColorBrush{ winrt::Windows::UI::Color{ 0xFF, 0x00, 0xC8, 0x00 } });
		} else {
			bgModeBorder.BorderBrush(SolidColorBrush{ winrt::Windows::UI::Color{ 0xFF, 0xFF, 0x00, 0x00 } });
		}

		bgModeBorder.PointerPressed([](winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& /*args*/) {
			auto border = sender.as<Border>();
			bool newState = ToggleTray();
			if (newState) {
				border.BorderBrush(SolidColorBrush{ winrt::Windows::UI::Color{ 0xFF, 0x00, 0xC8, 0x00 } });
			} else {
				border.BorderBrush(SolidColorBrush{ winrt::Windows::UI::Color{ 0xFF, 0xFF, 0x00, 0x00 } });
			}
		});

		leftPanel.Children().Append(bgModeBorder);
	}



	// Горизонтальная панель и кнопка автозапуска помещаем в общую вертикальную
	// панель, чтобы они располагались друг под другом и были центрированы
	StackPanel centerColumn{};
	centerColumn.Orientation(winrt::Microsoft::UI::Xaml::Controls::Orientation::Vertical);
	centerColumn.HorizontalAlignment(winrt::Microsoft::UI::Xaml::HorizontalAlignment::Center);
	centerColumn.VerticalAlignment(winrt::Microsoft::UI::Xaml::VerticalAlignment::Top);
	// Уменьшим верхний отступ, чтобы вся группа была выше
	centerColumn.Margin({0,0,0,0});

	centerColumn.Children().Append(leftPanel);

	// Кнопка "Автозапуск стратегии" под двумя кнопками, по центру и растянутая
	Border strategyBorder{};
	strategyBorder.CornerRadius({6,6,6,6});
	strategyBorder.Padding({10,6,10,6});
	strategyBorder.Background(SolidColorBrush{ winrt::Windows::UI::Color{ 0xFF, 0xF5, 0xF5, 0xF5 } });
	// Индикатор состояния для кнопки автозапуска стратегии (красная по умолчанию)
	bool stratEnabled = IsStrategyAutostartEnabled();
	if (stratEnabled) {
		strategyBorder.BorderBrush(SolidColorBrush{ winrt::Windows::UI::Color{ 0xFF, 0x00, 0xC8, 0x00 } });
	} else {
		strategyBorder.BorderBrush(SolidColorBrush{ winrt::Windows::UI::Color{ 0xFF, 0xFF, 0x00, 0x00 } });
	}
	strategyBorder.BorderThickness({1,1,1,1});
	strategyBorder.HorizontalAlignment(winrt::Microsoft::UI::Xaml::HorizontalAlignment::Stretch);
	strategyBorder.VerticalAlignment(winrt::Microsoft::UI::Xaml::VerticalAlignment::Top);
	strategyBorder.Margin({0,12,0,0});
	strategyBorder.MinHeight(36.0);
	strategyBorder.MaxWidth(360.0);

	TextBlock tbStrategy{};
	tbStrategy.Text(winrt::hstring(L"\u0410\u0432\u0442\u043E\u0437\u0430\u043F\u0443\u0441\u043A \u0441\u0442\u0440\u0430\u0442\u0435\u0433\u0438\u0438"));
	tbStrategy.FontSize(13);
	tbStrategy.HorizontalAlignment(winrt::Microsoft::UI::Xaml::HorizontalAlignment::Center);
	tbStrategy.VerticalAlignment(winrt::Microsoft::UI::Xaml::VerticalAlignment::Center);

	strategyBorder.Child(tbStrategy);

	strategyBorder.PointerPressed([](winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& /*args*/) {
		auto border = sender.as<Border>();
		bool newState = ToggleStrategyAutostart();
		if (newState) {
			border.BorderBrush(SolidColorBrush{ winrt::Windows::UI::Color{ 0xFF, 0x00, 0xC8, 0x00 } });
		} else {
			border.BorderBrush(SolidColorBrush{ winrt::Windows::UI::Color{ 0xFF, 0xFF, 0x00, 0x00 } });
		}
	});

	centerColumn.Children().Append(strategyBorder);

	// Добавляем общую колонку в корневой контейнер
	container.Children().Append(centerColumn);

	// Кнопка "Закрыть" по центру снизу
	Border closeBorder{};
	closeBorder.CornerRadius({6,6,6,6});
	closeBorder.Padding({10,6,10,6});
	closeBorder.Background(SolidColorBrush{ winrt::Windows::UI::Color{ 0xFF, 0xF5, 0xF5, 0xF5 } });
	closeBorder.BorderBrush(SolidColorBrush{ winrt::Windows::UI::Color{ 0xFF, 0xDD, 0xDD, 0xDD } });
	closeBorder.BorderThickness({1,1,1,1});
	closeBorder.HorizontalAlignment(winrt::Microsoft::UI::Xaml::HorizontalAlignment::Center);
	closeBorder.VerticalAlignment(winrt::Microsoft::UI::Xaml::VerticalAlignment::Bottom);
	closeBorder.Margin({0,0,0,12});

	TextBlock tbClose{};
	tbClose.Text(winrt::hstring(L"\u0417\u0430\u043A\u0440\u044B\u0442\u044C"));
	tbClose.FontSize(13);
	tbClose.HorizontalAlignment(winrt::Microsoft::UI::Xaml::HorizontalAlignment::Center);
	tbClose.VerticalAlignment(winrt::Microsoft::UI::Xaml::VerticalAlignment::Center);

	closeBorder.Child(tbClose);

	// Обработчик: прячем окно настроек (ищем по заголовку)
	closeBorder.PointerPressed([](winrt::Windows::Foundation::IInspectable const& /*sender*/, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& /*args*/) {
		HWND hwnd = FindWindowW(nullptr, L"\u041D\u0430\u0441\u0442\u0440\u043E\u0439\u043A\u0438");
		if (hwnd) {
			ShowWindow(hwnd, SW_HIDE);
		}
	});

	container.Children().Append(closeBorder);

	return container;
}
