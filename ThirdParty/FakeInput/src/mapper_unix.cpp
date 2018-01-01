/**
 * This file is part of the FakeInput library (https://github.com/uiii/FakeInput)
 *
 * Copyright (C) 2011 by Richard Jedlicka <uiii.dev@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "mapper.hpp"

#include <X11/Xlib.h>
#include <X11/keysym.h>

namespace FakeInput
{
    static long buttonTable[] = {
        Button1, // Mouse::LEFT
        Button2, // Mouse::MIDDLE
        Button3 // Mouse::RIGHT
    };

    static long keyTable[] = {
        XK_A, XK_B, XK_C, XK_D, XK_E, XK_F, XK_G, XK_H, XK_I,
        XK_J, XK_K, XK_L, XK_M, XK_N, XK_O, XK_P, XK_Q, XK_R,
        XK_S, XK_T, XK_U, XK_V, XK_W, XK_X, XK_Y, XK_Z, 

        XK_0, XK_1, XK_2, XK_3, XK_4, XK_5, XK_6, XK_7, XK_8, XK_9, 

        XK_F1, XK_F2, XK_F3, XK_F4, XK_F5, XK_F6, XK_F7, XK_F8, XK_F9, XK_F10, XK_F11, XK_F12,
        XK_F13, XK_F14, XK_F15, XK_F16, XK_F17, XK_F18, XK_F19, XK_F20, XK_F21, XK_F22, XK_F23, XK_F24,
        
		XK_Escape,
        XK_space,
		XK_Return,
        XK_BackSpace,
		XK_Tab,

		XK_Shift_L,
		XK_Shift_R,
		XK_Control_L,
		XK_Control_R,
		XK_Alt_L,
		XK_Alt_R,
		XK_Super_L,
		XK_Super_R,
		XK_Menu,

		XK_Caps_Lock,
		XK_Num_Lock,
		XK_Scroll_Lock,

		XK_Print,
		XK_Pause,

		XK_Insert,
		XK_Delete,
		XK_Prior,
		XK_Next,
		XK_Home,
		XK_End,

		XK_Left,
		XK_Up,
		XK_Right,
		XK_Down,

		XK_KP_0,
		XK_KP_1,
		XK_KP_2,
		XK_KP_3,
		XK_KP_4,
		XK_KP_5,
		XK_KP_6,
		XK_KP_7,
		XK_KP_8,
		XK_KP_9,

		XK_KP_Add,
		XK_KP_Subtract,
		XK_KP_Multiply,
		XK_KP_Divide,
		XK_KP_Decimal,
        XK_KP_Enter
    };

    long translateMouseButton(MouseButton button)
    {
        return buttonTable[button];
    }
    
    unsigned long translateKey(KeyType key)
    {
        return keyTable[key];
    }
}
