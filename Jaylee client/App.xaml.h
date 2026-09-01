#pragma once

#include "App.xaml.g.h"

namespace winrt::Jaylee_client::implementation
{
    struct App : AppT<App>
    {
        App();

        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

    private:
        winrt::Microsoft::UI::Xaml::Window window{ nullptr };
    };
}

namespace winrt::Jaylee_client::factory_implementation
{
    struct App : implementation::AppT<App>
    {
    };
}
