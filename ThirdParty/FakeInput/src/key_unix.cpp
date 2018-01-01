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

#include "key_unix.hpp"

#include <stdexcept>
#include <iostream>

#include "display_unix.hpp"
#include "mapper.hpp"

namespace FakeInput
{
    Key_unix::Key_unix()
    {
        keysym_ = 0;
    }

    Key_unix::Key_unix(KeyType type)
    {
        if(type == Key_NoKey)
        {
            keysym_ = 0;
            code_ = 0;
            name_ = "<no key>";
        }
        else
        {
            keysym_ = translateKey(type);
            code_ = XKeysymToKeycode(display(), keysym_);
            name_ = XKeysymToString(keysym_);
        }
    }

    Key_unix::Key_unix(XEvent* event):
        Key_base()
    {
        if(event->type != KeyPress && event->type != KeyRelease)
            throw std::logic_error("Cannot get key from non-key event");

        code_ = event->xkey.keycode;
        keysym_ = XKeycodeToKeysym(event->xkey.display, code_, 0);
        name_ = XKeysymToString(keysym_);
    }

    Key_unix::Key_unix(KeySym keysym):
        Key_base()
    {
        code_ = XKeysymToKeycode(display(), keysym);
        keysym_ = keysym;
        name_ = XKeysymToString(keysym);
    }

    KeySym Key_unix::keysym() const
    {
        return keysym_;
    }
}
