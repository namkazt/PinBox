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

#ifndef FI_KEY_WIN_HPP
#define FI_KEY_WIN_HPP

#include "config.hpp"

#include <windows.h>

#include <string>

#include "key_base.hpp"
#include "types.hpp"

namespace FakeInput
{
    /** Represents real keyboard button with appropriate methods on Windows.
     *
     * @warning @image html windows.png
     *    Windows platform only.
     */
    class Key_win : public Key_base
    {
    public:
        /** Key constructor.
         *
         * Creates key representing no real key.
         */
        Key_win();

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
        Key_win(KeyType type);
                
        /** Key constructor
         *
         * Creates key from virtual-key code from given Windows key message.
         *
         * @param message
         *     Windows WM_[SYS]KEY(DOWN|UP) message containing virtual-key code.
         *
         * @warning @image html windows.png
         *    Windows platform only
         */
        Key_win(MSG* message);

        /** Key constructor
         *
         * Creates key from the name of the virtual key.
         *
         * @param virtualKey
         *     The virtual-key code.
         *     Virtual-key codes can be obtained from winuser.h header file
         *     or http://msdn.microsoft.com/en-us/library/dd375731 .
         *
         * @warning @image html windows.png
         *    Windows platform only.
         */
        Key_win(WORD virtualKey);

        /** Gives the virtual-key code representing the key.
         *
         * @returns
         *     virtual-key code representing the key.
         *
         * @warning @image html windows.png
         *    Windows platform only.
         */
        WORD virtualKey() const;

    private:
        WORD virtualKey_;
    };
}

#endif
