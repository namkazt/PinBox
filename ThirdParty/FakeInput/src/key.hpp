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

#ifndef FI_KEY_HPP
#define FI_KEY_HPP

#include "config.hpp"

#ifdef UNIX
    #include "key_unix.hpp"
#elif WIN32
    #include "key_win.hpp"
#endif

namespace FakeInput
{
#ifdef UNIX
    /** Class representing a real key.
     *
     * @image html tux.png
     * On Unix-like platform derived from Key_unix
     */
    typedef Key_unix Key;
#elif WIN32
    /**
     * @image html windows.png
     * On Windows derived from Key_win
     */
    typedef Key_win Key;
#endif
}

#endif
