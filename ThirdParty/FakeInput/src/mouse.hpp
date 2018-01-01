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

#ifndef FI_MOUSE_HPP
#define FI_MOUSE_HPP

#include "config.hpp"
#include "types.hpp"

namespace FakeInput
{
    /** Represents mouse device.
     *
     * Allows you to move the cursor to the required position.
     * Or simulate mouse button press.
     */
    class Mouse
    {
    public:
        /** Moves mouse cursor in direction.
         *
         * @param xDirection
         *     Direction in pixels on X-axis.
         * @param yDirection
         *     Direction in pixels on Y-axis.
         */
        static void move(int xDirection, int yDirection);

        /** Moves mouse cursor to the specified position.
         *
         * @param x
         *     X coordinate of the position.
         * @param y
         *     Y coordinate of the position.
         */
        static void moveTo(int x, int y);

        /** Simulates mouse button press.
         *
         * @param button
         *     MouseButton to be pressed
         */
        static void pressButton(MouseButton button);

        /** Simulates mouse button release.
         *
         * @param button
         *     MouseButton to be released
         */
        static void releaseButton(MouseButton button);

        /** Simulates wheel up move */
        static void wheelUp();

        /** Simulates wheel up down */
        static void wheelDown();
    };
}

#endif
