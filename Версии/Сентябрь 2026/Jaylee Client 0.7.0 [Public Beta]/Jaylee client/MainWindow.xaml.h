#pragma once

#include "MainWindow.g.h"

namespace winrt::Jaylee_client::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow()
        {

        }

        int32_t MyProperty();
        void MyProperty(int32_t value);

    private:
        int32_t m_myProperty{ 0 };
    };
}

namespace winrt::Jaylee_client::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
