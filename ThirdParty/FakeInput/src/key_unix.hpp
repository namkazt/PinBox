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

#ifndef FI_KEY_UNIX_HPP
#define FI_KEY_UNIX_HPP

#include "config.hpp"

#include <X11/Xlib.h>

#include <string>

#include "key_base.hpp"
#include "types.hpp"

namespace FakeInput
{
    /** Represents real keyboard button with appropriate methods on Unix-like platform.
     *
     * @warning @image html tux.png
     *    Unix-like platform only
     */
    class Key_unix : public Key_base
    {
    public:
        /** Key constructor.
         *
         * Creates key representing no real key.
         */
        Key_unix();

        /** Key constructor
         *
         * Creates key according to the key type
         *
         * @param type
         *     KeyType to be created
         *
         * @note @image html note.png
         *     There is no quarantee the key type will act as you expected,
         *     it depends on keyboard layout, language etc., e.g. on Czech
         *     keyboard the @em Key_2 key will print @em 2 or @em ě according to
         *     the set keyboard layout (US or Czech)
         *
         * @note
         *     If there is not appropriate key type, you can use platform dependend
         *     methods Key::fromKeySym (unix) and Key::fromVirtualKey (win)
         */
        Key_unix(KeyType type);
        
        /** Key contructor
         *
         * Creates key from keycode from given X key event.
         *
         * @param event
         *     XKeyEvent (Xlib key event) containing keycode.
         *
         * @warning @image html tux.png
         *    Unix-like platform only
         */
        Key_unix(XEvent* event);

        /** Key contructor
         *
         * Creates key from the KeySym.
         *
         * @param keySym
         *     The KeySym representing appropriate key.
         *     KeySyms can be obtained from X11/keysymdef.h
         *     file in Xlib sources
         *
         * @warning @image html tux.png
         *    Unix-like platform only.
         */
        Key_unix(KeySym keySym);

        /** Gives the KeySym representing the key.
         *
         * @returns
         *     KeySym representing the key.
         *
         * @warning @image html tux.png
         *    Unix-like platform only
         */
        KeySym keysym() const;

    private:
        KeySym keysym_;
    };
}

#endif
