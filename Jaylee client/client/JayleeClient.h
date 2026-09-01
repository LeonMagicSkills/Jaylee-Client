#pragma once

#include <winrt/Microsoft.UI.Xaml.h>

namespace winrt::Jaylee_client::implementation {

struct JayleeClient
{
	JayleeClient();
	void Start();
	void StartWinws(const std::wstring& exePath, const std::wstring& args);
	winrt::Microsoft::UI::Xaml::Window Window() const { return m_window; }

private:
	winrt::Microsoft::UI::Xaml::Window m_window{ nullptr };
};

}