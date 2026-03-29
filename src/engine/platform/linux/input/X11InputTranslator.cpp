#include "engine/platform/linux/input/X11InputTranslator.h"

#include <X11/keysym.h>

namespace X11InputTranslator
{
bool tryTranslateKey(XKeyEvent &event, EngineKeyCode &outKey)
{
    const KeySym keysym = XLookupKeysym(&event, 0);

    switch (keysym)
    {
    case XK_c:
    case XK_C:
        outKey = EngineKeyCode::C;
        return true;
    case XK_Control_L:
        outKey = EngineKeyCode::ControlLeft;
        return true;
    case XK_Control_R:
        outKey = EngineKeyCode::ControlRight;
        return true;
    case XK_w:
    case XK_W:
        outKey = EngineKeyCode::W;
        return true;
    case XK_a:
    case XK_A:
        outKey = EngineKeyCode::A;
        return true;
    case XK_s:
    case XK_S:
        outKey = EngineKeyCode::S;
        return true;
    case XK_d:
    case XK_D:
        outKey = EngineKeyCode::D;
        return true;
    case XK_q:
    case XK_Q:
        outKey = EngineKeyCode::Q;
        return true;
    case XK_e:
    case XK_E:
        outKey = EngineKeyCode::E;
        return true;
    case XK_Up:
        outKey = EngineKeyCode::Up;
        return true;
    case XK_Down:
        outKey = EngineKeyCode::Down;
        return true;
    case XK_Left:
        outKey = EngineKeyCode::Left;
        return true;
    case XK_Right:
        outKey = EngineKeyCode::Right;
        return true;
    case XK_Return:
    case XK_KP_Enter:
        outKey = EngineKeyCode::Enter;
        return true;
    case XK_Escape:
        outKey = EngineKeyCode::Escape;
        return true;
    case XK_F1:
        outKey = EngineKeyCode::F1;
        return true;
    case XK_F2:
        outKey = EngineKeyCode::F2;
        return true;
    case XK_F3:
        outKey = EngineKeyCode::F3;
        return true;
    default:
        return false;
    }
}
}
