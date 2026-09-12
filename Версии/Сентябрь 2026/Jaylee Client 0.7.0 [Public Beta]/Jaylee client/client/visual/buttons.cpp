

#include "pch.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include "buttons.h"
#include "../logic/buttonsLogic.h"
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Controls.Primitives.h>
#include <winrt/Microsoft.UI.Xaml.XamlTypeInfo.h>
#include <winrt/Microsoft.UI.Xaml.Input.h>
#include <string>

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml::Media;
using namespace winrt::Windows::UI;

winrt::Microsoft::UI::Xaml::UIElement CreateStrategyButtonsPanel()
{
	// Внешний контейнер: вертикальная панель, содержит основную сетку стратегий и блок TLS-кнопок ниже
	StackPanel container{};
	container.Orientation(winrt::Microsoft::UI::Xaml::Controls::Orientation::Vertical);

	Grid grid{};
	// Создать 4 колонки (авто-ширина)
	for (int c = 0; c < 4; ++c) {
		ColumnDefinition cd{};
		cd.Width(GridLengthHelper::FromValueAndType(0.0, GridUnitType::Auto));
		grid.ColumnDefinitions().Append(cd);
	}
	// Создать 3 строки
	for (int r = 0; r < 3; ++r) {
		RowDefinition rd{};
		rd.Height(GridLengthHelper::FromValueAndType(1.0, GridUnitType::Star));
		grid.RowDefinitions().Append(rd);
	}


	Grid tlsGrid{};
	for (int i = 0; i < 4; ++i) {
		ColumnDefinition cd{};
		cd.Width(GridLengthHelper::FromValueAndType(0.0, GridUnitType::Auto));
		tlsGrid.ColumnDefinitions().Append(cd);
	}
	tlsGrid.HorizontalAlignment(HorizontalAlignment::Left);
	tlsGrid.Margin({2,0,0,0});


	Color white{}; white.A = 0xFF; white.R = 0xFF; white.G = 0xFF; white.B = 0xFF;
	Color black{}; black.A = 0xFF; black.R = 0x00; black.G = 0x00; black.B = 0x00;
	// Более спокойный синий, похожий на выделение в Windows
	Color hoverBlue{}; hoverBlue.A = 0xFF; hoverBlue.R = 0x1E; hoverBlue.G = 0x88; hoverBlue.B = 0xE5;
	Color selectedBlue{}; selectedBlue.A = 0xFF; selectedBlue.R = 0x0B; selectedBlue.G = 0x66; selectedBlue.B = 0xC7;
	Color outlineCol{}; outlineCol.A = 0xFF; outlineCol.R = 0xCC; outlineCol.G = 0xCC; outlineCol.B = 0xCC;

	SolidColorBrush normalBrush{ white };
	SolidColorBrush hoverBrush{ hoverBlue };
	SolidColorBrush selectedBrush{ selectedBlue };
	SolidColorBrush textNormalBrush{ black };
	SolidColorBrush textSelectedBrush{ white };
	SolidColorBrush outlineBrush{ outlineCol };
	SolidColorBrush selectedOutlineBrush{ selectedBlue };

	int index = 0;
	int currentSelected = GetSelectedStrategy();

	for (int r = 0; r < 3; ++r) {
		for (int c = 0; c < 4; ++c) {
			++index;
			
			// Используем Border вместо Button, чтобы избежать стандартных серых фонов и шаблонов
			Border btn{};
			std::wstring label;
			if (index <= 12) {
				label = std::wstring(L"\u0421\u0442\u0440\u0430\u0442\u0435\u0433\u0438\u044F ") + std::to_wstring(index);
			} else {
				label = std::wstring(L"\u0054\u004C\u0053 \u0421\u0442\u0440\u0430\u0442\u0435\u0433\u0438\u044F ") + std::to_wstring(index - 12);
			}

			btn.Margin({ 4,4,4,4 });
			btn.Tag(winrt::box_value(false));
			btn.HorizontalAlignment(winrt::Microsoft::UI::Xaml::HorizontalAlignment::Stretch);
			btn.VerticalAlignment(winrt::Microsoft::UI::Xaml::VerticalAlignment::Stretch);
			btn.Width(110);
			btn.Height(36);
			btn.MinWidth(72);
			btn.MinHeight(32);
			btn.Padding({ 6,3,6,3 });
			btn.CornerRadius({ 4,4,4,4 });
			btn.Background(normalBrush);
			btn.BorderBrush(outlineBrush);
			btn.BorderThickness({ 1,1,1,1 });

			TextBlock tb{};
			tb.Text(winrt::hstring(label));
			tb.HorizontalAlignment(HorizontalAlignment::Center);
			tb.VerticalAlignment(VerticalAlignment::Center);
			tb.TextWrapping(winrt::Microsoft::UI::Xaml::TextWrapping::NoWrap);
			tb.FontSize(13);
			tb.Foreground(textNormalBrush);
			btn.Child(tb);

			btn.PointerEntered([btn, tb, hoverBrush, textSelectedBrush](auto&&, auto&&) {
				auto tag = btn.Tag();
				bool selected = tag ? winrt::unbox_value<bool>(tag) : false;
				if (!selected) {
					btn.Background(hoverBrush);
					tb.Foreground(textSelectedBrush);
				}
			});

			btn.PointerExited([btn, tb, normalBrush, selectedBrush, textNormalBrush, textSelectedBrush, outlineBrush, selectedOutlineBrush](auto&&, auto&&) {
				auto tag = btn.Tag();
				bool selected = tag ? winrt::unbox_value<bool>(tag) : false;
				if (!selected) {
					btn.Background(normalBrush);
					btn.BorderBrush(outlineBrush);
					tb.Foreground(textNormalBrush);
				} else {
					btn.Background(selectedBrush);
					btn.BorderBrush(selectedOutlineBrush);
					tb.Foreground(textSelectedBrush);
				}
			});

			btn.PointerPressed([grid, tlsGrid, btn, tb, normalBrush, selectedBrush, textNormalBrush, textSelectedBrush, outlineBrush, selectedOutlineBrush, index](auto&&, auto&&) {
				auto tag = btn.Tag();
				bool selected = tag ? winrt::unbox_value<bool>(tag) : false;
				bool newSel = !selected;

				// Сброс всех кнопок в grid
				for (auto const& child : grid.Children()) {
					if (auto otherBtn = child.try_as<Border>()) {
						otherBtn.Tag(winrt::box_value(false));
						otherBtn.Background(normalBrush);
						otherBtn.BorderBrush(outlineBrush);
						if (auto tbInner = otherBtn.Child().try_as<TextBlock>()) {
							tbInner.Foreground(textNormalBrush);
						}
					}
				}
				
				// Сброс всех кнопок в tlsGrid
				for (auto const& child : tlsGrid.Children()) {
					if (auto otherBtn = child.try_as<Border>()) {
						otherBtn.Tag(winrt::box_value(false));
						otherBtn.Background(normalBrush);
						otherBtn.BorderBrush(outlineBrush);
						if (auto tbInner = otherBtn.Child().try_as<TextBlock>()) {
							tbInner.Foreground(textNormalBrush);
						}
					}
				}
				
				if (newSel) {
					btn.Tag(winrt::box_value(true));
					btn.Background(selectedBrush);
					btn.BorderBrush(selectedOutlineBrush);
					tb.Foreground(textSelectedBrush);
					SelectStrategy(index);
				} else {
					SelectStrategy(0);
				}
			});

			if (currentSelected == index) {
				btn.Tag(winrt::box_value(true));
				btn.Background(selectedBrush);
				btn.BorderBrush(selectedOutlineBrush);
				tb.Foreground(textSelectedBrush);
			}

			Grid::SetRow(btn, r);
			Grid::SetColumn(btn, c);
			grid.Children().Append(btn);
		}
	}

	// Функция-лямбда для создания TLS-кнопки с тем же стилем
	auto makeTlsButton = [=](int tlsIndex, const std::wstring &labelText) {
		Border tbtn{};
		tbtn.Margin({4,4,4,4});
		tbtn.Tag(winrt::box_value(false));
		tbtn.HorizontalAlignment(winrt::Microsoft::UI::Xaml::HorizontalAlignment::Stretch);
		tbtn.VerticalAlignment(winrt::Microsoft::UI::Xaml::VerticalAlignment::Stretch);
		tbtn.Width(140);
		tbtn.Height(36);
		tbtn.MinWidth(72);
		tbtn.MinHeight(32);
		tbtn.Padding({6,3,6,3});
		tbtn.CornerRadius({4,4,4,4});
		tbtn.Background(normalBrush);
		tbtn.BorderBrush(outlineBrush);
		tbtn.BorderThickness({1,1,1,1});

		TextBlock ctb{};
		ctb.Text(winrt::hstring(labelText));
		ctb.HorizontalAlignment(HorizontalAlignment::Center);
		ctb.VerticalAlignment(VerticalAlignment::Center);
		ctb.TextWrapping(winrt::Microsoft::UI::Xaml::TextWrapping::NoWrap);
		ctb.FontSize(13);
		ctb.Foreground(textNormalBrush);
		tbtn.Child(ctb);

		tbtn.PointerEntered([tbtn, ctb, hoverBrush, textSelectedBrush](auto&&, auto&&) {
			auto tag = tbtn.Tag();
			bool selected = tag ? winrt::unbox_value<bool>(tag) : false;
			if (!selected) {
				tbtn.Background(hoverBrush);
				ctb.Foreground(textSelectedBrush);
			}
		});

		tbtn.PointerExited([tbtn, ctb, normalBrush, selectedBrush, textNormalBrush, textSelectedBrush, outlineBrush, selectedOutlineBrush](auto&&, auto&&) {
			auto tag = tbtn.Tag();
			bool selected = tag ? winrt::unbox_value<bool>(tag) : false;
			if (!selected) {
				tbtn.Background(normalBrush);
				tbtn.BorderBrush(outlineBrush);
				ctb.Foreground(textNormalBrush);
			} else {
				tbtn.Background(selectedBrush);
				tbtn.BorderBrush(selectedOutlineBrush);
				ctb.Foreground(textSelectedBrush);
			}
		});

		tbtn.PointerPressed([grid, tlsGrid, normalBrush, selectedBrush, textNormalBrush, textSelectedBrush, outlineBrush, selectedOutlineBrush, tlsIndex](winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&) {
			if (auto selfBtn = sender.try_as<Border>()) {
				auto tag = selfBtn.Tag();
				bool selected = tag ? winrt::unbox_value<bool>(tag) : false;
				bool newSel = !selected;
				
				for (auto const& child : grid.Children()) {
					if (auto otherBtn = child.try_as<Border>()) {
						otherBtn.Tag(winrt::box_value(false));
						otherBtn.Background(normalBrush);
						otherBtn.BorderBrush(outlineBrush);
						if (auto tbInner = otherBtn.Child().try_as<TextBlock>()) tbInner.Foreground(textNormalBrush);
					}
				}
				for (auto const& child : tlsGrid.Children()) {
					if (auto otherBtn = child.try_as<Border>()) {
						otherBtn.Tag(winrt::box_value(false));
						otherBtn.Background(normalBrush);
						otherBtn.BorderBrush(outlineBrush);
						if (auto tbInner = otherBtn.Child().try_as<TextBlock>()) tbInner.Foreground(textNormalBrush);
					}
				}

				if (newSel) {
					selfBtn.Tag(winrt::box_value(true));
					selfBtn.Background(selectedBrush);
					selfBtn.BorderBrush(selectedOutlineBrush);
					if (auto tbInner = selfBtn.Child().try_as<TextBlock>()) {
						tbInner.Foreground(textSelectedBrush);
					}
					SelectStrategy(tlsIndex);
				} else {
					SelectStrategy(0);
				}
			}
		});

		if (GetSelectedStrategy() == tlsIndex) {
			tbtn.Tag(winrt::box_value(true));
			tbtn.Background(selectedBrush);
			tbtn.BorderBrush(selectedOutlineBrush);
			ctb.Foreground(textSelectedBrush);
		}

		return tbtn;
	};

	// Добавить три TLS-кнопки (индексы 13..15) с явными Unicode escape-последовательностями
	{
		auto t1 = makeTlsButton(13, std::wstring(L"\u0054\u004C\u0053 \u0421\u0442\u0440\u0430\u0442\u0435\u0433\u0438\u044F 1"));
		Grid::SetColumn(t1, 0);
		tlsGrid.Children().Append(t1);
		auto t2 = makeTlsButton(14, std::wstring(L"\u0054\u004C\u0053 \u0421\u0442\u0440\u0430\u0442\u0435\u0433\u0438\u044F 2"));
		Grid::SetColumn(t2, 1);
		tlsGrid.Children().Append(t2);
		auto t3 = makeTlsButton(15, std::wstring(L"\u0054\u004C\u0053 \u0421\u0442\u0440\u0430\u0442\u0435\u0433\u0438\u044F 3"));
		Grid::SetColumn(t3, 2);
		tlsGrid.Children().Append(t3);
	}

	// Собрать контейнер: сначала grid, затем tlsGrid
	container.Children().Append(grid);
	container.Children().Append(tlsGrid);

	return container;
}

// Создаёт кнопку запуска/остановки обхода блокировок для вставки в окно
winrt::Microsoft::UI::Xaml::UIElement CreateToggleBypassButton()
{
	// Возвращаем Border, выступающий как кнопка, чтобы избежать визуальных overlay из шаблона Button
	Border btnBorder{};
	btnBorder.Width(320);
	btnBorder.Height(52);
	btnBorder.CornerRadius({ 6,6,6,6 });
	btnBorder.HorizontalAlignment(winrt::Microsoft::UI::Xaml::HorizontalAlignment::Center);
	btnBorder.VerticalAlignment(winrt::Microsoft::UI::Xaml::VerticalAlignment::Bottom);
	btnBorder.Margin({ 0,0,0,12 });

	winrt::hstring startLabel = winrt::hstring(L"\u0417\u0430\u043F\u0443\u0441\u0442\u0438\u0442\u044C \u043E\u0431\u0445\u043E\u0434 \u0431\u043B\u043E\u043A\u0438\u0440\u043E\u0432\u043E\u043A");
	winrt::hstring stopLabel = winrt::hstring(L"\u041E\u0441\u0442\u0430\u043D\u043E\u0432\u0438\u0442\u044C \u043E\u0431\u0445\u043E\u0434 \u0431\u043B\u043E\u043A\u0438\u0440\u043E\u0432\u043E\u043A");

	TextBlock tb{};
	tb.Text(startLabel);
	tb.HorizontalAlignment(HorizontalAlignment::Center);
	tb.VerticalAlignment(VerticalAlignment::Center);
	tb.FontSize(16);
	// Сделать текст полностью непрозрачным и чуть жирнее для контраста
	tb.Opacity(1.0);
	winrt::Windows::UI::Text::FontWeight fw{};
	fw.Weight = 600; // semi-bold
	tb.FontWeight(fw);

	btnBorder.Child(tb);

	// Более сильная тусклость (~50% непрозрачности)
	winrt::Windows::UI::Color greenColor{}; greenColor.A = 0x80; greenColor.R = 0x21; greenColor.G = 0x96; greenColor.B = 0x35;
	winrt::Windows::UI::Color redColor{}; redColor.A = 0x80; redColor.R = 0xE5; redColor.G = 0x39; redColor.B = 0x35;
	winrt::Microsoft::UI::Xaml::Media::SolidColorBrush greenBrush{ greenColor };
	winrt::Microsoft::UI::Xaml::Media::SolidColorBrush redBrush{ redColor };
	// Контур (обводка) — полностью непрозрачный цвет того же тона
	winrt::Windows::UI::Color greenOutlineColor{}; greenOutlineColor.A = 0xFF; greenOutlineColor.R = greenColor.R; greenOutlineColor.G = greenColor.G; greenOutlineColor.B = greenColor.B;
	winrt::Windows::UI::Color redOutlineColor{}; redOutlineColor.A = 0xFF; redOutlineColor.R = redColor.R; redOutlineColor.G = redColor.G; redOutlineColor.B = redColor.B;
	winrt::Microsoft::UI::Xaml::Media::SolidColorBrush greenOutlineBrush{ greenOutlineColor };
	winrt::Microsoft::UI::Xaml::Media::SolidColorBrush redOutlineBrush{ redOutlineColor };
	// Текст — полностью непрозрачный белый
	winrt::Windows::UI::Color whiteOpaque{}; whiteOpaque.A = 0xFF; whiteOpaque.R = 0xFF; whiteOpaque.G = 0xFF; whiteOpaque.B = 0xFF;
	winrt::Microsoft::UI::Xaml::Media::SolidColorBrush whiteTextBrush{ whiteOpaque };

	// состояние в Tag: false - остановлено (предлагает Запустить), true - запущено
	bool initialRunning = GetUIBypassRunningState() || IsBypassRunning();
	btnBorder.Tag(winrt::box_value(initialRunning));
	if (initialRunning) {
		tb.Text(stopLabel);
		btnBorder.Background(redBrush);
		btnBorder.BorderBrush(redOutlineBrush);
		tb.Foreground(whiteTextBrush);
	} else {
		tb.Text(startLabel);
		btnBorder.Background(greenBrush);
		btnBorder.BorderBrush(greenOutlineBrush);
		tb.Foreground(whiteTextBrush);
	}

	// Кнопка теперь запускает/останавливает обход через логику
	btnBorder.PointerPressed([btnBorder, tb, startLabel, stopLabel, greenBrush, redBrush, whiteTextBrush, greenOutlineBrush, redOutlineBrush](winrt::Windows::Foundation::IInspectable const& /*sender*/, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& /*args*/) {
		OutputDebugStringW(L"UI: toggle button pressed -> invoking logic\n");
		auto tag = btnBorder.Tag();
		bool running = tag ? winrt::unbox_value<bool>(tag) : false;
		if (!running) {
			// попытаться запустить
			bool ok = StartSelectedBypass();
			if (ok) {
				// обновить UI
				btnBorder.Tag(winrt::box_value(true));
				tb.Text(stopLabel);
				btnBorder.Background(redBrush);
				btnBorder.BorderBrush(redOutlineBrush);
				tb.Foreground(whiteTextBrush);
			} else {
				// показать диагностический файл/диалог
				ReportBypassStatusIfNotRunning();
				btnBorder.Tag(winrt::box_value(false));
			}
		} else {
			// остановить
			StopBypass();
			btnBorder.Tag(winrt::box_value(false));
			tb.Text(startLabel);
			btnBorder.Background(greenBrush);
			btnBorder.BorderBrush(greenOutlineBrush);
			tb.Foreground(whiteTextBrush);
		}
	});

	// Убедиться, что PointerEntered/Exited не меняют фон — просто восстановим текущий фон
	btnBorder.PointerEntered([btnBorder, tb, greenBrush, redBrush, whiteTextBrush, greenOutlineBrush, redOutlineBrush](winrt::Windows::Foundation::IInspectable const& /*sender*/, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& /*args*/) {
		auto tag = btnBorder.Tag();
		bool running = tag ? winrt::unbox_value<bool>(tag) : false;
		if (running) {
			btnBorder.Background(redBrush);
			btnBorder.BorderBrush(redOutlineBrush);
		} else {
			btnBorder.Background(greenBrush);
			btnBorder.BorderBrush(greenOutlineBrush);
		}
		tb.Foreground(whiteTextBrush);
	});

	btnBorder.PointerExited([btnBorder, tb, greenBrush, redBrush, whiteTextBrush, greenOutlineBrush, redOutlineBrush](winrt::Windows::Foundation::IInspectable const& /*sender*/, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& /*args*/) {
		auto tag = btnBorder.Tag();
		bool running = tag ? winrt::unbox_value<bool>(tag) : false;
		if (running) {
			btnBorder.Background(redBrush);
			btnBorder.BorderBrush(redOutlineBrush);
		} else {
			btnBorder.Background(greenBrush);
			btnBorder.BorderBrush(greenOutlineBrush);
		}
		tb.Foreground(whiteTextBrush);
	});

	return btnBorder;
}

// Создаёт панель с кнопками в нижних краях: кнопку списка слева и панель (консоль+шестерёнка) справа
winrt::Microsoft::UI::Xaml::UIElement CreateBottomCornerButtonsPanel()
{
	Grid container{};

	// Левая панель внизу: кнопка списка и кнопка инструмента рядом
	StackPanel spLeft{};
	spLeft.Orientation(winrt::Microsoft::UI::Xaml::Controls::Orientation::Horizontal);
	spLeft.HorizontalAlignment(winrt::Microsoft::UI::Xaml::HorizontalAlignment::Left);
	spLeft.VerticalAlignment(winrt::Microsoft::UI::Xaml::VerticalAlignment::Bottom);
	spLeft.Margin({12,0,0,12});

	Border listBorder{};
	listBorder.Width(48);
	listBorder.Height(48);
	listBorder.CornerRadius({8,8,8,8});
	winrt::Windows::UI::Color listBg{}; listBg.A = 0xFF; listBg.R = 0xF5; listBg.G = 0xF5; listBg.B = 0xF5;
	listBorder.Background(SolidColorBrush{ listBg });
	winrt::Windows::UI::Color listBorderColor{}; listBorderColor.A = 0xFF; listBorderColor.R = 0xDD; listBorderColor.G = 0xDD; listBorderColor.B = 0xDD;
	listBorder.BorderBrush(SolidColorBrush{ listBorderColor });
	listBorder.BorderThickness({1,1,1,1});
	TextBlock listTb{};
	// Используем эмодзи "📋" (U+1F4CB) для иконки списка
	listTb.Text(winrt::hstring(L"\U0001F4CB")); // 📋
	listTb.HorizontalAlignment(winrt::Microsoft::UI::Xaml::HorizontalAlignment::Center);
	listTb.VerticalAlignment(winrt::Microsoft::UI::Xaml::VerticalAlignment::Center);
	listTb.FontSize(18);
	winrt::Windows::UI::Color listFg{}; listFg.A = 0xFF; listFg.R = 0x9B; listFg.G = 0x85; listFg.B = 0xC8;
	listTb.Foreground(SolidColorBrush{ listFg });
	listBorder.Child(listTb);
	// При клике открыть файл списка через логику
	listBorder.PointerPressed([](auto&&, auto&&) {
		::OpenListFileFromLogic();
	});
	spLeft.Children().Append(listBorder);

	// Кнопка инструментов рядом со списком (эмодзи 🛠️)
	Border toolBorder{};
	toolBorder.Width(48);
	toolBorder.Height(48);
	toolBorder.CornerRadius({8,8,8,8});
	toolBorder.Margin({8,0,0,0});
	toolBorder.Background(SolidColorBrush{ listBg });
	toolBorder.BorderBrush(SolidColorBrush{ listBorderColor });
	toolBorder.BorderThickness({1,1,1,1});
	TextBlock toolTb{};
	// Используем символ информации "ℹ️" (U+2139 U+FE0F)
	toolTb.Text(winrt::hstring(L"\u2139\uFE0F")); // ℹ️
	toolTb.HorizontalAlignment(winrt::Microsoft::UI::Xaml::HorizontalAlignment::Center);
	toolTb.VerticalAlignment(winrt::Microsoft::UI::Xaml::VerticalAlignment::Center);
	toolTb.FontSize(18);
	toolTb.Foreground(SolidColorBrush{ listFg });
	toolBorder.Child(toolTb);
	// Обработчик клика по кнопке информации — вызвать логику, которая откроет окно "О программе"
	toolBorder.PointerPressed([](auto&&, auto&&) {
		::OpenAboutFromLogic();
	});
	spLeft.Children().Append(toolBorder);

	container.Children().Append(spLeft);

	// Правая панель: консоль + шестерёнка
	StackPanel spRight{};
	spRight.Orientation(winrt::Microsoft::UI::Xaml::Controls::Orientation::Horizontal);
	spRight.HorizontalAlignment(winrt::Microsoft::UI::Xaml::HorizontalAlignment::Right);
	spRight.VerticalAlignment(winrt::Microsoft::UI::Xaml::VerticalAlignment::Bottom);
	spRight.Margin({0,0,12,12});

	Border consoleBorder{};
	consoleBorder.Width(48);
	consoleBorder.Height(48);
	consoleBorder.CornerRadius({8,8,8,8});
	consoleBorder.Margin({0,0,8,0});
	winrt::Windows::UI::Color consoleBg{}; consoleBg.A = 0xFF; consoleBg.R = 0xF5; consoleBg.G = 0xF5; consoleBg.B = 0xF5;
	consoleBorder.Background(SolidColorBrush{ consoleBg });
	winrt::Windows::UI::Color consoleBorderColor{}; consoleBorderColor.A = 0xFF; consoleBorderColor.R = 0xDD; consoleBorderColor.G = 0xDD; consoleBorderColor.B = 0xDD;
	consoleBorder.BorderBrush(SolidColorBrush{ consoleBorderColor });
	consoleBorder.BorderThickness({1,1,1,1});
	// Сделать символ консоли крупным, занимающим почти всю кнопку
	TextBlock consoleLargeTb{};
	consoleLargeTb.Text(winrt::hstring(L">_"));
	consoleLargeTb.HorizontalAlignment(winrt::Microsoft::UI::Xaml::HorizontalAlignment::Center);
	consoleLargeTb.VerticalAlignment(winrt::Microsoft::UI::Xaml::VerticalAlignment::Center);
	// Подогнать размер под кнопку: чуть меньше, чтобы оставить небольшой отступ сверху
	consoleLargeTb.FontSize(32);
	winrt::Windows::UI::Text::FontWeight consoleFw{}; consoleFw.Weight = 800;
	consoleLargeTb.FontWeight(consoleFw);
	winrt::Windows::UI::Color consoleFg{}; consoleFg.A = 0xFF; consoleFg.R = 0x8F; consoleFg.G = 0x78; consoleFg.B = 0xC4;
	consoleLargeTb.Foreground(SolidColorBrush{ consoleFg });
	// Смещаем немного вверх, чтобы символ находился выше центра
	consoleLargeTb.Margin({ -2, -6, 0, 0 });
	consoleBorder.Child(consoleLargeTb);

	// При клике по консольной кнопке — запустить service.bat через логику
	consoleBorder.PointerPressed([](auto&&, auto&&) {
		::OpenServiceBatchFromLogic();
	});

	Border gearBorder{};
	gearBorder.Width(48);
	gearBorder.Height(48);
	gearBorder.CornerRadius({8,8,8,8});
	winrt::Windows::UI::Color gearBg{}; gearBg.A = 0xFF; gearBg.R = 0xF5; gearBg.G = 0xF5; gearBg.B = 0xF5;
	gearBorder.Background(SolidColorBrush{ gearBg });
	winrt::Windows::UI::Color gearBorderColor{}; gearBorderColor.A = 0xFF; gearBorderColor.R = 0xDD; gearBorderColor.G = 0xDD; gearBorderColor.B = 0xDD;
	gearBorder.BorderBrush(SolidColorBrush{ gearBorderColor });
	gearBorder.BorderThickness({1,1,1,1});
	TextBlock gearTb{};
	gearTb.Text(winrt::hstring(L"\u2699")); // ⚙
	gearTb.HorizontalAlignment(winrt::Microsoft::UI::Xaml::HorizontalAlignment::Center);
	gearTb.VerticalAlignment(winrt::Microsoft::UI::Xaml::VerticalAlignment::Center);
	gearTb.FontSize(18);
	winrt::Windows::UI::Color gearFg{}; gearFg.A = 0xFF; gearFg.R = 0x9B; gearFg.G = 0x85; gearFg.B = 0xC8;
	gearTb.Foreground(SolidColorBrush{ gearFg });
	gearBorder.Child(gearTb);

	// Обработчик клика по шестерёнке: вызвать логику, которая откроет окно настроек
	// Подключаемся к функции в client/logic/buttonsLogic.cpp
	gearBorder.PointerPressed([](auto&&, auto&&) {
		// Вызов логики, которая показывает окно настроек
		OpenSettingsFromLogic();
	});

	spRight.Children().Append(consoleBorder);
	spRight.Children().Append(gearBorder);
	container.Children().Append(spRight);

	return container;
}