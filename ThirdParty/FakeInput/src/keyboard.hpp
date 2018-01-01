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

#ifndef FI_KEYBOARD_HPP
#define FI_KEYBOARD_HPP

#include "config.hpp"

#include "key.hpp"

namespace FakeInput
{
    /** Represents keyboard device.
     *
     * Allows you to simulate key press.
     */
    class Keyboard
    {
    public:
        /** Simulate key press.
         *
         * @param key
         *      Key object representing real key to be pressed.
         */
        static void pressKey(Key key);

        /** Simulate key release 
         *
         * @param key
         *      Key object representing real key to be released.
         */
        static void releaseKey(Key key);

    private:
        /** Send fake key event to the system.
         *
         * @param key
         *      Key object representing real key to be pressed.
         * @param isPress
         *      Whether event is press or release.
         */
        static void sendKeyEvent_(Key key, bool isPress);        
    };
}

#endif
