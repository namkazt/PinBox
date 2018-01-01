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

#include "mouse.hpp"

#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

#include "display_unix.hpp"

#include "mapper.hpp"

namespace FakeInput
{
    void Mouse::move(int x, int y)
    {
        XTestFakeRelativeMotionEvent(display(), x, y, CurrentTime);
        XFlush(display());
    }

    void Mouse::moveTo(int x, int y)
    {
        XTestFakeMotionEvent(display(), -1, x, y, CurrentTime);
        XFlush(display());
    }

    void Mouse::pressButton(MouseButton button)
    {
        int xButton = translateMouseButton(button);
        XTestFakeButtonEvent(display(), xButton, true, CurrentTime);
        XFlush(display());
    }

    void Mouse::releaseButton(MouseButton button)
    {
        int xButton = translateMouseButton(button);
        XTestFakeButtonEvent(display(), xButton, false, CurrentTime);
        XFlush(display());
    }

    void Mouse::wheelUp()
    {
        XTestFakeButtonEvent(display(), 4, true, CurrentTime);
        XSync(display(), false);
        XTestFakeButtonEvent(display(), 4, false, CurrentTime);
        XFlush(display());
    }

    void Mouse::wheelDown()
    {
        XTestFakeButtonEvent(display(), 5, true, CurrentTime);
        XSync(display(), false);
        XTestFakeButtonEvent(display(), 5, false, CurrentTime);
        XFlush(display());
    }
}
