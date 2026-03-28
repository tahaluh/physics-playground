#pragma once

#include <X11/X.h>
#include <X11/Xlib.h>

#include "engine/input/KeyCode.h"

namespace X11InputTranslator
{
bool tryTranslateKey(XKeyEvent &event, EngineKeyCode &outKey);
}
