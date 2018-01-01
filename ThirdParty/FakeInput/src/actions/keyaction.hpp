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

#ifndef FI_KEYACTION_HPP
#define FI_KEYACTION_HPP

#include "action.hpp"
#include "key.hpp"

namespace FakeInput
{
    typedef void (*KeyActionCallback) (Key key);

    /** Represent a key action. */
    class KeyAction : public Action
    {
    public:
        /** KeyAction constructor.
         *
         * @param callback
         *     Callback whitch can handle with key.
         * @param key
         *     Key to be handled.
         */
        KeyAction(KeyActionCallback callback, Key key);

        /** Creates new copy of this action.
         *
         * @returns
         *     New copy of this action allocated on heap.
         */
        Action* clone() const;

        /** Performs this action. */
        void send() const;

    private:
        KeyActionCallback callback_;
        Key key_;
    };

    /** Represent a key press action. */
    class KeyboardPress : public KeyAction
    {
    public:
        /** KeyboardPress action constructor.
         *
         * @param key
         *    Key to be pressed.
         */
        KeyboardPress(Key key);

        /** KeyboardPress action constructor.
         *
         * @param keyType
         *    Type of key to be pressed.
         */
        KeyboardPress(KeyType keyType);
    };

    /** Represent a key release action. */
    class KeyboardRelease : public KeyAction
    {
    public:
        /** KeyboardRelease action constructor.
         *
         * @param key
         *    Key to be released.
         */
        KeyboardRelease(Key key);

        /** KeyboardRelease action constructor.
         *
         * @param keyType
         *    Type of key to be released.
         */
        KeyboardRelease(KeyType keyType);
    };
}

#endif
