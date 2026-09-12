#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::Jaylee_client::implementation
{
    int32_t MainWindow::MyProperty()
    {
        return m_myProperty;
    }

    void MainWindow::MyProperty(int32_t value)
    {
        m_myProperty = value;
    }
}
