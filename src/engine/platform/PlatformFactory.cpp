#include "engine/platform/PlatformFactory.h"

#include <memory>

#include "engine/platform/IWindow.h"

#if defined(__linux__)
#include "engine/platform/linux/X11Window.h"
#endif

namespace PlatformFactory
{
std::unique_ptr<IWindow> createWindow()
{
#if defined(__linux__)
    return std::make_unique<X11Window>();
#else
    return nullptr;
#endif
}
}
