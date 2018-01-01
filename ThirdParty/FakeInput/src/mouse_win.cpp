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

#include <windows.h>

#include "mapper.hpp"
#include "types.hpp"

#define INIT_INPUT(var) \
    INPUT var; \
    ZeroMemory(&var, sizeof(INPUT)); \
    var.type = INPUT_MOUSE;

namespace FakeInput
{
    void Mouse::move(int x, int y)
    {
        INIT_INPUT(input);
        input.mi.dx = x;
        input.mi.dy = y;
        input.mi.dwFlags = MOUSEEVENTF_MOVE;
        SendInput(1, &input, sizeof(INPUT));
    }

    void Mouse::moveTo(int x, int y)
    {
        double screenWidth = GetSystemMetrics(SM_CXSCREEN) - 1; 
        double screenHeight = GetSystemMetrics(SM_CYSCREEN) - 1; 
        double fx = x * (65535.0f / screenWidth);
        double fy = y * (65535.0f / screenHeight);

        INIT_INPUT(input);
        input.mi.dx = (LONG) fx;
        input.mi.dy = (LONG) fy;
        input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
        SendInput(1, &input, sizeof(INPUT));
    }

    void Mouse::pressButton(MouseButton button)
    {
        INIT_INPUT(input);
        input.mi.dwFlags = translateMouseButton(button);

        SendInput(1, &input, sizeof(INPUT));
    }

    void Mouse::releaseButton(MouseButton button)
    {
        INIT_INPUT(input);
        input.mi.dwFlags = translateMouseButton(button) << 1;

        SendInput(1, &input, sizeof(INPUT));
    }

    void Mouse::wheelUp()
    {
        INIT_INPUT(input);
        input.mi.dwFlags = MOUSEEVENTF_WHEEL;
        input.mi.mouseData = WHEEL_DELTA;

        SendInput(1, &input, sizeof(INPUT));
    }

    void Mouse::wheelDown()
    {
        INIT_INPUT(input);
        input.mi.dwFlags = MOUSEEVENTF_WHEEL;
        input.mi.mouseData = -WHEEL_DELTA;

        SendInput(1, &input, sizeof(INPUT));
    }
}
