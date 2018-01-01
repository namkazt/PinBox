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

#include "key_win.hpp"

#include <stdexcept>
#include <iostream>

#include "mapper.hpp"

namespace FakeInput
{
    Key_win::Key_win()
    {
        virtualKey_ = 0;
    }

    Key_win::Key_win(KeyType type)
    {
        if(type == Key_NoKey)
        {
            virtualKey_ = 0;
            code_ = 0;
            name_ = "<no key>";
        }
        else
        {
            virtualKey_ = (WORD) translateKey(type);

            Key_win tmpKey(virtualKey_);
            code_ = tmpKey.code_;
            name_ = tmpKey.name_;
        }
    }

    Key_win::Key_win(MSG* message)
    {
        switch(message->message)
        {
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
                virtualKey_ = message->wParam;
            break;
            default:
                std::logic_error("Cannot get key from non-key message");
            break;
        }

        Key_win tmpKey(virtualKey_);
        code_ = tmpKey.code_;
        name_ = tmpKey.name_;
    }

    Key_win::Key_win(WORD virtualKey):
        virtualKey_(virtualKey)
    {
        code_ = MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC);
        LONG lParam = code_;
        switch (virtualKey)
        {
            case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN: // arrow keys
            case VK_PRIOR: case VK_NEXT: // page up and page down
            case VK_END: case VK_HOME:
            case VK_INSERT: case VK_DELETE:
            case VK_DIVIDE: // numpad slash
            case VK_NUMLOCK:
            {
                lParam |= 0x100; // set extended bit
                break;
            }
        }

        char name[128];
        if(GetKeyNameText(lParam << 16, name, 128))
        {
            name_ = std::string(name);
        }
        else
        {
            std::cerr << "Cannot get key name";
            name_ = "<unknown key>";
        }
    }

    WORD Key_win::virtualKey() const
    {
        return virtualKey_;
    }
}
