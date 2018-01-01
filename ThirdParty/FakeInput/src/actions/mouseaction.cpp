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

#include "mouseaction.hpp"

namespace FakeInput
{
    // -------- MOUSE BUTTONS --------- //
    MouseButtonAction::MouseButtonAction(MouseButtonActionCallback callback, MouseButton button):
        callback_(callback),
        button_(button)
    {
    }

    Action* MouseButtonAction::clone() const
    {
        return new MouseButtonAction(callback_, button_);
    }

    void MouseButtonAction::send() const
    {
        (*callback_)(button_);
    }

    MousePress::MousePress(MouseButton button):
        MouseButtonAction(Mouse::pressButton, button)
    {
    }

    MouseRelease::MouseRelease(MouseButton button):
        MouseButtonAction(Mouse::releaseButton, button)
    {
    }

    // -------- MOUSE MOTION --------- //
    MouseMotionAction::MouseMotionAction(MouseMotionActionCallback callback, int x, int y):
        callback_(callback),
        x_(x),
        y_(y)
    {
    }

    Action* MouseMotionAction::clone() const
    {
        return new MouseMotionAction(callback_, x_, y_);
    }

    void MouseMotionAction::send() const
    {
        callback_(x_, y_);
    }

    MouseRelativeMotion::MouseRelativeMotion(int dx, int dy):
        MouseMotionAction(Mouse::move, dx, dy)
    {
    }

    MouseAbsoluteMotion::MouseAbsoluteMotion(int x, int y):
        MouseMotionAction(Mouse::moveTo, x, y)
    {
    }

    // -------- MOUSE WHEEL --------- //
    MouseWheelAction::MouseWheelAction(MouseWheelActionCallback callback):
        callback_(callback)
    {
    }

    Action* MouseWheelAction::clone() const
    {
        return new MouseWheelAction(callback_);
    }

    void MouseWheelAction::send() const
    {
        callback_();
    }

    MouseWheelUp::MouseWheelUp():
        MouseWheelAction(Mouse::wheelUp)
    {
    }

    MouseWheelDown::MouseWheelDown():
        MouseWheelAction(Mouse::wheelDown)
    {
    }
}
