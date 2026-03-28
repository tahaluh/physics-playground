#pragma once

#include <memory>

class IWindow;

namespace PlatformFactory
{
std::unique_ptr<IWindow> createWindow();
}
